#include "mainwindow.h"
#include <QDir>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(QDir::currentPath() + "/resources/images/appico.ico"));
    w.show();
    return a.exec();
}
