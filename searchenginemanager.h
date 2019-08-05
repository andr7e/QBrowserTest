#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QUrl>
#include <QDebug>
#include <QIcon>

struct SearchEngine
{
    QString name;
    QString desc;
    QString shortcut;
    QString templateUrl;
    QString encoding;
    QIcon icon;
    QString iconBase64;
};

class SearchEngineManager
{
public:

    SearchEngineManager();

    static SearchEngineManager *instance();

    static QUrl getUrl(const QString &searchText);

    QStringList getList();
    SearchEngine get(const QString &name);

    static QString getDefaultName();

    QString getCurrentName() const;
    void setCurrentName(const QString &name);

    void load();
    void loadEngine(const QString &filePath);

private:
    static SearchEngineManager *s_instance;
    QHash<QString,SearchEngine> m_engines;

    QString m_currentSearch;
};

QDebug operator<<(QDebug debug, const SearchEngine &searchEngine);

#endif // SEARCHENGINE_H
