#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QtSql>
#include <QTableWidget>
#include <memory>
#include "structs.h"

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    ~Database() override;

    void connect_to_database(const ConnectionData& conData);
    void disconnect_from_database();
    QSqlError get_last_error() const;
    bool is_connected() const;
    bool reset_tables();
    void insert_data(std::unique_ptr<IndexingData>&& indData);
    void request_words();
    void request_docs(const QStringList& words);
    void read_data(RequestType reqType);

signals:
    void sig_connection_status(bool connected);
    void sig_request_status(QSqlError error, RequestType reqType);
    void sig_inserting_finished();
    void sig_send_table(QTableWidget* table, RequestType reqType);

private:
    QSqlDatabase m_database;
    QSqlQuery m_query;
    QString m_connectionName;
    bool m_connected = false;
    bool execute(const QString& sql, RequestType reqType);
    void reportError(const QSqlError& error, RequestType reqType);
};

#endif // DATABASE_H
