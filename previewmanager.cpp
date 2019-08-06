#include "previewmanager.h"

#include <QCryptographicHash>

#include "webview.h"
#include "utils.h"

PreviewManager::PreviewManager()
{

}

QString PreviewManager::getUrlKey(const QString &url)
{
    QByteArray md5data = QCryptographicHash::hash(url.toLocal8Bit(), QCryptographicHash::Md5);

    return md5data.toHex();
}

QImage PreviewManager::get(const QString &url)
{
    QImage image;

    QString key = getUrlKey(QUrl(url).host());

    CDEBUG << key;

    image.load(key);

    return image;
}

void PreviewManager::create(WebView *m_webView, const QString &m_url, const QSize &m_size)
{
    QSize contentsSize = m_webView->page()->view()->size();

    CDEBUG << VAR(contentsSize);

    QPixmap m_pixmap = QPixmap(contentsSize);
    m_pixmap.fill(Qt::white);

    QPainter painter(&m_pixmap);
    m_webView->page()->view()->render(&painter);
    painter.end();

    m_pixmap = m_pixmap.scaled(m_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QImage image = m_pixmap.toImage();

    //QString url = webView->url().toString();

    QString key = getUrlKey(m_url);

    CDEBUG << m_url << key;

    image.save(key + ".png");
}
