#ifndef PREVIEWMANAGER_H
#define PREVIEWMANAGER_H

#include <QString>
#include <QImage>

class WebView;

class PreviewManager
{
public:
    PreviewManager();

    static QImage get(const QString &url);

    static void create(WebView *m_webView, const QString &m_url, const QSize &m_size);

private:
    static QString getUrlKey(const QString &url);
};

#endif // PREVIEWMANAGER_H
