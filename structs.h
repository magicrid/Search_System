#ifndef STRUCTS_H
#define STRUCTS_H

#include <QStringList>
#include <QMap>
#include <QVector>
#include <QPair>

struct ConnectionData{
    QString hostName;
    QString dbName;
    QString user;
    QString password;
    int port = 5432;
};

struct IndexingParameters{
    QStringList dirs;
    QStringList extensions;
};

struct IndexingData{
    QMap<QString,int> docs;
    QMap<QString,int> words;
    QVector<QPair<int,int>> docWord;
};

enum class RequestType{
    CREATE_TABLES,
    INSERT_DATA,
    SELECT_WORDS,
    SELECT_DOCS
};

#endif // STRUCTS_H
