#include "tagswindow.h"
#include "ui_tagswindow.h"
#include "json.h"

tagsWindow::tagsWindow(std::vector<QString>* tags, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::tagsWindow)
    , tagsList(*tags)
{
    ui->setupUi(this);
    setWindowTitle("Manage Tags");
    for (int i = 0; i < tags->size(); i++){
        ui->tagsWinList->addItem(new QListWidgetItem(tags->at(i)));
    }
}

tagsWindow::~tagsWindow()
{
    delete ui;
}

void tagsWindow::on_tagsWinDoneButton_clicked()
{
    this->close();
}


void tagsWindow::on_tagsWinRemoveButton_clicked()
{
    json json;
    ui->tagsWinMessageLabel->setText("Tag removed");
    tagsList.erase(tagsList.begin()+ui->tagsWinList->currentRow());
    ui->tagsWinList->clear();
    for (int i = 0; i < tagsList.size(); i++){
        ui->tagsWinList->addItem(new QListWidgetItem(tagsList.at(i)));
    }
    json.writeTags(&tagsList);
}


void tagsWindow::on_tagsWinAddButton_clicked()
{
    QString newTag = ui->tagsWinAddLineEdit->text().toLower().toLatin1();
    if (newTag.isEmpty()){
        // nothing
    }
    else {
        bool dup = false;
        for (QString oldTag : tagsList){ // check if tag already exists
            if (oldTag.compare(newTag) == 0){
                dup = true;
                break;
            }
        }
        if (dup) { // Do not add tag, duplicate
            ui->tagsWinMessageLabel->setText("Tag already exists");
        }
        else { // Add tag
            json json;
            ui->tagsWinMessageLabel->setText("Tag added");
            tagsList.push_back(newTag);
            std::sort(tagsList.begin(),tagsList.end());
            ui->tagsWinList->addItem(newTag);
            ui->tagsWinList->sortItems();
            ui->tagsWinAddLineEdit->clear();
            json.writeTags(&tagsList);
        }
    }
}


void tagsWindow::on_tagsWinAddLineEdit_returnPressed()
{
    on_tagsWinAddButton_clicked();
}

