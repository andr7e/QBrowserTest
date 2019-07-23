#include "startpagewidget.h"
#include "ui_startpagewidget.h"

#include <QDebug>
#include <QPainter>

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
        QString url = ui->listView->model()->data(index, Qt::ToolTipRole).toString();

        emit openUrl(url);
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

    if ( ! index.isValid()) return;

    QString url = ui->listView->model()->data(index, Qt::ToolTipRole).toString();

    emit openUrl(url);
}

///


TileDelegate::TileDelegate(QObject *parent) : QStyledItemDelegate(parent)
{

}

void TileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    //QStyledItemDelegate::paint(painter, option, index);

    // draw correct background
    //QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    //style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget); // CE_ItemViewItem


    // /*
    if ( ! index.isValid()) return;

    bool mouseOver  = option.state.testFlag(QStyle::State_MouseOver);
    bool isSelected = option.state.testFlag(QStyle::State_HasFocus);

    QColor selectedRectColor(102, 102, 255);

    const QString title = index.data().toString();
    const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    painter->setBrush(Qt::black);
    painter->setPen(Qt::gray);

    QRect rect = option.rect;

    if (mouseOver)
    {
        painter->setBrush(selectedRectColor);
    }
    else
    {
        painter->setBrush(Qt::white);
    }

    painter->drawRect(rect);


    if (isSelected)
    {
        QRect border(rect.x() + 1, rect.y() + 1, rect.width() - 2, rect.height() - 2);

        painter->setPen(Qt::black);
        painter->setBrush(Qt::transparent);
        painter->drawRect(border);
    }

    painter->setBrush(Qt::black);
    painter->setPen(Qt::gray);

    drawIcon(painter, icon, QPoint(rect.x() + 2, rect.y() + 2));

    //painter->drawText(rect.x() + 5, rect.y() + 50, user);
    //painter->drawText(rect.x() +  5, rect.y() +  70, address);

    if (mouseOver)
    {
        //painter->setPen(QPen(QGuiApplication::palette().color(QPalette::Highlight), 3));
        painter->setPen(Qt::white);
    }

    painter->drawText(rect, Qt::AlignCenter, option.fontMetrics.elidedText(title, option.textElideMode, (rect.width() - 20)));
}

QSize TileDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(150);
    size.setWidth(150);
    return size;
}

void TileDelegate::drawIcon(QPainter *painter, const QIcon &icon, QPoint pos) const
{
    bool isEnabled = true;

    QPixmap pixmap = icon.pixmap(QSize(22, 22),
                                   isEnabled ? QIcon::Normal
                                               : QIcon::Disabled);

    painter->drawPixmap(pos, pixmap);
}
