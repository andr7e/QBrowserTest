#ifndef WEBENGINEURLREQUESTINTERCEPTOR_H
#define WEBENGINEURLREQUESTINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

class Blocking
{
public:
    Blocking(){}

    enum Mode
    {
        Off,
        Aggressive,
        Aggressive_NoImage
    };

    static Mode getMode();
    static void setMode(Mode mode);

private:
    static Mode s_mode;
};

class WebEngineUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:

    WebEngineUrlRequestInterceptor(QObject *parent);

    virtual void interceptRequest(QWebEngineUrlRequestInfo &info);
};

#endif // WEBENGINEURLREQUESTINTERCEPTOR_H
