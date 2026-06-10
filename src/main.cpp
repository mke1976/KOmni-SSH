#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // Enable system integration features
    QApplication::setApplicationName("komni-ssh");
    QApplication::setApplicationDisplayName("KOmni-SSH");
    QApplication::setOrganizationName("omni");
    QApplication::setDesktopSettingsAware(true);

    QApplication app(argc, argv);
    
    // Set style (on Plasma this automatically defaults to native Breeze)
    app.setWindowIcon(QIcon(":/icons/app_icon.png"));

    MainWindow w;
    w.show();

    return app.exec();
}
