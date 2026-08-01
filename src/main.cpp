#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("app_depenses");
    MainWindow window;
    window.setWindowTitle("Expense Splitter");
    window.resize(1000, 800);
    window.show();
    return app.exec();
}