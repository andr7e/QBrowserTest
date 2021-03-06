/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "browsermainwindow.h"

#include "autosaver.h"
#include "bookmarks.h"
#include "browserapplication.h"
#include "chasewidget.h"
#include "downloadmanager.h"
#include "history.h"
#include "printtopdfdialog.h"
#include "settings.h"
#include "tabwidget.h"
#include "toolbarsearch.h"
#include "ui_passworddialog.h"
#include "webview.h"
#include "searchenginemanager.h"
#include "finddialog.h"

#include <QtCore/QSettings>

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QPlainTextEdit>
#include <QtPrintSupport/QPrintDialog>
#include <QtPrintSupport/QPrintPreviewDialog>
#include <QtPrintSupport/QPrinter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QInputDialog>

#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include <QtCore/QDebug>
#include <QSplitter>
#include <QDockWidget>

#include "utils.h"
#include "previewmanager.h"
#include "webengineurlrequestinterceptor.h"

#define BROWSER_NAME "E7 Browser"

#define USE_SPLITTER_BETWEEN_LINE
#define USE_CUSTOM_ICONS

template<typename Arg, typename R, typename C>
struct InvokeWrapper {
    R *receiver;
    void (C::*memberFun)(Arg);
    void operator()(Arg result) {
        (receiver->*memberFun)(result);
    }
};

template<typename Arg, typename R, typename C>
InvokeWrapper<Arg, R, C> invoke(R *receiver, void (C::*memberFun)(Arg))
{
    InvokeWrapper<Arg, R, C> wrapper = {receiver, memberFun};
    return wrapper;
}

const char *BrowserMainWindow::defaultHome = "http://qt.io/";

BrowserMainWindow::BrowserMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_tabWidget(new TabWidget(this))
    , m_autoSaver(new AutoSaver(this))
    , m_historyBack(0)
    , m_historyForward(0)
    , m_stop(0)
    , m_reload(0)
    , m_currentPrinter(nullptr)
    , m_findDialog(new FindDialog(this))
    , m_progressBar(new QProgressBar(this))
{
    isPrivated = false;

    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setAttribute(Qt::WA_DeleteOnClose, true);
    statusBar()->setSizeGripEnabled(true);
    setupMenu();
    setupToolBar();

    m_progressBar->setMaximumHeight(2);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(QStringLiteral("QProgressBar {border: 0px} QProgressBar::chunk {background-color: #00cc66}")); // da4453

    QWidget *centralWidget = new QWidget(this);
    BookmarksModel *bookmarksModel = BrowserApplication::bookmarksManager()->bookmarksModel();
    m_bookmarksToolbar = new BookmarksToolBar(bookmarksModel, this);
    connect(m_bookmarksToolbar, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(m_bookmarksToolbar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateBookmarksToolbarActionText(bool)));

    //

    SearchEngineManager::instance()->load();

    QString key = SearchEngineManager::getDefaultName();
    SearchEngine searchEngine = SearchEngineManager::instance()->get(key);
    //m_searchSwitchAction->setIcon(searchEngine.icon);
    m_searchEngineToolButton->setIcon(searchEngine.icon);
    m_toolbarSearch->updateSearchName(searchEngine.name);
    m_toolbarSearch->update();

    //

    connect(m_findDialog, SIGNAL(textChanged(QString)), SLOT(slotEditFind(QString)));
    connect(m_findDialog, SIGNAL(caseSensitivelyChanged()), SLOT(slotEditFindSensitively()));
    connect(m_findDialog, SIGNAL(findClosed()),         SLOT(slotEndFind()));
    connect(m_findDialog, SIGNAL(findNext()),           SLOT(slotEditFindNext()));
    connect(m_findDialog, SIGNAL(findPrev()),           SLOT(slotEditFindPrevious()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
#if defined(Q_OS_OSX)
    layout->addWidget(m_bookmarksToolbar);
    layout->addWidget(new QWidget); // <- OS X tab widget style bug
#else
    addToolBarBreak();
    addToolBar(m_bookmarksToolbar);
#endif

    dockBookmakrksWidget = new QDockWidget(tr("Bookmarks"), this);
    QTreeView *treeView = new QTreeView(dockBookmakrksWidget);
    treeView->setModel(bookmarksModel);
    //treeView->setExpanded(bookmarksModel->index(0, 0), true);
    treeView->expandAll();
    treeView->setAlternatingRowColors(true);
    QFontMetrics fm(font());
    int header = fm.width(QLatin1Char('m')) * 40;
    treeView->header()->resizeSection(0, header);
    treeView->header()->setStretchLastSection(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotOpenBookmark(QModelIndex)));
    dockBookmakrksWidget->setWidget(treeView);
    dockBookmakrksWidget->setVisible(false);
    addDockWidget(Qt::LeftDockWidgetArea, dockBookmakrksWidget);

    layout->addWidget(m_tabWidget);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_findDialog);

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    m_findDialog->setVisible(false);

    connect(m_tabWidget, SIGNAL(loadPage(QString)),
        this, SLOT(loadPage(QString)));
    connect(m_tabWidget, SIGNAL(setCurrentTitle(QString)),
        this, SLOT(slotUpdateWindowTitle(QString)));
    connect(m_tabWidget, SIGNAL(showStatusBarMessage(QString)),
            statusBar(), SLOT(showMessage(QString)));
    connect(m_tabWidget, SIGNAL(linkHovered(QString)),
            statusBar(), SLOT(showMessage(QString)));
    connect(m_tabWidget, SIGNAL(loadProgress(int)),
            this, SLOT(slotLoadProgress(int)));
    connect(m_tabWidget, SIGNAL(tabsChanged()),
            m_autoSaver, SLOT(changeOccurred()));
    connect(m_tabWidget, SIGNAL(tabsChanged()),
            this, SLOT(slotEndFind()));
    connect(m_tabWidget, SIGNAL(geometryChangeRequested(QRect)),
            this, SLOT(geometryChangeRequested(QRect)));
#if defined(QWEBENGINEPAGE_PRINTREQUESTED)
    connect(m_tabWidget, SIGNAL(printRequested(QWebEngineFrame*)),
            this, SLOT(printRequested(QWebEngineFrame*)));
#endif
    connect(m_tabWidget, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            menuBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            statusBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_navigationBar, SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            m_bookmarksToolbar, SLOT(setVisible(bool)));

    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            m_tabWidget, SLOT(newTab()));

    connect(m_tabWidget, SIGNAL(zoomChanged(int)),
            this, SLOT(setZoomActionText()));
    connect(m_tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(setZoomActionText()));

    slotUpdateWindowTitle();
    loadDefaultState();

    if (m_tabWidget->count() == 0) m_tabWidget->newTab();

    int size = m_tabWidget->lineEditStack()->sizeHint().height();
    m_navigationBar->setIconSize(QSize(size, size));
}

BrowserMainWindow::~BrowserMainWindow()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNeccessary();
}

void BrowserMainWindow::loadDefaultState()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = settings.value(QLatin1String("defaultState")).toByteArray();
    restoreState(data);
    settings.endGroup();
}

QSize BrowserMainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

void BrowserMainWindow::setPrivateWindow()
{
    isPrivated = true;

    m_toolbarSearch->setPrivateMode();
    m_tabWidget->setPrivateMode();
}

void BrowserMainWindow::save()
{
    if (isPrivated) return;

    BrowserApplication::instance()->saveSession();

    QSettings settings;

    settings.beginGroup(QLatin1String("tabs"));
    bool saveTabs = settings.value(QLatin1String("saveTabs"), false).toBool();
    settings.endGroup();

    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = saveState(saveTabs);
    settings.setValue(QLatin1String("defaultState"), data);
    settings.endGroup();

    BrowserApplication::blockingManager()->saveSettings();
}

static const qint32 BrowserMainWindowMagic = 0xba;

QByteArray BrowserMainWindow::saveState(bool withTabs) const
{
    int version = 2;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(BrowserMainWindowMagic);
    stream << qint32(version);

    stream << size();
    stream << !m_navigationBar->isHidden();
    stream << !m_bookmarksToolbar->isHidden();
    stream << !statusBar()->isHidden();
    if (withTabs)
        stream << tabWidget()->saveState();
    else
        stream << QByteArray();
    return data;
}

bool BrowserMainWindow::restoreState(const QByteArray &state)
{
    CDEBUG;

    int version = 2;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != BrowserMainWindowMagic || v != version)
        return false;

    QSize size;
    bool showToolbar;
    bool showBookmarksBar;
    bool showStatusbar;
    QByteArray tabState;

    stream >> size;
    stream >> showToolbar;
    stream >> showBookmarksBar;
    stream >> showStatusbar;
    stream >> tabState;

    resize(size);

    m_navigationBar->setVisible(showToolbar);
    updateToolbarActionText(showToolbar);

    m_bookmarksToolbar->setVisible(showBookmarksBar);
    updateBookmarksToolbarActionText(showBookmarksBar);

    statusBar()->setVisible(showStatusbar);
    updateStatusbarActionText(showStatusbar);

    bool loadMainWindow = BrowserApplication::instance()->mainWindows().count() == 0;

    if (loadMainWindow && !tabWidget()->restoreState(tabState))
        return false;

    return true;
}

void BrowserMainWindow::runScriptOnOpenViews(const QString &source)
{
    for (int i = 0; i < tabWidget()->count(); ++i)
    {
        WebView *webView = tabWidget()->webView(i);
        if (webView) webView->page()->runJavaScript(source);
    }
}

void BrowserMainWindow::setupMenu()
{
    new QShortcut(QKeySequence(Qt::Key_F6), this, SLOT(slotSwapFocus()));

    // File
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(tr("&New Window"), this, SLOT(slotFileNew()), QKeySequence::New);
    fileMenu->addAction(tr("&New Private Window"), this, SLOT(slotFileNewPrivate()));
    fileMenu->addAction(m_tabWidget->newTabAction());
    fileMenu->addAction(tr("&Open saved page..."), this, SLOT(slotFileOpenSavedPage()), QKeySequence::Open);
    fileMenu->addAction(tr("&Open File..."), this, SLOT(slotFileOpen()));
    fileMenu->addAction(tr("Open &Location..."), this,
                SLOT(slotSelectLineEdit()), QKeySequence(Qt::ControlModifier + Qt::Key_L));
    fileMenu->addSeparator();
    fileMenu->addAction(m_tabWidget->closeTabAction());
    fileMenu->addSeparator();
#if defined(QWEBENGINE_SAVE_AS_FILE)
    fileMenu->addAction(tr("&Save As..."), this,
                SLOT(slotFileSaveAs()), QKeySequence(QKeySequence::Save));
    fileMenu->addSeparator();
#endif
    BookmarksManager *bookmarksManager = BrowserApplication::bookmarksManager();
    fileMenu->addAction(tr("&Import Bookmarks..."), bookmarksManager, SLOT(importBookmarks()));
    fileMenu->addAction(tr("&Export Bookmarks..."), bookmarksManager, SLOT(exportBookmarks()));
    fileMenu->addSeparator();
#if defined(QWEBENGINEPAGE_PRINT)
    fileMenu->addAction(tr("P&rint Preview..."), this, SLOT(slotFilePrintPreview()));
#endif
    fileMenu->addAction(tr("&Print..."), this, SLOT(slotFilePrint()), QKeySequence::Print);
    fileMenu->addAction(tr("&Print to PDF..."), this, SLOT(slotFilePrintToPDF()));
    fileMenu->addSeparator();

    QAction *action = fileMenu->addAction(tr("Private &Browsing..."), this, SLOT(slotPrivateBrowsing()));
    action->setCheckable(true);
    action->setChecked(BrowserApplication::instance()->privateBrowsing());
    connect(BrowserApplication::instance(), SIGNAL(privateBrowsingChanged(bool)), action, SLOT(setChecked(bool)));
    fileMenu->addSeparator();

#if defined(Q_OS_OSX)
    fileMenu->addAction(tr("&Quit"), BrowserApplication::instance(), SLOT(quitBrowser()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#else
    fileMenu->addAction(tr("&Quit"), this, SLOT(close()), QKeySequence(Qt::CTRL | Qt::Key_Q));
#endif

    // Edit
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QAction *m_undo = editMenu->addAction(tr("&Undo"));
    m_undo->setShortcuts(QKeySequence::Undo);
    m_tabWidget->addWebAction(m_undo, QWebEnginePage::Undo);
    QAction *m_redo = editMenu->addAction(tr("&Redo"));
    m_redo->setShortcuts(QKeySequence::Redo);
    m_tabWidget->addWebAction(m_redo, QWebEnginePage::Redo);
    editMenu->addSeparator();
    QAction *m_cut = editMenu->addAction(tr("Cu&t"));
    m_cut->setShortcuts(QKeySequence::Cut);
    m_tabWidget->addWebAction(m_cut, QWebEnginePage::Cut);
    QAction *m_copy = editMenu->addAction(tr("&Copy"));
    m_copy->setShortcuts(QKeySequence::Copy);
    m_tabWidget->addWebAction(m_copy, QWebEnginePage::Copy);
    QAction *m_paste = editMenu->addAction(tr("&Paste"));
    m_paste->setShortcuts(QKeySequence::Paste);
    m_tabWidget->addWebAction(m_paste, QWebEnginePage::Paste);
    editMenu->addSeparator();

    QAction *m_find = editMenu->addAction(tr("&Find"));
    m_find->setShortcuts(QKeySequence::Find);
    connect(m_find, SIGNAL(triggered()), this, SLOT(slotStartFind()));

    QAction *m_findNext = editMenu->addAction(tr("&Find Next"));
    m_findNext->setShortcuts(QKeySequence::FindNext);
    connect(m_findNext, SIGNAL(triggered()), this, SLOT(slotEditFindNext()));

    QAction *m_findPrevious = editMenu->addAction(tr("&Find Previous"));
    m_findPrevious->setShortcuts(QKeySequence::FindPrevious);
    connect(m_findPrevious, SIGNAL(triggered()), this, SLOT(slotEditFindPrevious()));
    editMenu->addSeparator();

    editMenu->addAction(tr("&Preferences"), this, SLOT(slotPreferences()), tr("Ctrl+,"));

    // View
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    m_viewBookmarkBar = new QAction(this);
    updateBookmarksToolbarActionText(true);
    m_viewBookmarkBar->setShortcut(tr("Shift+Ctrl+B"));
    connect(m_viewBookmarkBar, SIGNAL(triggered()), this, SLOT(slotViewBookmarksBar()));
    viewMenu->addAction(m_viewBookmarkBar);

    m_viewToolbar = new QAction(this);
    updateToolbarActionText(true);
    m_viewToolbar->setShortcut(tr("Ctrl+|"));
    connect(m_viewToolbar, SIGNAL(triggered()), this, SLOT(slotViewToolbar()));
    viewMenu->addAction(m_viewToolbar);

    m_viewStatusbar = new QAction(this);
    updateStatusbarActionText(true);
    m_viewStatusbar->setShortcut(tr("Ctrl+/"));
    connect(m_viewStatusbar, SIGNAL(triggered()), this, SLOT(slotViewStatusbar()));
    viewMenu->addAction(m_viewStatusbar);

    viewMenu->addSeparator();

    m_stop = viewMenu->addAction(tr("&Stop"));
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Period));
    shortcuts.append(Qt::Key_Escape);
    m_stop->setShortcuts(shortcuts);
    m_tabWidget->addWebAction(m_stop, QWebEnginePage::Stop);

    m_reload = viewMenu->addAction(tr("Reload Page"));
    m_reload->setShortcuts(QKeySequence::Refresh);
    m_tabWidget->addWebAction(m_reload, QWebEnginePage::Reload);

    viewMenu->addAction(tr("Zoom &In"), this, SLOT(slotViewZoomIn()), QKeySequence(Qt::CTRL | Qt::Key_Plus));
    viewMenu->addAction(tr("Zoom &Out"), this, SLOT(slotViewZoomOut()), QKeySequence(Qt::CTRL | Qt::Key_Minus));
    viewMenu->addAction(tr("Reset &Zoom"), this, SLOT(slotViewResetZoom()), QKeySequence(Qt::CTRL | Qt::Key_0));

    viewMenu->addSeparator();
    QAction *m_pageSource = viewMenu->addAction(tr("Page S&ource"));
    m_pageSource->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_U));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    m_tabWidget->addWebAction(m_pageSource, QWebEnginePage::ViewSource);
#else
    connect(m_pageSource, SIGNAL(triggered(bool)), this, SLOT(slotViewPageSource()));
#endif

    QAction *a = viewMenu->addAction(tr("&Full Screen"), this, SLOT(slotViewFullScreen(bool)),  Qt::Key_F11);
    a->setCheckable(true);

    // History
    HistoryMenu *historyMenu = new HistoryMenu(this);
    connect(historyMenu, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    connect(historyMenu, SIGNAL(hovered(QString)), this,
            SLOT(slotUpdateStatusbar(QString)));
    historyMenu->setTitle(tr("Hi&story"));
    menuBar()->addMenu(historyMenu);
    QList<QAction*> historyActions;

    m_historyBack = new QAction(tr("Back"), this);
    m_tabWidget->addWebAction(m_historyBack, QWebEnginePage::Back);
    QList<QKeySequence> backShortcuts = QKeySequence::keyBindings(QKeySequence::Back);
    for (auto it = backShortcuts.begin(); it != backShortcuts.end();) {
        // Chromium already handles navigate on backspace when appropriate.
        if ((*it)[0] == Qt::Key_Backspace)
            it = backShortcuts.erase(it);
        else
            ++it;
    }
    // For some reason Qt doesn't bind the dedicated Back key to Back.
    backShortcuts.append(QKeySequence(Qt::Key_Back));
    m_historyBack->setShortcuts(backShortcuts);
    m_historyBack->setIconVisibleInMenu(false);
    historyActions.append(m_historyBack);

    m_historyForward = new QAction(tr("Forward"), this);
    m_tabWidget->addWebAction(m_historyForward, QWebEnginePage::Forward);
    QList<QKeySequence> fwdShortcuts = QKeySequence::keyBindings(QKeySequence::Forward);
    for (auto it = fwdShortcuts.begin(); it != fwdShortcuts.end();) {
        if (((*it)[0] & Qt::Key_unknown) == Qt::Key_Backspace)
            it = fwdShortcuts.erase(it);
        else
            ++it;
    }
    fwdShortcuts.append(QKeySequence(Qt::Key_Forward));
    m_historyForward->setShortcuts(fwdShortcuts);
    m_historyForward->setIconVisibleInMenu(false);
    historyActions.append(m_historyForward);

    QAction *m_historyHome = new QAction(tr("Home"), this);
    connect(m_historyHome, SIGNAL(triggered()), this, SLOT(slotHome()));
    m_historyHome->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    historyActions.append(m_historyHome);

#if defined(QWEBENGINEHISTORY_RESTORESESSION)
    m_restoreLastSession = new QAction(tr("Restore Last Session"), this);
    connect(m_restoreLastSession, SIGNAL(triggered()), BrowserApplication::instance(), SLOT(restoreLastSession()));
    m_restoreLastSession->setEnabled(BrowserApplication::instance()->canRestoreSession());
    historyActions.append(m_tabWidget->recentlyClosedTabsAction());
    historyActions.append(m_restoreLastSession);
#endif

    historyMenu->setInitialActions(historyActions);

    // Bookmarks
    BookmarksMenu *bookmarksMenu = new BookmarksMenu(this);
    connect(bookmarksMenu, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab2(QUrl)));
    connect(bookmarksMenu, SIGNAL(hovered(QString)),
            this, SLOT(slotUpdateStatusbar(QString)));
    bookmarksMenu->setTitle(tr("&Bookmarks"));
    menuBar()->addMenu(bookmarksMenu);

    QList<QAction*> bookmarksActions;

    QAction *showAllBookmarksAction = new QAction(tr("Show All Bookmarks"), this);
    connect(showAllBookmarksAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksDialog()));

    QAction *showBookmarksPanelAction = new QAction(tr("Show Bookmarks panel"), this);
    connect(showBookmarksPanelAction, SIGNAL(triggered()), this, SLOT(slotShowBookmarksPanel()));

    m_addBookmark = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("Add Bookmark..."), this);
    m_addBookmark->setIconVisibleInMenu(false);

    connect(m_addBookmark, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
    m_addBookmark->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    bookmarksActions << showAllBookmarksAction;
    bookmarksActions << showBookmarksPanelAction;
    bookmarksActions << m_addBookmark;
    bookmarksMenu->setInitialActions(bookmarksActions);

    // Window
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    connect(m_windowMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowWindowMenu()));
    slotAboutToShowWindowMenu();

    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(tr("Web &Search"), this, SLOT(slotWebSearch()), QKeySequence(tr("Ctrl+K", "Web Search")));
#if defined(QWEBENGINEINSPECTOR)
    a = toolsMenu->addAction(tr("Enable Web &Inspector"), this, SLOT(slotToggleInspector(bool)));
    a->setCheckable(true);
#endif

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
    helpMenu->addAction(tr("About"), this, SLOT(slotAboutApplication()));
}

void BrowserMainWindow::setupToolBar()
{
    m_navigationBar = addToolBar(tr("Navigation"));
    connect(m_navigationBar->toggleViewAction(), SIGNAL(toggled(bool)),
            this, SLOT(updateToolbarActionText(bool)));

#ifdef USE_CUSTOM_ICONS
    m_historyBack->setIcon(QIcon(QStringLiteral(":go-previous.png")));
    m_historyForward->setIcon(QIcon(QStringLiteral(":go-next.png")));

    m_reloadIcon = QIcon(QStringLiteral(":process-refresh.png"));
    m_stopIcon   = QIcon(QStringLiteral(":process-stop.png"));
#else
    m_historyBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_historyForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));

    m_reloadIcon = style()->standardIcon(QStyle::SP_BrowserReload);
    m_stopIcon = style()->standardIcon(QStyle::SP_BrowserStop);
#endif

    m_historyBackMenu = new QMenu(this);
    m_historyBack->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotOpenActionUrl(QAction*)));
    m_navigationBar->addAction(m_historyBack);

    m_historyForwardMenu = new QMenu(this);
    connect(m_historyForwardMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotAboutToShowForwardMenu()));
    connect(m_historyForwardMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(slotOpenActionUrl(QAction*)));
    m_historyForward->setMenu(m_historyForwardMenu);
    m_navigationBar->addAction(m_historyForward);

    m_stopReload = new QAction(this);
    m_stopReload->setIcon(m_reloadIcon);

    m_navigationBar->addAction(m_stopReload);

    //

    m_addBookmarkToolBar = new QAction(QIcon(QLatin1String(":addbookmark.png")), tr("Add Bookmark..."), this);
    connect(m_addBookmarkToolBar, SIGNAL(triggered()), this, SLOT(slotAddBookmark()));
    m_addBookmarkToolBar->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    m_bookmarkMenuToolBar = new QAction(QIcon(QLatin1String(":bookmark-menu.png")), tr("Show bookmarks panel"), this);
    connect(m_bookmarkMenuToolBar, SIGNAL(triggered()), this, SLOT(slotShowBookmarksPanel()));
    m_bookmarkMenuToolBar->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));

    m_blockingSwitchAction = new QAction(QIcon(QLatin1String(":blocking.png")), tr("Blocking mode"), this);
    connect(m_blockingSwitchAction, SIGNAL(triggered()), this, SLOT(slotSwitchBlocking()));

    m_zoomSwitchAction = new QAction("", this);
    //connect(m_zoomSwitchAction, SIGNAL(triggered()), this, SLOT(slotSwitchBlocking()));
    m_zoomSwitchAction->setVisible(false);

    //

#ifndef USE_SPLITTER_BETWEEN_LINE
    m_navigationBar->addWidget(m_tabWidget->lineEditStack());
#endif

    // Circle loading progress
    m_chaseWidget = new ChaseWidget(this);
#ifndef USE_SPLITTER_BETWEEN_LINE
    m_navigationBar->addWidget(m_chaseWidget);
#endif

    m_toolbarSearch = new ToolbarSearch(m_navigationBar);
    connect(m_toolbarSearch, SIGNAL(search(QUrl)), SLOT(loadUrl(QUrl)));

    m_searchEngineToolButton = new QToolButton(this);
    m_searchEngineToolButton->setToolTip(tr("Switch current search..."));
    connect(m_searchEngineToolButton, SIGNAL(clicked(bool)), this, SLOT(slotSwitchSearch()));

#ifndef USE_SPLITTER_BETWEEN_LINE
    m_navigationBar->addAction(m_addBookmarkToolBar);
    m_navigationBar->addWidget(m_toolbarSearch);
#endif

    //-----------

#ifdef USE_SPLITTER_BETWEEN_LINE

    QToolBar *toolBar = new QToolBar(this);
    toolBar->addAction(m_addBookmarkToolBar);
    toolBar->addAction(m_bookmarkMenuToolBar);
    toolBar->addSeparator();
    toolBar->addAction(m_blockingSwitchAction);
    toolBar->addAction(m_zoomSwitchAction);

    QHBoxLayout *leftSideLayout = new QHBoxLayout(this);
    leftSideLayout->setContentsMargins(0,0,0,0);

    QWidget *leftSideWidget = new QWidget(this);
    leftSideLayout->addWidget(m_tabWidget->lineEditStack(), 3);
    leftSideLayout->addWidget(m_chaseWidget);
    leftSideLayout->addWidget(toolBar);

    leftSideWidget->setLayout(leftSideLayout);

    //

    QHBoxLayout *rightSideLayout = new QHBoxLayout(this);
    rightSideLayout->setContentsMargins(0,0,0,0);

    QWidget *rightSideWidget = new QWidget(this);
    rightSideLayout->addWidget(m_toolbarSearch);
    rightSideLayout->addWidget(m_searchEngineToolButton);
    rightSideWidget->setLayout(rightSideLayout);

    //

    QSplitter *splitter = new QSplitter(this);
    splitter->addWidget(leftSideWidget);
    splitter->addWidget(rightSideWidget);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    m_navigationBar->addWidget(splitter);
#endif

    m_chaseWidget->setVisible(false);
}

void BrowserMainWindow::slotShowBookmarksDialog()
{
    BookmarksDialog *dialog = new BookmarksDialog(this);
    connect(dialog, SIGNAL(openUrl(QUrl)),
            m_tabWidget, SLOT(loadUrlInCurrentTab(QUrl)));
    dialog->show();
}

void BrowserMainWindow::slotShowBookmarksPanel()
{
    bool isVisible = dockBookmakrksWidget->isVisible();
    dockBookmakrksWidget->setVisible( ! isVisible);
}

void BrowserMainWindow::slotAddBookmark()
{
    WebView *webView = currentTab();

    if (webView)
    {
        QString url = webView->url().toString();
        QString title = webView->title();
        QIcon icon = webView->icon();
        AddBookmarkDialog dialog(url, title, icon);
        dialog.exec();

        // Test thumbnail

        QSize m_size(600,400);
        PreviewManager::create(webView, url, m_size);
    }
}

void BrowserMainWindow::slotViewToolbar()
{
    if (m_navigationBar->isVisible()) {
        updateToolbarActionText(false);
        m_navigationBar->close();
    } else {
        updateToolbarActionText(true);
        m_navigationBar->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::slotViewBookmarksBar()
{
    if (m_bookmarksToolbar->isVisible()) {
        updateBookmarksToolbarActionText(false);
        m_bookmarksToolbar->close();
    } else {
        updateBookmarksToolbarActionText(true);
        m_bookmarksToolbar->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::updateStatusbarActionText(bool visible)
{
    m_viewStatusbar->setText(!visible ? tr("Show Status Bar") : tr("Hide Status Bar"));
}

void BrowserMainWindow::handleFindTextResult(bool found)
{
    if (!found)
        slotUpdateStatusbar(tr("\"%1\" not found.").arg(m_lastSearch));
}

void BrowserMainWindow::updateToolbarActionText(bool visible)
{
    m_viewToolbar->setText(!visible ? tr("Show Toolbar") : tr("Hide Toolbar"));
}

void BrowserMainWindow::updateBookmarksToolbarActionText(bool visible)
{
    m_viewBookmarkBar->setText(!visible ? tr("Show Bookmarks bar") : tr("Hide Bookmarks bar"));
}

void BrowserMainWindow::slotSwitchSearch()
{
    QMenu *menu = new QMenu(this);

    // Search

    QStringList keys = SearchEngineManager::instance()->getList();

    qSort(keys);

    foreach (QString key, keys)
    {
        SearchEngine engine = SearchEngineManager::instance()->get(key);
        menu->addAction(engine.icon, engine.name)->setData(key);
    }

    menu->addSeparator();
    menu->addAction(tr("Open search settings..."))->setData("settings");

    connect(menu, SIGNAL(triggered(QAction*)), SLOT(slotCurrentSearchChanged(QAction*)));

    menu->exec(QCursor::pos());
}

void BrowserMainWindow::slotCurrentSearchChanged(QAction *action)
{
    QString key = action->data().toString();
    CDEBUG << key;

    if (key == "settings")
    {
        SettingsDialog *s = new SettingsDialog(this);
        s->setTabIndex(SettingsDialog::SEARCH);
        s->show();
    }
    else
    {
        SearchEngineManager::instance()->setCurrentName(key);

        SearchEngine searchEngine = SearchEngineManager::instance()->get(key);

        //m_searchSwitchAction->setIcon(searchEngine.icon);
        m_searchEngineToolButton->setIcon(searchEngine.icon);
        m_toolbarSearch->updateSearchName(searchEngine.name);
        m_toolbarSearch->update();
    }
}

void BrowserMainWindow::slotSwitchBlocking()
{
    QMenu *menu = new QMenu(this);

    // Search

    QList<int> keys;
    keys << BlockingManager::Off
         << BlockingManager::Aggressive
         << BlockingManager::Aggressive_NoJS
         << BlockingManager::Aggressive_NoImage;

    QStringList labels;
    labels  << tr("Off")
            << tr("Aggressive")
            << tr("Aggressive+NoJS")
            << tr("Aggressive+NoImage");


    QString hostname;
    WebView *webView = currentTab();
    if (webView)
    {
        hostname = webView->url().host();

        CDEBUG << hostname;
    }

    BlockingManager::Mode mode = BrowserApplication::blockingManager()->getMode(hostname);

    for (int i = 0 ; i < keys.size(); i++)
    {
        QAction *action = menu->addAction(labels.at(i));
        action->setData(keys.at(i));
        action->setCheckable(true);

        if (i == mode)
        {
            action->setChecked(true);
        }
    }

    //menu->addSeparator();
    //menu->addAction(tr("Open search settings..."))->setData("settings");

    connect(menu, SIGNAL(triggered(QAction*)), SLOT(slotCurrentBlockingChanged(QAction*)));

    menu->exec(QCursor::pos());
}

void BrowserMainWindow::slotCurrentBlockingChanged(QAction *action)
{
    BlockingManager::Mode mode = (BlockingManager::Mode)action->data().toInt();

    CDEBUG << mode;

    QString hostname;
    WebView *webView = currentTab();
    if (webView)
    {
        hostname = webView->url().host();

        CDEBUG << hostname;
    }

    if ( ! hostname.isEmpty())
        BrowserApplication::blockingManager()->setMode(hostname, mode);
}

void BrowserMainWindow::setZoomActionText()
{
    int zoom = getZoom();

    bool visible = ! (zoom == 100 || zoom == -1);
    m_zoomSwitchAction->setVisible(visible);

    QString zoomStr = getZoomAsString(zoom);
    m_zoomSwitchAction->setText(zoomStr);
}

void BrowserMainWindow::slotViewStatusbar()
{
    if (statusBar()->isVisible()) {
        updateStatusbarActionText(false);
        statusBar()->close();
    } else {
        updateStatusbarActionText(true);
        statusBar()->show();
    }
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::loadUrl(const QUrl &url)
{
    qDebug() << "loadUrl" << currentTab() << url.isValid();

    if ( ! url.isValid()) // if (!currentTab() || !url.isValid())
        return;

    if (!currentTab() || !url.isValid())
    {
        int index = m_tabWidget->currentIndex();
        m_tabWidget->newTab();
        m_tabWidget->closeStartPageTab(index);
    }

    m_tabWidget->currentLineEdit()->setText(QString::fromUtf8(url.toEncoded()));
    m_tabWidget->loadUrlInCurrentTab(url);
}

void BrowserMainWindow::slotDownloadManager()
{
    BrowserApplication::downloadManager()->show();
}

void BrowserMainWindow::slotSelectLineEdit()
{
    m_tabWidget->currentLineEdit()->selectAll();
    m_tabWidget->currentLineEdit()->setFocus();
}

void BrowserMainWindow::slotFileSaveAs()
{
    // not implemented yet.
}

void BrowserMainWindow::slotPreferences()
{
    SettingsDialog *s = new SettingsDialog(this);
    s->show();
}

void BrowserMainWindow::slotUpdateStatusbar(const QString &string)
{
    statusBar()->showMessage(string, 2000);
}

void BrowserMainWindow::slotUpdateWindowTitle(const QString &title)
{
    if (title.isEmpty()) {
        setWindowTitle(tr(BROWSER_NAME));
    } else {
#if defined(Q_OS_OSX)
        setWindowTitle(title);
#else
        bool privateBrowsing = BrowserApplication::instance()->privateBrowsing() || isPrivated;

        QString privateText;

        if (privateBrowsing)
        {
            privateText = QString(" - ") + tr("Private Browsing");
        }

        setWindowTitle(tr("%1 - %2%3", "Page title and Browser name").arg(title).arg(BROWSER_NAME).arg(privateText));
#endif
    }
}

void BrowserMainWindow::slotAboutApplication()
{
    QMessageBox::about(this, tr("About"), tr(
        "Version %1"
        "<p>E7 Browser<p>"
        "<p>It's based on Qt demo browser with Qt WebEngine.<p>"
        "<p>Qt WebEngine is based on the Chromium open source project "
        "developed at <a href=\"http://www.chromium.org/\">http://www.chromium.org/</a>."
        ).arg(QCoreApplication::applicationVersion()));
}

void BrowserMainWindow::slotFileNew()
{
    BrowserApplication::instance()->newMainWindow();
    BrowserApplication::instance()->mainWindow();
    //BrowserMainWindow *mw = BrowserApplication::instance()->mainWindow();
    //mw->slotHome();
}

void BrowserMainWindow::slotFileNewPrivate()
{
    BrowserApplication::instance()->newMainWindow();
    BrowserMainWindow *mw = BrowserApplication::instance()->mainWindow();
    mw->setPrivateWindow();
    BrowserApplication::instance()->setPrivateBrowsingInWindow(mw);
    mw->slotPrivateHome();
    //mw->tabWidget()->newTab();
}

void BrowserMainWindow::slotFileOpenSavedPage()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open Web Page"), QString(),
            tr("Web Resources (*.html *.htm *mhtml);;All files (*.*)"));

    if (file.isEmpty())
        return;

    loadPage(file);
}

void BrowserMainWindow::slotFileOpen()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open Web Resource"), QString(),
            tr("Web Resources (*.html *.htm *.svg *.png *.gif *.svgz);;All files (*.*)"));

    if (file.isEmpty())
        return;

    loadPage(file);
}

void BrowserMainWindow::slotFilePrintPreview()
{
    if (!currentTab())
        return;
    QPrintPreviewDialog *dialog = new QPrintPreviewDialog(this);
    connect(dialog, SIGNAL(paintRequested(QPrinter*)),
            currentTab(), SLOT(print(QPrinter*)));
    dialog->exec();
}

void BrowserMainWindow::slotFilePrint()
{
    if (!currentTab())
        return;
    printRequested(currentTab()->page());
}

void BrowserMainWindow::slotHandlePdfPrinted(const QByteArray& result)
{
    if (!result.size())
        return;

    QFile file(m_printerOutputFileName);

    m_printerOutputFileName.clear();
    if (!file.open(QFile::WriteOnly))
        return;

    file.write(result.data(), result.size());
    file.close();
}

void BrowserMainWindow::slotFilePrintToPDF()
{
    if (!currentTab() || !m_printerOutputFileName.isEmpty())
        return;

    QFileInfo info(QStringLiteral("printout.pdf"));
    PrintToPdfDialog *dialog = new PrintToPdfDialog(info.absoluteFilePath(), this);
    dialog->setWindowTitle(tr("Print to PDF"));
    if (dialog->exec() != QDialog::Accepted || dialog->filePath().isEmpty())
        return;

    m_printerOutputFileName = dialog->filePath();
    currentTab()->page()->printToPdf(invoke(this, &BrowserMainWindow::slotHandlePdfPrinted), dialog->pageLayout());
}

void BrowserMainWindow::slotHandlePagePrinted(bool result)
{
    Q_UNUSED(result);

    delete m_currentPrinter;
    m_currentPrinter = nullptr;
}


void BrowserMainWindow::printRequested(QWebEnginePage *page)
{
    if (m_currentPrinter)
        return;
    m_currentPrinter = new QPrinter();
    QScopedPointer<QPrintDialog> dialog(new QPrintDialog(m_currentPrinter, this));
    dialog->setWindowTitle(tr("Print Document"));
    if (dialog->exec() != QDialog::Accepted) {
        slotHandlePagePrinted(false);
        return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    page->print(m_currentPrinter, invoke(this, &BrowserMainWindow::slotHandlePagePrinted));
#else
    Q_UNUSED(page)
    qDebug() << "print not implemented yet.";
#endif
}

void BrowserMainWindow::slotPrivateBrowsing()
{
    if (!BrowserApplication::instance()->privateBrowsing()) {
        QString title = tr("Are you sure you want to turn on private browsing?");
        QString text = tr("<b>%1</b><br><br>"
            "This action will reload all open tabs.<br>"
            "When private browsing in turned on,"
            " webpages are not added to the history,"
            " items are automatically removed from the Downloads window," \
            " new cookies are not stored, current cookies can't be accessed," \
            " site icons wont be stored, session wont be saved, " \
            " and searches are not added to the pop-up menu in the Google search box." \
            "  Until you close the window, you can still click the Back and Forward buttons" \
            " to return to the webpages you have opened.").arg(title);

        QMessageBox::StandardButton button = QMessageBox::question(this, QString(), text,
                               QMessageBox::Ok | QMessageBox::Cancel,
                               QMessageBox::Ok);

        if (button == QMessageBox::Ok)
            BrowserApplication::instance()->setPrivateBrowsing(true);
    } else {
        // TODO: Also ask here
        BrowserApplication::instance()->setPrivateBrowsing(false);
    }
}

void BrowserMainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;

    settings.beginGroup(QLatin1String("tabs"));
    bool saveTabs = settings.value(QLatin1String("saveTabs"), false).toBool();
    settings.endGroup();

    if (m_tabWidget->count() > 1 && ! saveTabs) {
        int ret = QMessageBox::warning(this, QString(),
                           tr("Are you sure you want to close the window?"
                              "  There are %1 tabs open").arg(m_tabWidget->count()),
                           QMessageBox::Yes | QMessageBox::No,
                           QMessageBox::No);
        if (ret == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    BrowserApplication::closeDownloadManager();

    event->accept();
    deleteLater();
}

void BrowserMainWindow::slotStartFind()
{
    if (!currentTab())
        return;

    m_findDialog->setVisible(true);
    m_findDialog->setFindFocus();
    m_findDialog->setText("");
    m_findDialog->setText(m_lastSearch);
}

void BrowserMainWindow::slotEndFind()
{
    if (!currentTab())
        return;

    m_findDialog->setVisible(false);

    currentTab()->findText("", 0);
}

void BrowserMainWindow::slotEditFind(const QString &search)
{
    if (!currentTab())
        return;

    QWebEnginePage::FindFlags flags;

    bool caseSensitively = m_findDialog->useCaseSensitively();
    if (caseSensitively) flags |= QWebEnginePage::FindCaseSensitively;

    bool ok = true;

    if (ok && !search.isEmpty()) {
        m_lastSearch = search;
        currentTab()->findText(m_lastSearch, flags, invoke(this, &BrowserMainWindow::handleFindTextResult));
    }
}

void BrowserMainWindow::slotEditFindSensitively()
{
    if (!currentTab())
        return;

    QWebEnginePage::FindFlags flags;

    bool caseSensitively = m_findDialog->useCaseSensitively();
    if (caseSensitively) flags |= QWebEnginePage::FindCaseSensitively;

    // If text not changed, flags not apply. Use clear old text.
    currentTab()->findText("", 0);
    currentTab()->findText(m_lastSearch, flags, invoke(this, &BrowserMainWindow::handleFindTextResult));
}

void BrowserMainWindow::slotEditFindNext()
{
    if (!currentTab() && !m_lastSearch.isEmpty())
        return;

    QWebEnginePage::FindFlags flags;

    bool caseSensitively = m_findDialog->useCaseSensitively();
    if (caseSensitively) flags |= QWebEnginePage::FindCaseSensitively;

    currentTab()->findText(m_lastSearch, flags);
}

void BrowserMainWindow::slotEditFindPrevious()
{
    if (!currentTab() && !m_lastSearch.isEmpty())
        return;

    QWebEnginePage::FindFlags flags;
    flags |= QWebEnginePage::FindBackward;

    bool caseSensitively = m_findDialog->useCaseSensitively();
    if (caseSensitively) flags |= QWebEnginePage::FindCaseSensitively;

    currentTab()->findText(m_lastSearch, flags);
}

void BrowserMainWindow::slotViewZoomIn()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(currentTab()->zoomFactor() + 0.1);
}

void BrowserMainWindow::slotViewZoomOut()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(currentTab()->zoomFactor() - 0.1);
}

void BrowserMainWindow::slotViewResetZoom()
{
    if (!currentTab())
        return;
    currentTab()->setZoomFactor(1.0);
}

void BrowserMainWindow::slotViewFullScreen(bool makeFullScreen)
{
    if (makeFullScreen) {
        showFullScreen();
    } else {
        if (isMinimized())
            showMinimized();
        else if (isMaximized())
            showMaximized();
        else showNormal();
    }
}

void BrowserMainWindow::slotOpenBookmark(const QModelIndex &index)
{
    qDebug() << index;

    if ( ! index.isValid()) return;

    if (!index.parent().isValid())
        return;

    QUrl url = index.sibling(index.row(), 1).data(BookmarksModel::UrlRole).toUrl();

    loadUrl(url);
}

void BrowserMainWindow::slotHome()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    QString home = settings.value(QLatin1String("home"), QLatin1String(defaultHome)).toString();
    loadPage(home);
}

void BrowserMainWindow::slotPrivateHome()
{
    QString home = "home.private";
    loadPage(home);
}

void BrowserMainWindow::slotViewPageSource()
{
    if (!currentTab())
        return;

    QPlainTextEdit *view = new QPlainTextEdit;
    view->setWindowTitle(tr("Page Source of %1").arg(currentTab()->title()));
    view->setMinimumWidth(800);
    view->setMinimumHeight(600);
    view->setAttribute(Qt::WA_DeleteOnClose);
    view->show();

    currentTab()->page()->toHtml(invoke(view, &QPlainTextEdit::setPlainText));

    //m_tabWidget->newTab()->setHtml(html);
}

void BrowserMainWindow::slotWebSearch()
{
    m_toolbarSearch->lineEdit()->selectAll();
    m_toolbarSearch->lineEdit()->setFocus();
}

void BrowserMainWindow::slotToggleInspector(bool enable)
{
#if defined(QWEBENGINEINSPECTOR)
    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::DeveloperExtrasEnabled, enable);
    if (enable) {
        int result = QMessageBox::question(this, tr("Web Inspector"),
                                           tr("The web inspector will only work correctly for pages that were loaded after enabling.\n"
                                           "Do you want to reload all pages?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            m_tabWidget->reloadAllTabs();
        }
    }
#else
    Q_UNUSED(enable);
#endif
}

void BrowserMainWindow::slotSwapFocus()
{
    if (currentTab()->hasFocus())
        m_tabWidget->currentLineEdit()->setFocus();
    else
        currentTab()->setFocus();
}

void BrowserMainWindow::loadPage(const QString &page)
{
    QUrl url = QUrl::fromUserInput(page);

    qDebug() << "loadPage" << url.toString();

    //qDebug() << url.scheme(); // http, ftp
    //qDebug() << url.host();   // site.zone
    //qDebug() << url.port();
    //qDebug() << url.isValid();

    if ( ! url.isValid())
        url = SearchEngineManager::getUrl(page);

    loadUrl(url);
}

TabWidget *BrowserMainWindow::tabWidget() const
{
    return m_tabWidget;
}

WebView *BrowserMainWindow::currentTab() const
{
    return m_tabWidget->currentWebView();
}

void BrowserMainWindow::slotLoadProgress(int progress)
{
    if (progress < 100 && progress > 0) {
        m_chaseWidget->setAnimated(true);
        disconnect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        //if (m_stopIcon.isNull())
        //    m_stopIcon = style()->standardIcon(QStyle::SP_BrowserStop);
        m_stopReload->setIcon(m_stopIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setToolTip(tr("Stop loading the current page"));
    } else {
        m_chaseWidget->setAnimated(false);
        disconnect(m_stopReload, SIGNAL(triggered()), m_stop, SLOT(trigger()));
        m_stopReload->setIcon(m_reloadIcon);
        connect(m_stopReload, SIGNAL(triggered()), m_reload, SLOT(trigger()));
        m_stopReload->setToolTip(tr("Reload the current page"));
    }

    if (0 < progress && progress < 100) {
        m_progressBar->setValue(progress);
        m_progressBar->setVisible(true);
    } else {
        m_progressBar->setValue(0);
        m_progressBar->setVisible(false);
    }
}

void BrowserMainWindow::slotAboutToShowBackMenu()
{
    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    QWebEngineHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = history->backItems(historyCount).count() - 1; i >= 0; --i) {
        QWebEngineHistoryItem item = history->backItems(history->count()).at(i);
        QAction *action = new QAction(this);
        action->setData(-1*(historyCount-i-1));
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowForwardMenu()
{
    m_historyForwardMenu->clear();
    if (!currentTab())
        return;
    QWebEngineHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = 0; i < history->forwardItems(history->count()).count(); ++i) {
        QWebEngineHistoryItem item = history->forwardItems(historyCount).at(i);
        QAction *action = new QAction(this);
        action->setData(historyCount-i);
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyForwardMenu->addAction(action);
    }
}

void BrowserMainWindow::slotAboutToShowWindowMenu()
{
    m_windowMenu->clear();
    m_windowMenu->addAction(m_tabWidget->nextTabAction());
    m_windowMenu->addAction(m_tabWidget->previousTabAction());
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(tr("Downloads"), this, SLOT(slotDownloadManager()), QKeySequence(tr("Alt+Ctrl+L", "Download Manager")));
    m_windowMenu->addSeparator();

    QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
    for (int i = 0; i < windows.count(); ++i) {
        BrowserMainWindow *window = windows.at(i);
        QAction *action = m_windowMenu->addAction(window->windowTitle(), this, SLOT(slotShowWindow()));
        action->setData(i);
        action->setCheckable(true);
        if (window == this)
            action->setChecked(true);
    }
}

void BrowserMainWindow::slotShowWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QVariant v = action->data();
        if (v.canConvert<int>()) {
            int offset = qvariant_cast<int>(v);
            QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
            windows.at(offset)->activateWindow();
            windows.at(offset)->currentTab()->setFocus();
        }
    }
}

void BrowserMainWindow::slotOpenActionUrl(QAction *action)
{
    int offset = action->data().toInt();
    QWebEngineHistory *history = currentTab()->history();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first()); // back
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back()); // forward
}

void BrowserMainWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

int BrowserMainWindow::getZoom()
{
    WebView *webView = currentTab();

    if (webView)
    {
        return webView->zoomFactor() * 100;
    }

    return -1;
}

QString BrowserMainWindow::getZoomAsString(int zoom)
{
    if (zoom > 0)
    {
        return QString::number(zoom) + "%";
    }

    return QString();
}
