#ifndef TAGSWINDOW_H
#define TAGSWINDOW_H

#include <QDialog>
#include "listtag.h"
#include "prefs.h"

namespace Ui {
class tagsWindow;
}

class tagsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit tagsWindow(std::vector<ListTag>* tags, Prefs *prefs, QWidget *parent = nullptr);
    ~tagsWindow();

private slots:
    void on_tagsWinDoneButton_clicked();

    void on_tagsWinRemoveButton_clicked();

    void on_tagsWinAddButton_clicked();

    void on_tagsWinAddLineEdit_returnPressed();

private:
    Ui::tagsWindow *ui;
    std::vector<ListTag> &tagsList;
};

#endif // TAGSWINDOW_H
