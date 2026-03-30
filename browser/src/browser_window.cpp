#include "browser_window.hpp"
#include "html6_engine.hpp"
#include "dom_processor.hpp"
#include "utils.hpp"

#include <windows.h>
#include <wil/com.h>
#include <WebView2.h>
#include <fstream>
#include <sstream>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BrowserWindow::BrowserWindow() {}

BrowserWindow::~BrowserWindow() {
    if (webview_) {
        ((ICoreWebView2Controller*)webview_)->Close();
    }
    if (sunay_) {
        sunay_->shutdown();
    }
}

bool BrowserWindow::create() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MinuanBrowserWindow";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"MinuanBrowserWindow",
        L"Minuan Browser",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd) {
        Utils::logError("Failed to create browser window");
        return false;
    }

    native_window_handle_ = hwnd;

    // Initialize Sunay (Browser Engine)
    sunay_ = Sunay::Engine::create();
    sunay_->initialize(hwnd);

    // Initial UI state
    ui_state_.tabs.push_back({ "New Tab", "about:blank", true });
    ui_state_.addressBarText = "about:blank";
    ui_state_.isLoading = false;

    // Render browser chrome
    sunay_->render(ui_state_);

    ShowWindow(hwnd, SW_SHOW);

    return initWebView();
}

void BrowserWindow::show() {
    MSG msg = { };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool BrowserWindow::initWebView() {
    HWND hwnd = (HWND)native_window_handle_;

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this, hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                env->CreateCoreWebView2Controller(
                    hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this, hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {

                            if (!controller) {
                                Utils::logError("WebView2 controller creation failed");
                                return E_FAIL;
                            }

                            webview_ = controller;

                            // Store controller pointer for WindowProc
                            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)controller);

                            ICoreWebView2* webview;
                            controller->get_CoreWebView2(&webview);

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            controller->put_Bounds(bounds);

                            return S_OK;
                        }
                    ).Get()
                );

                return S_OK;
            }
        ).Get()
    );

    return SUCCEEDED(hr);
}

void BrowserWindow::navigateToHtml6(const std::string& path) {
    // Update Sunay UI state
    ui_state_.addressBarText = path;
    ui_state_.isLoading = true;
    sunay_->render(ui_state_);

    std::ifstream in(path);
    if (!in) {
        Utils::logError("Failed to open HTML6 file: " + path);
        ui_state_.isLoading = false;
        sunay_->render(ui_state_);
        return;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string html6 = buffer.str();

    Minuan::HTML6::HTML6Engine engine;
    if (!engine.parseDocument(html6)) {
        Utils::logError("HTML6 parse failed");
        ui_state_.isLoading = false;
        sunay_->render(ui_state_);
        return;
    }

    Minuan::HTML6::DOMProcessor processor;
    std::string html5 = processor.transformToHTML5(engine.getAST());

    loadHtml5IntoWebView(html5);

    // Done loading
    ui_state_.isLoading = false;
    sunay_->render(ui_state_);
}

void BrowserWindow::loadHtml5IntoWebView(const std::string& html5) {
    ICoreWebView2Controller* controller = (ICoreWebView2Controller*)webview_;
    if (!controller) {
        Utils::logError("WebView2 controller is null");
        return;
    }

    ICoreWebView2* webview;
    controller->get_CoreWebView2(&webview);

    std::wstring whtml(html5.begin(), html5.end());
    webview->NavigateToString(whtml.c_str());
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

        case WM_SIZE: {
            ICoreWebView2Controller* controller =
                reinterpret_cast<ICoreWebView2Controller*>(
                    GetWindowLongPtr(hwnd, GWLP_USERDATA)
                );

            if (controller) {
                RECT bounds;
                GetClientRect(hwnd, &bounds);
                controller->put_Bounds(bounds);
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
