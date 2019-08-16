#include "webengineurlrequestinterceptor.h"
#include "utils.h"

#include <QDebug>

WebEngineUrlRequestInterceptor::WebEngineUrlRequestInterceptor(QObject *parent) : QWebEngineUrlRequestInterceptor(parent)
{

}

void WebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    CDEBUG << info.firstPartyUrl();
    CDEBUG << info.requestUrl();

    QWebEngineUrlRequestInfo::ResourceType resourceType = info.resourceType();
    CDEBUG << resourceType;

    Blocking::Mode mode = Blocking::getMode();

    switch(mode)
    {
        case Blocking::Off:
        break;

        case Blocking::Aggressive:
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

        case Blocking::Aggressive_NoJS:
        {
            bool ok = (resourceType == QWebEngineUrlRequestInfo::ResourceTypeMainFrame ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeStylesheet ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeImage ||
                resourceType == QWebEngineUrlRequestInfo::ResourceTypeFavicon);

            if ( ! ok)
                info.block(true);
        }
        break;

        case Blocking::Aggressive_NoImage:
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

Blocking::Mode Blocking::s_mode = Blocking::Aggressive_NoJS;

Blocking::Mode Blocking::getMode()
{
    return s_mode;
}

void Blocking::setMode(Blocking::Mode mode)
{
    s_mode = mode;
}
