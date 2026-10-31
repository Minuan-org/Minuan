#include "browser_window.hpp"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Minuan"));
    QCoreApplication::setOrganizationName(QStringLiteral("SimVerse"));

    BrowserWindow window;
    window.show();

    return app.exec();
}
