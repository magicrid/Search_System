#include "database.h"

#include <QCoreApplication>
#include <QSet>

Database::Database(QObject *parent) : QObject(parent)
{
    m_connectionName = QStringLiteral("SearchSystemConnection_%1")
                           .arg(reinterpret_cast<quintptr>(this));
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connectionName);
    m_query = QSqlQuery(m_database);
}

Database::~Database()
{
    if (m_database.isValid()) {
        m_database.close();
    }
    m_query = QSqlQuery();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

void Database::reportError(const QSqlError& error, RequestType reqType)
{
    if (error.isValid()) emit sig_request_status(error, reqType);
}

bool Database::execute(const QString& sql, RequestType reqType)
{
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        reportError(query.lastError(), reqType);
        return false;
    }
    return true;
}

void Database::connect_to_database(const ConnectionData& conData)
{
    if (m_database.isOpen()) m_database.close();
    m_database.setHostName(conData.hostName.trimmed());
    m_database.setDatabaseName(conData.dbName.trimmed());
    m_database.setUserName(conData.user.trimmed());
    m_database.setPassword(conData.password);
    m_database.setPort(conData.port > 0 ? conData.port : 5432);
    m_connected = m_database.open();
    emit sig_connection_status(m_connected);
}

void Database::disconnect_from_database()
{
    if (m_database.isOpen()) m_database.close();
    m_connected = false;
}

QSqlError Database::get_last_error() const { return m_database.lastError(); }
bool Database::is_connected() const { return m_connected && m_database.isOpen(); }

bool Database::reset_tables()
{
    if (!is_connected()) {
        reportError(QSqlError(QStringLiteral("Нет подключения к базе данных"), QString(), QSqlError::ConnectionError), RequestType::CREATE_TABLES);
        return false;
    }

    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS documents (id INTEGER PRIMARY KEY, name TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS words (id INTEGER PRIMARY KEY, name TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS document_word (id BIGSERIAL PRIMARY KEY, document_id INTEGER NOT NULL REFERENCES documents(id) ON DELETE CASCADE, word_id INTEGER NOT NULL REFERENCES words(id) ON DELETE CASCADE)")
    };
    for (const QString& sql : statements) {
        if (!execute(sql, RequestType::CREATE_TABLES)) return false;
    }
    return true;
}

void Database::insert_data(std::unique_ptr<IndexingData>&& indData)
{
    if (!indData || !is_connected()) {
        reportError(QSqlError(QStringLiteral("Нет данных для индексации или отсутствует подключение"), QString(), QSqlError::ConnectionError), RequestType::INSERT_DATA);
        return;
    }

    m_query = QSqlQuery();
    if (!m_database.transaction()) {
        reportError(m_database.lastError(), RequestType::INSERT_DATA);
        return;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("DELETE FROM document_word")) ||
        !query.exec(QStringLiteral("DELETE FROM documents")) ||
        !query.exec(QStringLiteral("DELETE FROM words"))) {
        const QSqlError error = query.lastError();
        m_database.rollback();
        reportError(error, RequestType::INSERT_DATA);
        return;
    }

    query.prepare(QStringLiteral("INSERT INTO documents (id, name) VALUES (:id, :name)"));
    for (auto it = indData->docs.cbegin(); it != indData->docs.cend(); ++it) {
        query.bindValue(QStringLiteral(":id"), it.value());
        query.bindValue(QStringLiteral(":name"), it.key());
        if (!query.exec()) {
            const QSqlError error = query.lastError();
            m_database.rollback(); reportError(error, RequestType::INSERT_DATA); return;
        }
    }

    query.prepare(QStringLiteral("INSERT INTO words (id, name) VALUES (:id, :name)"));
    for (auto it = indData->words.cbegin(); it != indData->words.cend(); ++it) {
        query.bindValue(QStringLiteral(":id"), it.value());
        query.bindValue(QStringLiteral(":name"), it.key());
        if (!query.exec()) {
            const QSqlError error = query.lastError();
            m_database.rollback(); reportError(error, RequestType::INSERT_DATA); return;
        }
    }

    query.prepare(QStringLiteral("INSERT INTO document_word (document_id, word_id) VALUES (:document_id, :word_id)"));
    for (const auto& pair : indData->docWord) {
        query.bindValue(QStringLiteral(":document_id"), pair.first);
        query.bindValue(QStringLiteral(":word_id"), pair.second);
        if (!query.exec()) {
            const QSqlError error = query.lastError();
            m_database.rollback(); reportError(error, RequestType::INSERT_DATA); return;
        }
    }

    if (!m_database.commit()) {
        const QSqlError error = m_database.lastError();
        m_database.rollback(); reportError(error, RequestType::INSERT_DATA); return;
    }
    emit sig_inserting_finished();
}

void Database::request_words()
{
    m_query = QSqlQuery(m_database);
    if (!m_query.exec(QStringLiteral(
            "SELECT w.name, COUNT(dw.word_id) AS frequency FROM words w "
            "LEFT JOIN document_word dw ON w.id = dw.word_id "
            "GROUP BY w.id, w.name ORDER BY w.name"))) {
        reportError(m_query.lastError(), RequestType::SELECT_WORDS);
        return;
    }
    emit sig_request_status(QSqlError(), RequestType::SELECT_WORDS);
}

void Database::request_docs(const QStringList& inputWords)
{
    QStringList words;
    QSet<QString> seen;
    for (QString word : inputWords) {
        word = word.trimmed().toCaseFolded();
        if (!word.isEmpty() && !seen.contains(word)) {
            seen.insert(word);
            words.append(word);
        }
    }
    if (words.isEmpty()) {
        m_query = QSqlQuery(m_database);
        if (!m_query.exec(QStringLiteral("SELECT name, 0 FROM documents WHERE FALSE"))) {
            reportError(m_query.lastError(), RequestType::SELECT_DOCS);
            return;
        }
        emit sig_request_status(QSqlError(), RequestType::SELECT_DOCS);
        return;
    }

    QStringList placeholders;
    for (int i = 0; i < words.size(); ++i) placeholders << QStringLiteral(":word%1").arg(i);
    const QString sql = QStringLiteral(
        "SELECT d.name, COUNT(DISTINCT w.id) AS rank "
        "FROM documents d JOIN document_word dw ON d.id = dw.document_id "
        "JOIN words w ON w.id = dw.word_id WHERE w.name IN (%1) "
        "GROUP BY d.id, d.name ORDER BY rank DESC, d.name ASC LIMIT 10")
        .arg(placeholders.join(QStringLiteral(", ")));

    m_query = QSqlQuery(m_database);
    m_query.prepare(sql);
    for (int i = 0; i < words.size(); ++i)
        m_query.bindValue(placeholders.at(i), words.at(i));
    if (!m_query.exec()) {
        reportError(m_query.lastError(), RequestType::SELECT_DOCS);
        return;
    }
    emit sig_request_status(QSqlError(), RequestType::SELECT_DOCS);
}

void Database::read_data(RequestType reqType)
{
    auto *table = new QTableWidget;
    table->setAttribute(Qt::WA_DeleteOnClose, false);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(reqType == RequestType::SELECT_DOCS
                                         ? QStringList{QStringLiteral("Файл"), QStringLiteral("Ранг")}
                                         : QStringList{QStringLiteral("Слово"), QStringLiteral("Частота")});
    int row = 0;
    while (m_query.next()) {
        table->insertRow(row);
        for (int col = 0; col < 2; ++col) {
            table->setItem(row, col, new QTableWidgetItem(m_query.value(col).toString()));
        }
        ++row;
    }
    table->resizeColumnsToContents();
    emit sig_send_table(table, reqType);
}
