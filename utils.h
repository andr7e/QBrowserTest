#ifndef UTILS_H
#define UTILS_H

class QString;
class QIcon;

class Utils
{
public:

    static QString convertIconToBase64(const QIcon &icon);
    static QIcon convertBase64ToIcon(const QString &iconBase64);
};

#endif // UTILS_H
