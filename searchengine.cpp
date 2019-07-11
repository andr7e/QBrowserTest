#include "searchengine.h"

#include <QUrlQuery>

SearchEngine::SearchEngine()
{

}

QUrl SearchEngine::getUrl(const QString &searchText)
{
    QUrl url(QLatin1String("http://www.google.com/search"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QLatin1String("q"), searchText);
    urlQuery.addQueryItem(QLatin1String("ie"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("oe"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("client"), QLatin1String("qtdemobrowser"));
    url.setQuery(urlQuery);

    /*
    QUrl url(QLatin1String("https://duckduckgo.com"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QLatin1String("q"), searchText);
    urlQuery.addQueryItem(QLatin1String("ie"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("oe"), QLatin1String("UTF-8"));
    url.setQuery(urlQuery);
    */

    return url;
}
