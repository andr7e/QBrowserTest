#ifndef STARTPAGEWIDGET_H
#define STARTPAGEWIDGET_H

#include <QWidget>
#include <QStandardItemModel>

#include "bookmarks.h"


namespace Ui {
class StartPageWidget;
}

class StartPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StartPageWidget(QWidget *parent = 0);
    ~StartPageWidget();

    void updateInfo(BookmarksModel *bookmarksModel);

signals:
    void openUrl(QUrl url);

public slots:
    void onCustomContextMenuRequested(const QPoint &pos);
    void openItem();

private:
    Ui::StartPageWidget *ui;

    QStandardItemModel *m_model;
};

#include <QStyledItemDelegate>

class TileDelegate : public QStyledItemDelegate
{
public:
    TileDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    void drawIcon(QPainter *painter, const QIcon &icon, QPoint pos) const;
};

#endif // STARTPAGEWIDGET_H
