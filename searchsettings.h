#ifndef SEARCHSETTINGS_H
#define SEARCHSETTINGS_H

#include <QSettings>
#include <QObject>
#include "structs.h"

class SearchSettings : public QSettings
{
    Q_OBJECT
public:
    explicit SearchSettings(QObject *parent = nullptr);

    ConnectionData read_connection_settings();
    void write_connection_settings(const ConnectionData& conData);

    IndexingParameters read_indexing_settings();
    void write_indexing_settings(const IndexingParameters& indParams);
};

#endif // SEARCHSETTINGS_H
