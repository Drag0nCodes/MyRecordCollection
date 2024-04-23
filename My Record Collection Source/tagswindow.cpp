#include "tagswindow.h"
#include "ui_tagswindow.h"
#include "json.h"
#include "QDir"

tagsWindow::tagsWindow(std::vector<ListTag>* tags, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::tagsWindow)
    , tagsList(*tags)
{
    ui->setupUi(this);
    setWindowTitle("Manage Tags");
    for (int i = 0; i < tags->size(); i++){
        ui->tagsWinList->addItem(new QListWidgetItem(tags->at(i).getName()));
    }

    // Set window theme
    QDir dir;
    QFile styleFile(dir.absolutePath() + "/resources/darktheme.qss");
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());
    setStyleSheet(style);
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
    if (ui->tagsWinList->currentRow()>=0) {
        json json;
        ui->tagsWinMessageLabel->setText("Tag removed: " + tagsList.at(ui->tagsWinList->currentRow()).getName());
        tagsList.erase(tagsList.begin()+ui->tagsWinList->currentRow());
        ui->tagsWinList->clear();
        for (int i = 0; i < tagsList.size(); i++){
            ui->tagsWinList->addItem(new QListWidgetItem(tagsList.at(i).getName()));
        }
        json.writeTags(&tagsList);
    }
}


void tagsWindow::on_tagsWinAddButton_clicked()
{
    QString newTag = ui->tagsWinAddLineEdit->text().toLower().toLatin1();
    if (newTag.isEmpty()){ // Don't add a blank tag
        // nothing
    }
    else {
        bool dup = false;
        for (ListTag oldTag : tagsList){ // check if tag already exists
            if (oldTag.getName().compare(newTag) == 0){
                dup = true;
                break;
            }
        }
        if (dup) { // Do not add tag, duplicate
            ui->tagsWinMessageLabel->setText("Tag already exists");
        }
        else { // Add tag
            json json;
            ui->tagsWinMessageLabel->setText("Tag added: " + newTag);
            tagsList.push_back(ListTag(newTag));

            //Sort the tags alphabetically
            std::vector<QString> names;
            std::vector<ListTag> copy = tagsList;

            for (ListTag tag : tagsList){
                names.push_back(tag.getName());
            }
            std::sort(names.begin(), names.end());

            tagsList.clear();
            for (int i = 0; i < names.size(); i++) {
                for (int j = 0; j < copy.size(); j++) {
                    if (names.at(i).compare(copy.at(j).getName()) == 0){
                        tagsList.push_back(copy.at(j));
                        break;
                    }
                }
            }

            // Add to all lists
            ui->tagsWinList->addItem(newTag);
            ui->tagsWinList->sortItems();
            ui->tagsWinAddLineEdit->clear();
            json.writeTags(&tagsList); // Write tag to json
        }
    }
}


void tagsWindow::on_tagsWinAddLineEdit_returnPressed()
{
    on_tagsWinAddButton_clicked();
}

