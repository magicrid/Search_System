#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QSet>
#include "indexer.h"

Indexer::Indexer(QObject *parent) : QObject(parent) {}

void Indexer::search_dir(const QDir& dir, const QStringList& filter)
{
    const QString canonicalDir = dir.canonicalPath();
    if (canonicalDir.isEmpty() || m_visitedDirs.contains(canonicalDir)) return;
    m_visitedDirs.insert(canonicalDir);

    const QFileInfoList directories = dir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
    for (const QFileInfo& info : directories) {
        search_dir(QDir(info.absoluteFilePath()), filter);
    }

    const QFileInfoList files = dir.entryInfoList(
        filter, QDir::Files | QDir::Readable | QDir::NoSymLinks, QDir::Name);
    for (const QFileInfo& info : files) {
        const QString canonicalFile = info.canonicalFilePath();
        if (canonicalFile.isEmpty() || m_visitedFiles.contains(canonicalFile)) continue;
        m_visitedFiles.insert(canonicalFile);
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const int currentDocId = m_doc_id;
        m_indData->docs.insert(info.absoluteFilePath(), currentDocId);
        process_file(file);
        file.close();
        ++m_doc_id;
    }
}

void Indexer::process_file(QFile& file)
{
    if (!file.isOpen() && !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    const QString fileData = in.readAll();
    const QStringList words = fileData.split(
        QRegularExpression(QStringLiteral("[^\\p{L}]+")), Qt::SkipEmptyParts);

    for (const QString& word : words) {
        if (word.size() < 3 || word.size() > 32)
            continue;
        const QString normalized = word.toCaseFolded();
        auto it = m_indData->words.constFind(normalized);
        if (it == m_indData->words.cend()) {
            m_indData->words.insert(normalized, m_word_id);
            it = m_indData->words.constFind(normalized);
            ++m_word_id;
        }
        m_indData->docWord.append(qMakePair(m_doc_id, it.value()));
    }
}

std::unique_ptr<IndexingData> Indexer::start(const IndexingParameters& indParams)
{
    m_indData = std::make_unique<IndexingData>();
    m_doc_id = 1;
    m_word_id = 1;
    m_visitedDirs.clear();
    m_visitedFiles.clear();

    if (indParams.dirs.isEmpty()) {
        emit sig_no_dirs();
        return std::move(m_indData);
    }
    if (indParams.extensions.isEmpty()) {
        emit sig_no_extensions();
        return std::move(m_indData);
    }

    QStringList filter;
    for (QString ext : indParams.extensions) {
        ext = ext.trimmed();
        if (ext.startsWith('.')) ext.remove(0, 1);
        if (!ext.isEmpty()) filter << QStringLiteral("*.%1").arg(ext);
    }
    filter.removeDuplicates();
    if (filter.isEmpty()) {
        emit sig_no_extensions();
        return std::move(m_indData);
    }

    for (const QString& path : indParams.dirs) {
        QDir dir(path.trimmed());
        if (!dir.exists()) {
            emit sig_dir_not_exists(dir.absolutePath());
            continue;
        }
        search_dir(dir, filter);
    }
    return std::move(m_indData);
}
