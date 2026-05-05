#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("LoveCicada");
    app.setApplicationName("visualFrameInfo");

    MainWindow window;
    window.show();

    return app.exec();
}
