#pragma once

#include <QHash>
#include <QMainWindow>
#include <QSet>
#include <QUrl>

class QAction;
class QLabel;
class QLineEdit;
class QProgressBar;
class QTabWidget;
class QWidget;
class QWebEngineView;

namespace Sunay {
class RenderingPipeline;
}

class BrowserWindow final : public QMainWindow {
public:
    BrowserWindow();
    ~BrowserWindow() override = default;

private:
    struct TabRuntime {
        QWebEngineView* view = nullptr;
        Sunay::RenderingPipeline* renderer = nullptr;
    };

    void buildUi();
    void createTab(const QUrl& initialUrl = QUrl(QStringLiteral("about:blank")));
    void closeTab(int index);
    void navigateAddress();
    void openLocalHtml6();
    void loadHtml6File(const QString& path);
    void updateCurrentTab();
    void updateNavigationActions();
    bool loadWebsites();
    bool isAllowedUrl(const QUrl& url) const;
    bool isAllowedHost(const QString& host) const;
    TabRuntime* currentTab();

    QTabWidget* tabs_ = nullptr;
    QLineEdit* addressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QAction* backAction_ = nullptr;
    QAction* forwardAction_ = nullptr;
    QAction* reloadAction_ = nullptr;
    QAction* stopAction_ = nullptr;

    QSet<QString> allowedHosts_;
    QUrl homeUrl_;
    QHash<QWidget*, TabRuntime> tabRuntimes_;
};
