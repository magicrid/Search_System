#ifndef INDEXER_H
#define INDEXER_H

#include <QObject>
#include <QDir>
#include <QFile>
#include <memory>
#include <QSet>
#include "structs.h"

class Indexer : public QObject
{
    Q_OBJECT

private:
    int m_doc_id;
    int m_word_id;
    std::unique_ptr<IndexingData> m_indData;
    QSet<QString> m_visitedDirs;
    QSet<QString> m_visitedFiles;

    void search_dir(const QDir& dir,const QStringList& filter);
    void process_file(QFile& file);

public:
    explicit Indexer(QObject *parent = nullptr);

    std::unique_ptr<IndexingData> start(const IndexingParameters& indParams);


signals:
    void sig_no_dirs();
    void sig_no_extensions();
    void sig_dir_not_exists(QString);
};

#endif // INDEXER_H
