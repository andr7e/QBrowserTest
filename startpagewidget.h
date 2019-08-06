#ifndef STARTPAGEWIDGET_H
#define STARTPAGEWIDGET_H

#include <QWidget>
#include <QStandardItemModel>

#include "bookmarks.h"

class WebView;

namespace Ui {
class StartPageWidget;
}

class StartPageWidget : public QWidget
{
    Q_OBJECT

public:

    enum Roles
    {
        VirtualRole = Qt::UserRole,
        PreviewRole = Qt::UserRole + 1
    };

    explicit StartPageWidget(QWidget *parent = 0);
    ~StartPageWidget();

    void updateInfo();

signals:
    void openUrl(QUrl url);

public slots:
    void onCustomContextMenuRequested(const QPoint &pos);
    void openItem();
    void addItem();
    void slotAddBookmark(const QModelIndex &index);

private slots:
    void on_settingsButton_clicked();
    //void loadFinished();

private:
    void openItem(const QModelIndex &index);

private:
    Ui::StartPageWidget *ui;

    QStandardItemModel *m_model;

    QMainWindow *m_chooseBookmarkWindow;

    WebView *m_webView;
    QString m_url;
};

#include <QStyledItemDelegate>

class TileDelegate : public QStyledItemDelegate
{
public:
    TileDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void drawIcon(QPainter *painter, const QIcon &icon, QSize iconSize, QPoint pos) const;

private:
    QSize m_tileSize;
};

#endif // STARTPAGEWIDGET_H
