#include "settingsmanager.h"

#include <QSettings>

SettingsManager::SettingsManager()
{

}

QString SettingsManager::getDefaultSearch()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("search"));
    QString searchKey = settings.value(QLatin1String("default"), "google").toString();
    settings.endGroup();

    return searchKey;
}
