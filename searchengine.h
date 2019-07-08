#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <QUrl>

class SearchEngine
{
public:
    SearchEngine();

    static QUrl getUrl(const QString &searchText);
};

#endif // SEARCHENGINE_H
