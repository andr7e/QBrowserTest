#ifndef HTMLTEMPLATEMANAGER_H
#define HTMLTEMPLATEMANAGER_H

#include <QHash>

class HtmlTemplateManager
{
public:
    HtmlTemplateManager();

    static QString get(const QString &key);

private:
    static QHash<QString,QString> m_hash;
};

#endif // HTMLTEMPLATEMANAGER_H
