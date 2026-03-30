#pragma once

#include <string>

class BrowserWindow {
public:
    BrowserWindow();
    ~BrowserWindow();

    bool create();
    void show();
    void navigateToHtml6(const std::string& path);

private:
    bool initWebView();
    void loadHtml5IntoWebView(const std::string& html5);

    void* native_window_handle_ = nullptr;
    void* webview_ = nullptr;
};
