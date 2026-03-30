#pragma once

#include <string>
#include <memory>
#include "sunay.hpp"   // Sunay lives in its own repo, Minuan depends on it

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

    // Sunay browser engine (browser renderer)
    std::unique_ptr<Sunay::Engine> sunay_;
    Sunay::BrowserUIState ui_state_;
};
