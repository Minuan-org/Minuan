#include "browser_window.hpp"

#include "dom_processor.hpp"
#include "html6_engine.hpp"
#include "navigation_utils.hpp"
#include "sunay/rendering_pipeline.hpp"
#include "utils.hpp"

#include <QAction>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QProgressBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QWebEngineDownloadRequest>
#else
#include <QWebEngineDownloadItem>
#endif

#include <functional>
#include <utility>

namespace {

class SafeWebPage final : public QWebEnginePage {
public:
    using BlockedHandler = std::function<void(const QUrl&)>;
    using InternalHandler = std::function<void(const QUrl&)>;
    using NewWindowHandler =
        std::function<QWebEnginePage*(QWebEnginePage::WebWindowType)>;

    SafeWebPage(const QSet<QString>& allowedHosts,
                BlockedHandler blockedHandler,
                InternalHandler internalHandler,
                NewWindowHandler newWindowHandler,
                QObject* parent = nullptr)
        : QWebEnginePage(parent),
          allowedHosts_(allowedHosts),
          blockedHandler_(std::move(blockedHandler)),
          internalHandler_(std::move(internalHandler)),
          newWindowHandler_(std::move(newWindowHandler)) {}

protected:
    QWebEnginePage* createWindow(QWebEnginePage::WebWindowType type) override {
        return newWindowHandler_ ? newWindowHandler_(type) : nullptr;
    }

    bool acceptNavigationRequest(const QUrl& url,
                                 QWebEnginePage::NavigationType type,
                                 bool isMainFrame) override {
        if (url.scheme().compare(QStringLiteral("minuan"),
                                 Qt::CaseInsensitive) == 0) {
            if (isMainFrame &&
                type == QWebEnginePage::NavigationTypeLinkClicked &&
                internalHandler_) {
                internalHandler_(url);
            }
            return false;
        }

        if (url.scheme() == QStringLiteral("about") &&
            url.toString() == QStringLiteral("about:blank")) {
            return true;
        }

        if (NavigationUtils::isAllowedHttpsUrl(url, allowedHosts_)) {
            return QWebEnginePage::acceptNavigationRequest(
                url, type, isMainFrame);
        }

        if (blockedHandler_) blockedHandler_(url);
        return false;
    }

private:
    QSet<QString> allowedHosts_;
    BlockedHandler blockedHandler_;
    InternalHandler internalHandler_;
    NewWindowHandler newWindowHandler_;
};

} // namespace

BrowserWindow::BrowserWindow() {
    setWindowTitle(QStringLiteral("Minuan"));
    resize(1280, 820);

    const bool sitesLoaded = loadWebsites();
    buildUi();
    connect(QWebEngineProfile::defaultProfile(),
            &QWebEngineProfile::downloadRequested,
            this, [this](auto* download) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QString suggestedName = download->suggestedFileName().isEmpty()
                    ? QStringLiteral("download")
                    : download->suggestedFileName();
                const QString initialPath = QDir(
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
                    .filePath(suggestedName);
                const QString filePath = QFileDialog::getSaveFileName(
                    this, tr("Save download"), initialPath);
                if (filePath.isEmpty()) {
                    download->cancel();
                    statusLabel_->setText(tr("Download cancelled."));
                    return;
                }

                const QFileInfo destination(filePath);
                download->setDownloadDirectory(destination.absolutePath());
                download->setDownloadFileName(destination.fileName());
                download->accept();
                statusLabel_->setText(tr("Downloading %1").arg(destination.fileName()));
                connect(download, &QWebEngineDownloadRequest::stateChanged,
                        this, [this, download](QWebEngineDownloadRequest::DownloadState state) {
                            if (state == QWebEngineDownloadRequest::DownloadCompleted) {
                                statusLabel_->setText(
                                    tr("Download saved: %1").arg(download->downloadFileName()));
                            } else if (state == QWebEngineDownloadRequest::DownloadInterrupted) {
                                statusLabel_->setText(tr("The download was interrupted."));
                            } else if (state == QWebEngineDownloadRequest::DownloadCancelled) {
                                statusLabel_->setText(tr("Download cancelled."));
                            }
                        });
#else
                const QString suggestedName = QFileInfo(download->path()).fileName().isEmpty()
                    ? QStringLiteral("download")
                    : QFileInfo(download->path()).fileName();
                const QString initialPath = QDir(
                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
                    .filePath(suggestedName);
                const QString filePath = QFileDialog::getSaveFileName(
                    this, tr("Save download"), initialPath);
                if (filePath.isEmpty()) {
                    download->cancel();
                    statusLabel_->setText(tr("Download cancelled."));
                    return;
                }

                download->setPath(filePath);
                download->accept();
                statusLabel_->setText(tr("Downloading %1").arg(QFileInfo(filePath).fileName()));
                connect(download, &QWebEngineDownloadItem::finished, this,
                        [this, filePath]() {
                            statusLabel_->setText(
                                tr("Download finished: %1").arg(QFileInfo(filePath).fileName()));
                        });
#endif
            });
    createTab(QUrl(QStringLiteral("about:blank")));

    if (!sitesLoaded) {
        statusBar()->showMessage(
            tr("No allowed websites were loaded. Add HTTPS sites to browser/websites.json."),
            10000);
    }
}

void BrowserWindow::buildUi() {
    setStyleSheet(QString::fromUtf8(R"STYLE(
QMainWindow {
    background: #0b1220;
    color: #e5edf8;
}
QToolBar {
    background: #14233b;
    border: none;
    border-bottom: 1px solid #263c5a;
    spacing: 7px;
    padding: 9px 12px;
}
QToolBar::separator {
    background: #38506f;
    width: 1px;
    margin: 5px 4px;
}
QToolButton {
    color: #e7efff;
    background: #203451;
    border: 1px solid #304a6d;
    border-radius: 8px;
    padding: 7px 11px;
    font-weight: 600;
}
QToolButton:hover {
    background: #2d4b70;
    border-color: #55c5ff;
}
QToolButton:pressed {
    background: #087e9b;
}
QToolButton:disabled {
    color: #71839b;
    background: #19283e;
    border-color: #243650;
}
QLineEdit {
    background: #f8fbff;
    color: #182438;
    border: 2px solid #435b7e;
    border-radius: 10px;
    padding: 8px 12px;
    min-height: 24px;
    selection-background-color: #0ea5e9;
}
QLineEdit:focus {
    border: 2px solid #38bdf8;
}
QTabWidget::pane {
    background: #0b1220;
    border: 1px solid #243a58;
    top: -1px;
}
QTabBar::tab {
    background: #172842;
    color: #bac9df;
    border: 1px solid #253b59;
    border-bottom: none;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    padding: 9px 16px;
    margin-right: 3px;
    min-width: 90px;
}
QTabBar::tab:selected {
    background: #0b6f91;
    color: #ffffff;
    border-color: #38bdf8;
}
QTabBar::tab:hover:!selected {
    background: #233d5e;
    color: #ffffff;
}
QStatusBar {
    background: #14233b;
    color: #c5d3e6;
    border-top: 1px solid #263c5a;
}
QLabel {
    color: #c5d3e6;
}
QProgressBar {
    background: #1c2e49;
    border: 1px solid #36516f;
    border-radius: 5px;
    height: 9px;
    text-align: center;
    color: #dbeafe;
}
QProgressBar::chunk {
    background: #38bdf8;
    border-radius: 4px;
}
)STYLE"));

    auto* navigation = addToolBar(tr("Navigation"));
    navigation->setMovable(false);

    backAction_ = navigation->addAction(tr("Back"));
    forwardAction_ = navigation->addAction(tr("Forward"));
    reloadAction_ = navigation->addAction(tr("Reload"));
    stopAction_ = navigation->addAction(tr("Stop"));
    QAction* homeAction = navigation->addAction(tr("Home"));
    navigation->addSeparator();

    addressBar_ = new QLineEdit(navigation);
    addressBar_->setClearButtonEnabled(true);
    addressBar_->setPlaceholderText(tr("Search Google or enter a website address"));
    addressBar_->setMinimumWidth(420);
    navigation->addWidget(addressBar_);

    QAction* goAction = navigation->addAction(tr("Go"));
    navigation->addSeparator();
    QAction* newTabAction = navigation->addAction(tr("New tab"));
    QAction* openFileAction = navigation->addAction(tr("Open HTML6"));
    QAction* bookmarkAction = navigation->addAction(tr("Bookmark"));

    bookmarksMenu_ = menuBar()->addMenu(tr("Bookmarks"));
    connect(bookmarksMenu_, &QMenu::aboutToShow,
            this, &BrowserWindow::rebuildBookmarksMenu);

    auto* privacyMenu = menuBar()->addMenu(tr("Privacy"));
    javaScriptAction_ = privacyMenu->addAction(tr("Allow JavaScript"));
    javaScriptAction_->setCheckable(true);
    javaScriptAction_->setChecked(true);
    imagesAction_ = privacyMenu->addAction(tr("Load images"));
    imagesAction_->setCheckable(true);
    imagesAction_->setChecked(true);

    backAction_->setShortcut(QKeySequence::Back);
    forwardAction_->setShortcut(QKeySequence::Forward);
    reloadAction_->setShortcut(QKeySequence::Refresh);
    stopAction_->setShortcut(QKeySequence(Qt::Key_Escape));
    stopAction_->setEnabled(false);
    homeAction->setShortcut(QKeySequence(QStringLiteral("Alt+Home")));
    newTabAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    openFileAction->setShortcut(QKeySequence::Open);
    bookmarkAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    bookmarkAction->setShortcutContext(Qt::WindowShortcut);
    addAction(bookmarkAction);

    auto* focusAddressAction = new QAction(this);
    focusAddressAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    focusAddressAction->setShortcutContext(Qt::WindowShortcut);
    addAction(focusAddressAction);
    connect(focusAddressAction, &QAction::triggered, this, [this]() {
        addressBar_->setFocus();
        addressBar_->selectAll();
    });

    auto* closeCurrentTabAction = new QAction(this);
    closeCurrentTabAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    closeCurrentTabAction->setShortcutContext(Qt::WindowShortcut);
    addAction(closeCurrentTabAction);
    connect(closeCurrentTabAction, &QAction::triggered, this, [this]() {
        closeTab(tabs_->currentIndex());
    });

    connect(bookmarkAction, &QAction::triggered,
            this, &BrowserWindow::addBookmark);
    connect(javaScriptAction_, &QAction::toggled, this, [this](bool enabled) {
        if (auto* tab = currentTab()) {
            tab->javaScriptEnabled = enabled;
            Sunay::RenderingOptions options;
            options.javaScriptEnabled = tab->javaScriptEnabled;
            options.imagesEnabled = tab->imagesEnabled;
            options.localContentCanAccessRemoteUrls = false;
            tab->renderer->setOptions(options);
            statusLabel_->setText(enabled
                ? tr("JavaScript is enabled for this tab.")
                : tr("JavaScript is disabled for this tab."));
        }
    });
    connect(imagesAction_, &QAction::toggled, this, [this](bool enabled) {
        if (auto* tab = currentTab()) {
            tab->imagesEnabled = enabled;
            Sunay::RenderingOptions options;
            options.javaScriptEnabled = tab->javaScriptEnabled;
            options.imagesEnabled = tab->imagesEnabled;
            options.localContentCanAccessRemoteUrls = false;
            tab->renderer->setOptions(options);
            statusLabel_->setText(enabled
                ? tr("Images are enabled for this tab.")
                : tr("Images are disabled for this tab."));
        }
    });

    backAction_->setEnabled(false);
    forwardAction_->setEnabled(false);

    connect(backAction_, &QAction::triggered, this, [this]() {
        if (auto* tab = currentTab()) tab->view->back();
    });
    connect(forwardAction_, &QAction::triggered, this, [this]() {
        if (auto* tab = currentTab()) tab->view->forward();
    });
    connect(reloadAction_, &QAction::triggered, this, [this]() {
        if (auto* tab = currentTab()) tab->view->reload();
    });
    connect(stopAction_, &QAction::triggered, this, [this]() {
        if (auto* tab = currentTab()) tab->renderer->stop();
    });
    connect(homeAction, &QAction::triggered, this, [this]() {
        showHomePage(currentTab());
    });
    connect(goAction, &QAction::triggered, this, &BrowserWindow::navigateAddress);
    connect(addressBar_, &QLineEdit::returnPressed,
            this, &BrowserWindow::navigateAddress);
    connect(newTabAction, &QAction::triggered, this, [this]() {
        createTab(QUrl(QStringLiteral("about:blank")));
    });
    connect(openFileAction, &QAction::triggered,
            this, &BrowserWindow::openLocalHtml6);

    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(true);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    setCentralWidget(tabs_);

    connect(tabs_, &QTabWidget::currentChanged,
            this, [this](int) { updateCurrentTab(); });
    connect(tabs_, &QTabWidget::tabCloseRequested,
            this, &BrowserWindow::closeTab);

    statusLabel_ = new QLabel(tr("Ready"), this);
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setMaximumWidth(150);
    progressBar_->hide();

    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(progressBar_);
}

bool BrowserWindow::loadWebsites() {
    QString configPath = QCoreApplication::applicationDirPath()
                             + QStringLiteral("/websites.json");
    if (!QFile::exists(configPath)) {
        configPath = QCoreApplication::applicationDirPath()
                     + QStringLiteral("/browser/websites.json");
    }

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        Utils::logWarning("Could not open websites.json at " +
                          configPath.toStdString());
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        Utils::logError("websites.json must contain a JSON array");
        return false;
    }

    for (const QJsonValue& value : document.array()) {
        QString urlText;
        if (value.isObject()) {
            urlText = value.toObject().value(QStringLiteral("url")).toString();
        } else if (value.isString()) {
            urlText = value.toString();
        }

        const QUrl url = QUrl::fromUserInput(urlText);
        if (url.scheme() != QStringLiteral("https") || url.host().isEmpty()) {
            continue;
        }

        allowedHosts_.insert(NavigationUtils::cleanHost(url.host()));
    }

    return !allowedHosts_.isEmpty();
}

bool BrowserWindow::isAllowedUrl(const QUrl& url) const {
    return NavigationUtils::isAllowedHttpsUrl(url, allowedHosts_);
}

void BrowserWindow::createTab(const QUrl& initialUrl) {
    auto* tabWidget = new QWidget(tabs_);
    auto* layout = new QVBoxLayout(tabWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* view = new QWebEngineView(tabWidget);
    auto* page = new SafeWebPage(
        allowedHosts_,
        [this](const QUrl& blockedUrl) {
            statusLabel_->setText(
                tr("Blocked navigation outside the allowed-site list: %1")
                    .arg(blockedUrl.toDisplayString()));
        },
        [this](const QUrl& internalUrl) {
            const QString action = internalUrl.host().toLower();
            if (action == QStringLiteral("home")) {
                showHomePage(currentTab());
            } else if (action == QStringLiteral("open-html6")) {
                openLocalHtml6();
            } else {
                statusLabel_->setText(tr("That Minuan action is not available."));
            }
        },
        [this](QWebEnginePage::WebWindowType) -> QWebEnginePage* {
            createTab(QUrl(QStringLiteral("about:blank")));
            auto* newTab = currentTab();
            return newTab ? newTab->view->page() : nullptr;
        },
        view);
    view->setPage(page);
    layout->addWidget(view);

    auto* renderer = new Sunay::RenderingPipeline(view, tabWidget);
    Sunay::RenderingOptions options;
    options.javaScriptEnabled = true;
    options.imagesEnabled = true;
    options.localContentCanAccessRemoteUrls = false;
    renderer->setOptions(options);

    tabRuntimes_.insert(tabWidget, TabRuntime{view, renderer, true, true, false, 0});

    connect(page, &QWebEnginePage::windowCloseRequested, this, [this, tabWidget]() {
        const int index = tabs_->indexOf(tabWidget);
        if (index >= 0) closeTab(index);
    });

    connect(renderer, &Sunay::RenderingPipeline::renderingStarted,
            this, [this, tabWidget]() {
                auto it = tabRuntimes_.find(tabWidget);
                if (it != tabRuntimes_.end()) {
                    it->isLoading = true;
                    it->loadProgress = 0;
                }
                if (tabs_->currentWidget() == tabWidget) {
                    statusLabel_->setText(tr("Loading page..."));
                    progressBar_->setValue(0);
                    progressBar_->show();
                    stopAction_->setEnabled(true);
                }
            });

    connect(renderer, &Sunay::RenderingPipeline::loadProgressChanged,
            this, [this, tabWidget](int progress) {
                auto it = tabRuntimes_.find(tabWidget);
                if (it != tabRuntimes_.end()) {
                    it->loadProgress = progress;
                }
                if (tabs_->currentWidget() == tabWidget) {
                    progressBar_->setValue(progress);
                    progressBar_->setVisible(progress < 100);
                }
            });

    connect(renderer, &Sunay::RenderingPipeline::renderingFinished,
            this, [this, tabWidget](bool success) {
                auto it = tabRuntimes_.find(tabWidget);
                if (it != tabRuntimes_.end()) {
                    it->isLoading = false;
                    it->loadProgress = 100;
                }
                if (tabs_->currentWidget() == tabWidget) {
                    progressBar_->hide();
                    stopAction_->setEnabled(false);
                    statusLabel_->setText(success ? tr("Ready")
                                                  : tr("Page failed to load"));
                    updateNavigationActions();
                }
            });

    connect(view, &QWebEngineView::titleChanged,
            this, [this, tabWidget](const QString& title) {
                const int index = tabs_->indexOf(tabWidget);
                if (index >= 0) {
                    tabs_->setTabText(index, title.isEmpty()
                                                  ? tr("New tab")
                                                  : title.left(32));
                }
                if (tabs_->currentWidget() == tabWidget) {
                    setWindowTitle(title.isEmpty()
                                       ? QStringLiteral("Minuan")
                                       : title + QStringLiteral(" — Minuan"));
                }
            });

    connect(view, &QWebEngineView::urlChanged,
            this, [this, tabWidget](const QUrl& url) {
                if (tabs_->currentWidget() == tabWidget) {
                    addressBar_->setText(
                        url.toString() == QStringLiteral("about:blank")
                            ? QStringLiteral("simverse://home")
                            : url.toDisplayString());
                    updateNavigationActions();
                }
            });

    const int index = tabs_->addTab(tabWidget, tr("New tab"));
    tabs_->setCurrentIndex(index);

    if (initialUrl.isValid() &&
        initialUrl != QUrl(QStringLiteral("about:blank"))) {
        if (isAllowedUrl(initialUrl)) {
            renderer->navigate(initialUrl);
        } else {
            statusLabel_->setText(tr("The configured start URL is not allowed."));
            showHomePage(currentTab());
        }
    } else {
        showHomePage(currentTab());
    }

    updateCurrentTab();
}

void BrowserWindow::closeTab(int index) {
    QWidget* tabWidget = tabs_->widget(index);
    if (!tabWidget) return;

    tabRuntimes_.remove(tabWidget);
    tabs_->removeTab(index);
    tabWidget->deleteLater();

    if (tabs_->count() == 0) {
        createTab(QUrl(QStringLiteral("about:blank")));
    }
    updateCurrentTab();
}

BrowserWindow::TabRuntime* BrowserWindow::currentTab() {
    if (!tabs_ || !tabs_->currentWidget()) return nullptr;
    auto it = tabRuntimes_.find(tabs_->currentWidget());
    return it == tabRuntimes_.end() ? nullptr : &it.value();
}

void BrowserWindow::navigateAddress() {
    if (!addressBar_) return;

    const QString enteredText = addressBar_->text().trimmed();
    if (enteredText.isEmpty()) return;

    if (enteredText.compare(QStringLiteral("simverse://home"),
                            Qt::CaseInsensitive) == 0) {
        showHomePage(currentTab());
        return;
    }
    if (enteredText.compare(QStringLiteral("minuan://open-html6"),
                            Qt::CaseInsensitive) == 0) {
        openLocalHtml6();
        return;
    }

    const QUrl url = NavigationUtils::resolveAddressInput(enteredText);
    if (!url.isValid() || url.isEmpty()) {
        statusLabel_->setText(tr("Enter a website address or a search."));
        return;
    }
    if (url.scheme() == QStringLiteral("http")) {
        statusLabel_->setText(tr("Only HTTPS sites are permitted."));
        return;
    }

    if (!isAllowedUrl(url)) {
        statusLabel_->setText(
            tr("That address is not in Minuan's allowed-site list."));
        return;
    }

    if (auto* tab = currentTab()) tab->renderer->navigate(url);
}

void BrowserWindow::showHomePage(TabRuntime* tab) {
    if (!tab || !tab->renderer) return;

    QFile file(QCoreApplication::applicationDirPath()
               + QStringLiteral("/home.html"));
    if (!file.open(QIODevice::ReadOnly)) {
        statusLabel_->setText(
            tr("The SimVerse home page could not be opened. Reinstall Minuan."));
        return;
    }

    const QByteArray page = file.readAll();
    if (page.isEmpty()) {
        statusLabel_->setText(tr("The SimVerse home page is empty."));
        return;
    }

    tab->renderer->renderHtml(QString::fromUtf8(page), QUrl(QStringLiteral("about:blank")));
    addressBar_->setText(QStringLiteral("simverse://home"));
    statusLabel_->setText(tr("Welcome to SimVerse."));
}

void BrowserWindow::openLocalHtml6() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open an HTML6 document"),
        QString(),
        tr("HTML6 documents (*.html6 *.html *.htm);;All files (*)"));
    if (!path.isEmpty()) loadHtml6File(path);
}

void BrowserWindow::loadHtml6File(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        statusLabel_->setText(tr("Could not open the selected document."));
        return;
    }

    const QByteArray source = file.readAll();
    if (source.isEmpty()) {
        statusLabel_->setText(tr("The selected document is empty."));
        return;
    }

    Minuan::HTML6::HTML6Engine engine;
    if (!engine.parseDocument(source.toStdString())) {
        statusLabel_->setText(tr("The HTML6 document could not be parsed."));
        return;
    }

    Minuan::HTML6::DOMProcessor processor;
    const std::string html = processor.generateHTML(engine.getAST());
    if (html.empty()) {
        statusLabel_->setText(tr("HTML6 conversion produced no HTML."));
        return;
    }

    auto* tab = currentTab();
    if (!tab) return;

    QString baseDirectory = QDir(QFileInfo(path).absolutePath()).absolutePath();
    if (!baseDirectory.endsWith(QDir::separator())) {
        baseDirectory += QDir::separator();
    }
    const QUrl baseUrl = QUrl::fromLocalFile(baseDirectory);
    tab->renderer->renderHtml(QString::fromUtf8(
                                  html.data(), static_cast<int>(html.size())),
                              baseUrl);

    const int index = tabs_->currentIndex();
    if (index >= 0) tabs_->setTabText(index, QFileInfo(path).fileName().left(32));
    addressBar_->setText(QUrl::fromLocalFile(path).toDisplayString());
    statusLabel_->setText(tr("HTML6 converted and sent to Sunay."));
}

void BrowserWindow::updateCurrentTab() {
    auto* tab = currentTab();
    if (!tab) return;

    const QUrl url = tab->view->url();
    addressBar_->setText(
        url.toString() == QStringLiteral("about:blank")
            ? QStringLiteral("simverse://home")
            : url.toDisplayString());
    const QString title = tab->view->title();
    setWindowTitle(title.isEmpty()
                       ? QStringLiteral("Minuan")
                       : title + QStringLiteral(" — Minuan"));
    {
        const QSignalBlocker jsBlocker(javaScriptAction_);
        javaScriptAction_->setChecked(tab->javaScriptEnabled);
    }
    {
        const QSignalBlocker imagesBlocker(imagesAction_);
        imagesAction_->setChecked(tab->imagesEnabled);
    }
    progressBar_->setValue(tab->loadProgress);
    progressBar_->setVisible(tab->isLoading);
    stopAction_->setEnabled(tab->isLoading);
    if (!tab->isLoading) statusLabel_->setText(tr("Ready"));
    updateNavigationActions();
}

void BrowserWindow::updateNavigationActions() {
    auto* tab = currentTab();
    if (!tab) {
        backAction_->setEnabled(false);
        forwardAction_->setEnabled(false);
        return;
    }

    QWebEngineHistory* history = tab->view->history();
    backAction_->setEnabled(history->canGoBack());
    forwardAction_->setEnabled(history->canGoForward());
    reloadAction_->setEnabled(true);
}

void BrowserWindow::addBookmark() {
    auto* tab = currentTab();
    if (!tab) return;

    const QUrl url = tab->view->url();
    if (!isAllowedUrl(url)) {
        statusLabel_->setText(tr("Only approved HTTPS pages can be bookmarked."));
        return;
    }

    QString title = tab->view->title().trimmed();
    if (title.isEmpty()) title = url.host();
    title = title.left(80);
    const QString urlText = url.toString(QUrl::FullyEncoded);

    QSettings settings;
    QStringList titles;
    QStringList urls;
    const int count = settings.beginReadArray(QStringLiteral("bookmarks"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        titles.push_back(settings.value(QStringLiteral("title")).toString());
        urls.push_back(settings.value(QStringLiteral("url")).toString());
    }
    settings.endArray();

    if (urls.contains(urlText)) {
        statusLabel_->setText(tr("That page is already bookmarked."));
        return;
    }

    titles.push_back(title);
    urls.push_back(urlText);
    settings.beginWriteArray(QStringLiteral("bookmarks"), urls.size());
    for (int i = 0; i < urls.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(QStringLiteral("title"), titles.at(i));
        settings.setValue(QStringLiteral("url"), urls.at(i));
    }
    settings.endArray();

    statusLabel_->setText(tr("Bookmark saved: %1").arg(title));
}

void BrowserWindow::rebuildBookmarksMenu() {
    if (!bookmarksMenu_) return;
    bookmarksMenu_->clear();

    QSettings settings;
    const int count = settings.beginReadArray(QStringLiteral("bookmarks"));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        const QString title = settings.value(QStringLiteral("title")).toString();
        const QUrl url(settings.value(QStringLiteral("url")).toString());
        QAction* action = bookmarksMenu_->addAction(
            title.isEmpty() ? url.host() : title);
        action->setData(url.toString(QUrl::FullyEncoded));
        connect(action, &QAction::triggered, this, [this, url]() {
            if (!isAllowedUrl(url)) {
                statusLabel_->setText(
                    tr("This bookmark is no longer in the approved-sites list."));
                return;
            }
            if (auto* tab = currentTab()) tab->renderer->navigate(url);
        });
    }
    settings.endArray();

    if (count == 0) {
        QAction* emptyAction = bookmarksMenu_->addAction(tr("No bookmarks saved yet"));
        emptyAction->setEnabled(false);
    }
}

