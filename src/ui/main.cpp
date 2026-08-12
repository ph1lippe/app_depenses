#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("app_depenses");
    MainWindow window;
    window.setWindowTitle("Expense Splitter");
    window.showMaximized();
    return app.exec();
}