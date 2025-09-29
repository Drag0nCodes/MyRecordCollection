/* Colin Brown - My Record Collection - 2024
 *
 * With vinyl records making such a comeback, this is a software to help you manage your growing collection!
 * Records can be given tags and ratings to help you sort through your collection and decide what to listen to next.
 */

#include "mainwindow.h"
#include <QDir>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/appico.ico"));
    w.show();
    return a.exec();
}
