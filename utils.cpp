#include "utils.h"

#include <QPixmap>
#include <QBuffer>
#include <QImage>
#include <QIcon>

QString Utils::convertIconToBase64(const QIcon &icon)
{
    QImage image(icon.pixmap(24,24).toImage());

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    image.save(&buffer, "PNG"); // writes the image in PNG format inside the buffer
    QString base64 = QString::fromLatin1(byteArray.toBase64().data());

    return base64;
}

QIcon Utils::convertBase64ToIcon(const QString &iconBase64)
{
    QByteArray byteArray = QByteArray::fromBase64(iconBase64.toLatin1());

    QBuffer buffer(&byteArray);

    QImage image;
    image.load(&buffer, "PNG");

    QPixmap pixmap = QPixmap::fromImage(image);

    return QIcon(pixmap);
}

QPixmap Utils::loadPixmapFromDataUri(const QString &dataBase64)
{
    return QPixmap::fromImage(QImage::fromData(QByteArray::fromBase64(dataBase64.toUtf8())));
}

QStringList Utils::split(const QString &str, const QString &sep)
{
    if ( ! str.isEmpty())
    {
        return str.split(sep, QString::SkipEmptyParts);
    }

    return QStringList();
}
