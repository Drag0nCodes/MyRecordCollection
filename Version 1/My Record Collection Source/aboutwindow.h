#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

#include <QDialog>
#include "prefs.h"

namespace Ui {
class AboutWindow;
}

class AboutWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AboutWindow(Prefs *prefs, QWidget *parent = nullptr);
    ~AboutWindow();

private slots:
    void on_aboutWinClose_clicked();

private:
    Ui::AboutWindow *ui;
};

#endif // ABOUTWINDOW_H
