#include "webengineurlrequestinterceptor.h"
#include "utils.h"
#include "browserapplication.h"

#include <QDebug>

WebEngineUrlRequestInterceptor::WebEngineUrlRequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent)
{

}

void WebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    CDEBUG << info.firstPartyUrl();
    CDEBUG << info.requestUrl();

    QUrl baseUrl = info.firstPartyUrl();

    QString hostname = baseUrl.host();\

    CDEBUG << VAR(hostname);

    QWebEngineUrlRequestInfo::ResourceType resourceType = info.resourceType();
    CDEBUG << resourceType;

    BlockingManager::Mode mode = BrowserApplication::blockingManager()->getMode(hostname);

    CDEBUG << VAR(mode);

    switch(mode)
    {
        case BlockingManager::Off:
        break;

        case BlockingManager::Aggressive:
        {
            bool ok = (resourceType == QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeStylesheet ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeImage ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeFavicon ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeScript);

            if ( ! ok)
                info.block(true);
        }
        break;

        case BlockingManager::Aggressive_NoJS:
        {
            bool ok = (resourceType == QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeStylesheet ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeImage ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeFavicon);

            if ( ! ok)
                info.block(true);
        }
        break;

        case BlockingManager::Aggressive_NoImage:
        {
            bool ok = (resourceType == QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeStylesheet ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeFavicon);

            if ( ! ok)
                info.block(true);
        }
        break;

        default : break;
    }

    /*
    if (resourceType == QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
        resourceType == QWebEngineUrlRequestInfo::ResourceTypeStylesheet ||
        resourceType == QWebEngineUrlRequestInfo::ResourceTypeImage ||
        resourceType == QWebEngineUrlRequestInfo::ResourceTypeFavicon)
    {
        if (resourceType == QWebEngineUrlRequestInfo::ResourceTypeImage &&
                info.requestUrl().host() != info.firstPartyUrl().host())
        {
            info.block(true);
        }
    }
    else
    {
        info.block(true);
    }
    */
}

//

#include <QSettings>

BlockingManager::Mode BlockingManager::getMode(const QString &hostname)
{
    QString host = hostname;
    if (host.startsWith("wwww.")) host = host.mid(4);

    if (hostModeHash_.contains(host))
    {
        int mode = hostModeHash_[host];

        return BlockingManager::Mode(mode);
    }

    return defaultMode;
}

void BlockingManager::setMode(const QString &hostname, BlockingManager::Mode mode)
{
    QString host = hostname;
    if (host.startsWith("wwww.")) host = host.mid(4);

    hostModeHash_[host] = mode;
}

#include <QSettings>

void BlockingManager::loadSettings()
{
    QSettings s;
    s.beginGroup(QLatin1String("blocking"));
    QString info = s.value("list").toString();
    s.endGroup();

    QStringList lines = Utils::split(info, ";");
    for (QString line : lines)
    {
        QStringList items = Utils::split(line, "=");

        if (items.size() == 2)
        {
            QString host = items[0];
            int mode     = items[1].toInt();

            hostModeHash_[host] = mode;
        }
    }
}

void BlockingManager::saveSettings()
{
    QString info;

    for (QString host : hostModeHash_.keys())
    {
        int mode = hostModeHash_[host];

        if (mode != defaultMode)
        {
            info += host + "=" +  QString::number(mode) + ";";
        }
    }

    QSettings s;
    s.beginGroup(QLatin1String("blocking"));
    s.setValue("list", info);
    s.endGroup();
}
