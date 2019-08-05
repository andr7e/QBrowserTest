#include "searchenginemanager.h"
#include "settingsmanager.h"
#include "utils.h"

#include <QUrlQuery>
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>
#include <QDebug>

SearchEngineManager * SearchEngineManager::s_instance = nullptr;

SearchEngineManager::SearchEngineManager()
{

}

QUrl SearchEngineManager::getUrl(const QString &searchText)
{
    SearchEngineManager *manager = SearchEngineManager::instance();
    QString key = manager->getCurrentName();
    SearchEngine searchEngine = manager->get(key);

    //CDEBUG << searchEngine;

    QString templateUrl = searchEngine.templateUrl;

    QString lang = QLocale().name().left(2);

    templateUrl.replace("{searchTerms}", searchText);
    templateUrl.replace("{sourceid}",    "chrome");
    templateUrl.replace("{lang}",        lang);

    QUrl url = QUrl(templateUrl);

    return url;

    /*
    QUrl url(QLatin1String("http://www.google.com/search"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QLatin1String("q"), searchText);
    urlQuery.addQueryItem(QLatin1String("ie"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("oe"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("client"), QLatin1String("qtdemobrowser"));
    url.setQuery(urlQuery);
    */

    /*
    QUrl url(QLatin1String("https://duckduckgo.com"));
    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QLatin1String("q"), searchText);
    urlQuery.addQueryItem(QLatin1String("ie"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("oe"), QLatin1String("UTF-8"));
    url.setQuery(urlQuery);
    */

    // "https://search.yahoo.com/search?p={searchTerms}&amp;fr=opensearch&amp;sourceid={sourceid}"

    //return url;
}


SearchEngineManager *SearchEngineManager::instance()
{
    if (s_instance == nullptr)
    {
        s_instance = new SearchEngineManager();
    }

    return s_instance;
}

QStringList SearchEngineManager::getList()
{
    return m_engines.keys();
}

SearchEngine SearchEngineManager::get(const QString &name)
{
    return m_engines[name];
}

QString SearchEngineManager::getDefaultName()
{
    return SettingsManager::getDefaultSearch();
}

QString SearchEngineManager::getCurrentName() const
{
    if ( ! m_currentSearch.isEmpty()) return m_currentSearch;

    return SearchEngineManager::getDefaultName();
}

void SearchEngineManager::setCurrentName(const QString &name)
{
    m_currentSearch = name;
}

void SearchEngineManager::load()
{
    QString folder = ":/search";

    QDir dir(folder);

    QStringList engines = dir.entryList();
    qSort(engines);

    qDebug() << engines;

    foreach(QString engine, engines)
    {
        loadEngine(folder + QDir::separator() + engine);
    }
}

void SearchEngineManager::loadEngine(const QString &filePath)
{
    SearchEngine searchEngine;

    QFile file(filePath);

    //qDebug() << filePath;

    if (file.open(QIODevice::ReadOnly))
    {
        QXmlStreamReader reader(&file);

        if (reader.readNextStartElement())
        {
            if (reader.name() != QLatin1String("OpenSearchDescription"))
            {
                return;
            }
        }

        while (!reader.atEnd())
        {
            if (reader.readNextStartElement())
            {
                if (reader.name() == QLatin1String("ShortName"))
                {
                    searchEngine.name = reader.readElementText();
                }
                else if (reader.name() == QLatin1String("Description"))
                {
                    searchEngine.desc = reader.readElementText();
                }
                else if (reader.name() == QLatin1String("InputEncoding"))
                {
                    searchEngine.encoding = reader.readElementText();
                }
                else if (reader.name() == QLatin1String("Url"))
                {
                    QXmlStreamAttributes attributes = reader.attributes();

                    /*
                    foreach(QXmlStreamAttribute attr, attributes)
                    {
                        qDebug() << attr.name().toString();
                        qDebug() << attr.value().toString();
                    }
                    */

                    if (attributes.value("rel")  == "results" ||
                        attributes.value("type") == "text/html")
                    {
                        searchEngine.templateUrl = attributes.value("template").toString();
                    }
                }
                else if (reader.name() == QLatin1String("Image"))
                {
                    // data:image/png;base64,XXX

                    const QString data = reader.readElementText();

                    if (data.startsWith(QLatin1String("data:image/")))
                    {
                        int index = data.indexOf(QLatin1String("base64,"));

                        if (index >= 0)
                        {
                            searchEngine.iconBase64 = data.mid(index + 7);
                            searchEngine.icon = QIcon(Utils::loadPixmapFromDataUri(searchEngine.iconBase64));
                            //searchEngine.icon = Utils::convertBase64ToIcon(searchEngine.iconBase64);
                        }
                    }
                    else
                    {
                        //searchEngine.iconUrl = QUrl(data);
                    }
                }

            }
        }
    }

    QFileInfo fi(filePath);
    QString key = fi.fileName();
    key.remove(".xml");

    m_engines[key] = searchEngine;

    qDebug() << key << searchEngine;
}

//

QDebug operator<<(QDebug debug, const SearchEngine &searchEngine)
{
    debug << searchEngine.name;
    debug << searchEngine.desc;
    debug << searchEngine.shortcut;
    debug << searchEngine.templateUrl;
    debug << searchEngine.encoding;
    debug << searchEngine.iconBase64.length();

    return debug.space();
}
