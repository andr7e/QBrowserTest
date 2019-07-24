#include "startpagewidget.h"
#include "ui_startpagewidget.h"

#include <QDebug>
#include <QPainter>

#include "bookmarks.h"
#include "xbel.h"

StartPageWidget::StartPageWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StartPageWidget)
{
    ui->setupUi(this);

    m_model = new QStandardItemModel(this);

    m_model->setRowCount(2);
    m_model->setColumnCount(2);

    ui->listView->setViewMode(QListView::IconMode);

    ui->listView->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listView->setFrameStyle(QFrame::NoFrame);
    ui->listView->setStyleSheet(QLatin1String("QListView { padding:0; border:0; background:transparent; }"));

    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listView->setTabKeyNavigation(true);

    ui->listView->setMovement(QListView::Static);

    ui->listView->viewport()->setAttribute(Qt::WA_Hover);
    ui->listView->viewport()->setMouseTracking(true);
    ui->listView->viewport()->installEventFilter(this);

    ui->listView->setItemDelegate(new TileDelegate(ui->listView));
    ui->listView->setIconSize(QSize(200,200));

    //ui->listView->setLayoutMode(QListView::Batched);
    //ui->listView->setBatchSize(100);
    ui->listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //ui->listView->setAutoScroll(false);
    //ui->listView->setUniformItemSizes(true);
    //ui->listView->setResizeMode(QListView::Adjust);

    ui->listView->setModel(m_model);

    connect(ui->listView, &QListView::doubleClicked,  this, [=](QModelIndex index) {
        openItem(index);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect( this, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(onCustomContextMenuRequested(QPoint)) );
}

StartPageWidget::~StartPageWidget()
{
    delete ui;
}

void StartPageWidget::updateInfo(BookmarksModel *bookmarksModel)
{
    m_model->setRowCount(0);

    int row = 0;
    for (int i = 0; i < bookmarksModel->rowCount(); i++)
    {
        QModelIndex parentIndex = bookmarksModel->index(i, 0);

        BookmarkNode *node = bookmarksModel->node(parentIndex);

        if (node->title != tr("Start Page")) continue;

        QList<BookmarkNode *> nodes = node->children();

        foreach (BookmarkNode *node, nodes)
        {
            QModelIndex index = bookmarksModel->index(node);

            QIcon icon = bookmarksModel->data(index, Qt::DecorationRole).value<QIcon>();
            QString title = bookmarksModel->data(index).toString();
            QString url = bookmarksModel->data(index, BookmarksModel::UrlStringRole).toString();

            qDebug() << "url" << url;

            title = title.left(20);

            m_model->setItem(row, 0, new QStandardItem(icon, title));

            QModelIndex index0 = m_model->index(row, 0);

            m_model->setData(index0, url, Qt::ToolTipRole);

            row++;

            m_model->setRowCount(row);
        }

        // Virtual add tile

        QIcon icon = QIcon(QLatin1String(":addbookmark.png"));

        m_model->setItem(row, 0, new QStandardItem(icon, tr("New Page")));
        QModelIndex index0 = m_model->index(row, 0);

        m_model->setData(index0, tr("Add new page"), Qt::ToolTipRole);
        m_model->setData(index0, 1, Roles::VirtualRole);

        row++;

        m_model->setRowCount(row);
    }
}

void StartPageWidget::onCustomContextMenuRequested(const QPoint &pos)
{
    qDebug() << "onCustomContextMenuRequested" << pos;

    QModelIndex index = ui->listView->currentIndex();

    if ( ! index.isValid()) return;

    QMenu menu(this);

    QAction *openAction = menu.addAction(tr("Open"));
    connect (openAction, SIGNAL(triggered()), SLOT(openItem()));

    menu.exec (mapToGlobal(pos) );
}

void StartPageWidget::openItem()
{
    QModelIndex index = ui->listView->currentIndex();

    openItem(index);
}

void StartPageWidget::openItem(const QModelIndex &index)
{
    if ( ! index.isValid()) return;

    int virt = ui->listView->model()->data(index, Roles::VirtualRole).toInt();

    if (virt)
    {
        QString title = "New Page";
        QString url   = "www.example.com";
        QIcon icon;
        AddBookmarkDialog dialog(url, title, icon);
        dialog.setStartPageDefault();
        dialog.exec();
    }
    else
    {

        QString url = ui->listView->model()->data(index, Qt::ToolTipRole).toString();

        emit openUrl(url);
    }
}

///


TileDelegate::TileDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    m_tileSize.setWidth(240);
    m_tileSize.setHeight(200);
}

void TileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    //QStyledItemDelegate::paint(painter, option, index);

    // Widget itself
    //QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    //style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget); // CE_ItemViewItem

    int iconSize = m_tileSize.height() / 10; // 20

    if ( ! index.isValid()) return;

    QRect rect = option.rect;

    // Test
    //painter->setBrush(Qt::black);
    //painter->drawRect(rect);

    bool mouseOver  = option.state.testFlag(QStyle::State_MouseOver);
    //bool isSelected = option.state.testFlag(QStyle::State_HasFocus);

    QColor activeRectColor(102, 102, 255);
    QColor nonActiveRectColor(153, 153, 255);

    const QString title = index.data().toString();
    const QIcon icon    = index.data(Qt::DecorationRole).value<QIcon>();

    // Background

    painter->setBrush(Qt::black);
    painter->setPen(Qt::transparent);

    int offset = rect.height() / 20;
    QRect tileRect = QRect(rect.x() + offset, rect.y() + offset,
                           rect.width() - offset * 2, rect.height() - offset * 2);

    if (mouseOver)
    {
        // painter->setBrush(activeRectColor);

        QPen pen(activeRectColor);
        pen.setWidth(offset / 2);

        painter->setPen(pen);
        painter->setBrush(Qt::white);
    }
    else
    {
        painter->setBrush(Qt::white);
    }

    painter->drawRect(tileRect);

    // Icon

    QPoint iconPos(tileRect.x() + iconSize / 2,
               tileRect.y() + iconSize / 2);

    drawIcon(painter, icon, QSize(iconSize, iconSize), iconPos);

    // Label rect

    int labelOffset = tileRect.height() / 4;

    QRect labelRect = QRect(tileRect.x(), tileRect.y() + 3 * labelOffset,
                           tileRect.width(), labelOffset);

    painter->setBrush(nonActiveRectColor);

    if (mouseOver)
    {
        painter->setBrush(activeRectColor);
    }

    painter->drawRect(labelRect);

    // Label text

    painter->setBrush(Qt::black);
    painter->setPen(QColor(240,240,240));

    if (mouseOver)
    {
        //painter->setPen(QPen(QGuiApplication::palette().color(QPalette::Highlight), 3));
        painter->setPen(Qt::white);
    }

    QString text = option.fontMetrics.elidedText(title, option.textElideMode, (labelRect.width() - offset)); // 20

    painter->drawText(labelRect, Qt::AlignCenter, text);
}

QSize TileDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    //QSize size = QStyledItemDelegate::sizeHint(option, index);
    //size.setHeight(m_tileSize);
    //size.setWidth(m_tileSize);

    return m_tileSize;
}

void TileDelegate::drawIcon(QPainter *painter, const QIcon &icon, QSize iconSize, QPoint pos) const
{
    bool isEnabled = true;

    QPixmap pixmap = icon.pixmap(iconSize,
                                   isEnabled ? QIcon::Normal
                                               : QIcon::Disabled);

    QPixmap scaledPixmap = pixmap.scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    painter->drawPixmap(pos, scaledPixmap);
}

void StartPageWidget::on_settingsButton_clicked()
{
    BookmarksDialog *dialog = new BookmarksDialog(this);
    /*
    connect(dialog, SIGNAL(openUrl(QUrl)),
            this, SLOT(loadUrlInCurrentTab(QUrl)));
            */

    dialog->show();
}
