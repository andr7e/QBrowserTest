#include "htmltemplatemanager.h"

#include <QFile>

QHash<QString,QString> HtmlTemplateManager::m_hash;

HtmlTemplateManager::HtmlTemplateManager()
{

}

QString HtmlTemplateManager::get(const QString &key)
{
    if ( ! m_hash.contains(key))
    {
        QString path = QString(":/%1.html").arg(key);

        QFile file(path);

        bool ok = file.open(QIODevice::ReadOnly);

        if (ok)
        {
            QString html = QString(QLatin1String(file.readAll()));

            m_hash[key] = html;
        }
    }

    return m_hash.value(key);
}
