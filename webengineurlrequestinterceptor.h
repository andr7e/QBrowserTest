#ifndef WEBENGINEURLREQUESTINTERCEPTOR_H
#define WEBENGINEURLREQUESTINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

class BlockingManager
{
public:

    enum Mode
    {
        Off,
        Aggressive,
        Aggressive_NoJS,
        Aggressive_NoImage
    };

    BlockingManager() : defaultMode(BlockingManager::Aggressive_NoJS)
    {

    }

    Mode getMode(const QString &hostname);
    void setMode(const QString &hostname, Mode mode);

    void loadSettings();
    void saveSettings();

private:
    Mode defaultMode;
    QHash<QString,int> hostModeHash_;
};

class WebEngineUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:

    WebEngineUrlRequestInterceptor(QObject *parent);

    virtual void interceptRequest(QWebEngineUrlRequestInfo &info);
};

#endif // WEBENGINEURLREQUESTINTERCEPTOR_H
