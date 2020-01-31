#ifndef UTILS_H
#define UTILS_H

class QString;
class QIcon;
class QPixmap;
class QStringList;

#ifndef __FUNC__
#define __FUNC__ __PRETTY_FUNCTION__
#endif

#ifndef VAR
#define VAR(x) # x << (x)
#endif

#ifndef CDEBUG
#define CDEBUG qDebug() << __FUNC__ << __LINE__
#endif

class Utils
{
public:

    static QString convertIconToBase64(const QIcon &icon);
    static QIcon convertBase64ToIcon(const QString &iconBase64);

    static QPixmap loadPixmapFromDataUri(const QString &dataBase64);

    static QStringList split(const QString &str, const QString &sep);
};

#endif // UTILS_H
