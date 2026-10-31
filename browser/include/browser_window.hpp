#pragma once

#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QSet>
#include <QUrl>

class QAction;
class QLabel;
class QLineEdit;
class QMenu;
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
        bool javaScriptEnabled = true;
        bool imagesEnabled = true;
        bool isLoading = false;
        int loadProgress = 0;
        QString displayAddress;
        bool keepDisplayAddress = false;
    };

    void buildUi();
    void createTab(const QUrl& initialUrl = QUrl(QStringLiteral("about:blank")));
    void closeTab(int index);
    void navigateAddress();
    void addBookmark();
    void rebuildBookmarksMenu();
    void showHomePage(TabRuntime* tab);
    void openLocalHtml6();
    void loadHtml6File(const QString& path);
    void updateCurrentTab();
    void updateNavigationActions();
    bool loadWebsites();
    bool isAllowedUrl(const QUrl& url) const;
    TabRuntime* currentTab();

    QTabWidget* tabs_ = nullptr;
    QLineEdit* addressBar_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QAction* backAction_ = nullptr;
    QAction* forwardAction_ = nullptr;
    QAction* reloadAction_ = nullptr;
    QAction* stopAction_ = nullptr;
    QAction* javaScriptAction_ = nullptr;
    QAction* imagesAction_ = nullptr;
    QMenu* bookmarksMenu_ = nullptr;

    QSet<QString> allowedHosts_;
    QHash<QWidget*, TabRuntime> tabRuntimes_;
};
