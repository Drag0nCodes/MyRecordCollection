#ifndef TAGSWINDOW_H
#define TAGSWINDOW_H

#include <QDialog>
#include "listtag.h"
#include "prefs.h"

namespace Ui {
class TagsWindow;
}

class TagsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit TagsWindow(std::vector<ListTag>* tags, Prefs *prefs, QWidget *parent = nullptr);
    ~TagsWindow();

private slots:
    void on_tagsWinDoneButton_clicked();

    void on_tagsWinRemoveButton_clicked();

    void on_tagsWinAddButton_clicked();

    void on_tagsWinAddLineEdit_returnPressed();

private:
    Ui::TagsWindow *ui;
    std::vector<ListTag> &tagsList;
};

#endif // TAGSWINDOW_H
