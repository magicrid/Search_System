#include "searchsettings.h"

SearchSettings::SearchSettings(QObject *parent)
    : QSettings{QSettings::IniFormat,QSettings::UserScope,"FaaZMaaR","Search_System",parent}
{

}

ConnectionData SearchSettings::read_connection_settings(){
    ConnectionData conData;
    beginGroup("ConnectionToDB");
    conData.hostName=value("hostName").toString();
    conData.dbName=value("dbName").toString();
    conData.user=value("user").toString();
    conData.password=value("password").toString();
    conData.port=value("port", 5432).toInt();
    if (conData.port < 1 || conData.port > 65535) conData.port = 5432;
    endGroup();
    return conData;
}

void SearchSettings::write_connection_settings(const ConnectionData& conData){
    beginGroup("ConnectionToDB");
    setValue("hostName",conData.hostName);
    setValue("dbName",conData.dbName);
    setValue("user",conData.user);
    setValue("password",conData.password);
    setValue("port",conData.port);
    endGroup();
}

IndexingParameters SearchSettings::read_indexing_settings(){
    IndexingParameters indParams;
    beginGroup("Indexing");
    const QVariant dirsValue = value("dirs");
    const QVariant extensionsValue = value("extensions");
    endGroup();

    if (dirsValue.canConvert<QStringList>()) {
        indParams.dirs = dirsValue.toStringList();
    } else if (!dirsValue.toString().isEmpty()) {
        indParams.dirs = dirsValue.toString().split(';', Qt::SkipEmptyParts);
    }
    if (extensionsValue.canConvert<QStringList>()) {
        indParams.extensions = extensionsValue.toStringList();
    } else if (!extensionsValue.toString().isEmpty()) {
        indParams.extensions = extensionsValue.toString().split(',', Qt::SkipEmptyParts);
    }
    for (QString& dir : indParams.dirs) dir = dir.trimmed();
    for (QString& ext : indParams.extensions) {
        ext = ext.trimmed();
        if (ext.startsWith('.')) ext.remove(0, 1);
        ext = ext.toLower();
    }
    indParams.dirs.removeDuplicates();
    indParams.extensions.removeDuplicates();
    return indParams;
}

void SearchSettings::write_indexing_settings(const IndexingParameters& indParams){
    beginGroup("Indexing");
    setValue("dirs", indParams.dirs);
    setValue("extensions", indParams.extensions);
    endGroup();
}
