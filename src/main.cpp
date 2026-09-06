#include <QApplication>
#include <QWebEngineView>
#include <QMainWindow>
#include <QUrl>

class BrowserWindow : public QMainWindow
{
public:
    explicit BrowserWindow(const QUrl &url = QUrl("https://www.example.com"),
                           QWidget *parent = nullptr)
        : QMainWindow(parent)
    {
        auto *view = new QWebEngineView(this);
        view->load(url);
        setCentralWidget(view);
        setWindowTitle("Minuan Browser");
        resize(1024, 768);
    }

    void run(QApplication &app) { app.exec(); }
    void run(QApplication &app) { app.exec(); }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    BrowserWindow window;
    window.show();
    window.run(app);
    return 0;
}