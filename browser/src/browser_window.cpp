#include "browser_window.hpp"

#include "dom_processor.hpp"
#include "html6_engine.hpp"
#include "sunay/rendering_pipeline.hpp"
#include "utils.hpp"

#include <QAction>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineView>

#include <functional>
#include <utility>

namespace {

class SafeWebPage final : public QWebEnginePage {
public:
    using BlockedHandler = std::function<void(const QUrl&)>;

    SafeWebPage(const QSet<QString>& allowedHosts,
                BlockedHandler blockedHandler,
                QObject* parent = nullptr)
        : QWebEnginePage(parent),
          allowedHosts_(allowedHosts),
          blockedHandler_(std::move(blockedHandler)) {}

protected:
    bool acceptNavigationRequest(const QUrl& url,
                                 QWebEnginePage::NavigationType type,
                                 bool isMainFrame) override {
        Q_UNUSED(type);
        Q_UNUSED(isMainFrame);

        if (url.scheme() == QStringLiteral("about") &&
            url.toString() == QStringLiteral("about:blank")) {
            return true;
        }

        const QString scheme = url.scheme().toLower();
        const QString host = url.host().toLower();
        for (const QString& allowed : allowedHosts_) {
            if (host == allowed || host.endsWith(QStringLiteral(".") + allowed)) {
                if (scheme == QStringLiteral("https")) {
                    return QWebEnginePage::acceptNavigationRequest(
                        url, type, isMainFrame);
                }
                break;
            }
        }

        if (blockedHandler_) blockedHandler_(url);
        return false;
    }

private:
    QSet<QString> allowedHosts_;
    BlockedHandler blockedHandler_;
};

QString cleanHost(QString host) {
    host = host.trimmed().toLower();
    if (host.startsWith(QStringLiteral("www."))) {
        host.remove(0, 4);
    }
    while (host.startsWith(QStringLiteral("*."))) {
        host.remove(0, 2);
    }
    while (host.endsWith(QLatin1Char('.'))) {
        host.chop(1);
    }
    return host;
}

} // namespace

BrowserWindow::BrowserWindow() {
    setWindowTitle(QStringLiteral("Minuan"));
    resize(1280, 820);

    const bool sitesLoaded = loadWebsites();
    buildUi();
    createTab(homeUrl_.isEmpty()
                  ? QUrl(QStringLiteral("about:blank"))
                  : homeUrl_);

    if (!sitesLoaded) {
        statusBar()->showMessage(
            tr("No allowed websites were loaded. Add HTTPS sites to browser/websites.json."),
            10000);
    }
}

void BrowserWindow::buildUi() {
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
    addressBar_->setPlaceholderText(tr("Enter an allowed HTTPS address"));
    addressBar_->setMinimumWidth(360);
    navigation->addWidget(addressBar_);

    QAction* goAction = navigation->addAction(tr("Go"));
    navigation->addSeparator();
    QAction* newTabAction = navigation->addAction(tr("New tab"));
    QAction* openFileAction = navigation->addAction(tr("Open HTML6"));

    backAction_->setShortcut(QKeySequence::Back);
    forwardAction_->setShortcut(QKeySequence::Forward);
    reloadAction_->setShortcut(QKeySequence::Refresh);
    stopAction_->setShortcut(QKeySequence(Qt::Key_Escape));
    stopAction_->setEnabled(false);
    homeAction->setShortcut(QKeySequence(QStringLiteral("Alt+Home")));
    newTabAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    openFileAction->setShortcut(QKeySequence::Open);
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
        if (!homeUrl_.isEmpty()) {
            if (auto* tab = currentTab()) tab->renderer->navigate(homeUrl_);
        }
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

        allowedHosts_.insert(cleanHost(url.host()));
        if (homeUrl_.isEmpty()) homeUrl_ = url;
    }

    return !allowedHosts_.isEmpty();
}

bool BrowserWindow::isAllowedHost(const QString& candidate) const {
    const QString host = candidate.toLower();
    for (const QString& allowed : allowedHosts_) {
        if (host == allowed || host.endsWith(QStringLiteral(".") + allowed)) {
            return true;
        }
    }
    return false;
}

bool BrowserWindow::isAllowedUrl(const QUrl& url) const {
    return url.isValid() &&
           url.scheme() == QStringLiteral("https") &&
           isAllowedHost(url.host());
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
        view);
    view->setPage(page);
    layout->addWidget(view);

    auto* renderer = new Sunay::RenderingPipeline(view, tabWidget);
    Sunay::RenderingOptions options;
    options.javaScriptEnabled = true;
    options.imagesEnabled = true;
    options.localContentCanAccessRemoteUrls = false;
    renderer->setOptions(options);

    tabRuntimes_.insert(tabWidget, TabRuntime{view, renderer});

    connect(renderer, &Sunay::RenderingPipeline::renderingStarted,
            this, [this, tabWidget]() {
                if (tabs_->currentWidget() == tabWidget) {
                    statusLabel_->setText(tr("Loading page..."));
                    progressBar_->setValue(0);
                    progressBar_->show();
                    stopAction_->setEnabled(true);
                }
            });

    connect(renderer, &Sunay::RenderingPipeline::loadProgressChanged,
            this, [this, tabWidget](int progress) {
                if (tabs_->currentWidget() == tabWidget) {
                    progressBar_->setValue(progress);
                    progressBar_->setVisible(progress < 100);
                }
            });

    connect(renderer, &Sunay::RenderingPipeline::renderingFinished,
            this, [this, tabWidget](bool success) {
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
                    addressBar_->setText(url.toDisplayString());
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
        }
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
    QUrl url = QUrl::fromUserInput(enteredText);
    if (url.scheme() == QStringLiteral("http") &&
        !enteredText.contains(QStringLiteral("://"))) {
        url.setScheme(QStringLiteral("https"));
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

    const QUrl baseUrl = QUrl::fromLocalFile(
        QFileInfo(path).absoluteFilePath());
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
    addressBar_->setText(url.toDisplayString());
    const QString title = tab->view->title();
    setWindowTitle(title.isEmpty()
                       ? QStringLiteral("Minuan")
                       : title + QStringLiteral(" — Minuan"));
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

