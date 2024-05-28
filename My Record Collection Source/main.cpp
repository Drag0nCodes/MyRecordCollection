#include "mainwindow.h"
#include <QDir>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QDir dir;
    w.setWindowIcon(QIcon(dir.absolutePath() + "/resources/images/appico.ico"));
    w.show();
    return a.exec();
}
