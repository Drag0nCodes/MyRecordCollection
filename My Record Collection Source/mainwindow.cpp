#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "json.h"
#include <QTableWidgetItem>
#include <QDir>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <iostream>
#include <QTimer>
#include "listtag.h"
#include "tagswindow.h"
#include "importdiscogs.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QtConcurrent>

json Json;
QDir dir;
std::vector<Record> results; // Search records results
std::vector<Record> allMyRecords; // All records in collection
std::vector<Record> recordsList; // Records shown on my collection table
std::vector<ListTag> tags; // All tags
std::vector<ListTag> suggestedTags; // The list of suggested tags on the search records page
bool selectedMyRecord = false; // If a record is selected on the my record page
bool dark = true;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("My Record Collection 1.2.0");

    // Set the style sheet for the program
    QFile styleFile(dir.absolutePath() + "/resources/darktheme.qss");
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());
    setStyleSheet(style);

    QFont font("Arial");
    font.setPixelSize(14);

    // Fill the records vectors and tag vector
    allMyRecords = Json.getRecords();
    recordsList = allMyRecords;
    tags = Json.getTags();
    sortTagsAlpha(&tags);

    // Set the style of the tables
    ui->myRecord_Table->setColumnWidth(0, 140);
    ui->myRecord_Table->setColumnWidth(1, 250);
    ui->myRecord_Table->setColumnWidth(2, 130);
    ui->myRecord_Table->setColumnWidth(3, 50);
    ui->myRecord_Table->setColumnWidth(4, 192);
    ui->myRecord_Table->verticalHeader()->hide();
    ui->myRecord_Table->setFont(font);

    ui->searchRecord_Table->setColumnWidth(0, 140);
    ui->searchRecord_Table->setColumnWidth(1, 385);
    ui->searchRecord_Table->setColumnWidth(2, 237);
    ui->searchRecord_Table->verticalHeader()->hide();
    ui->searchRecord_Table->setFont(font);

    ui->myRecord_SortBox->setCurrentIndex(7); // Sort by rating decending
    updateRecordsListOrder();

    // Fill tables
    //QTimer::singleShot(1, [=](){ emit updateMyRecords(); }); // Fill my records table a second after program startup
    updateTagsList();
    updateMyRecordsTable();

    // Start off with no record selected and unable to use some buttons
    ui->myRecord_RemoveRecordButton->setDisabled(true);
    ui->myRecord_EditTagsList->setDisabled(true);
    ui->myRecord_RatingSlider->setDisabled(true);
    ui->searchRecord_AddToMyRecordButton->setDisabled(true);
    ui->myRecord_Table->setCurrentCell(-1, -1);
}

MainWindow::~MainWindow()
{
    delete ui;
}


QPixmap MainWindow::getPixmapFromUrl(const QUrl& imageUrl) { // Get Qt pixmap from image URL
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    QNetworkRequest request(imageUrl);
    QNetworkReply *reply = manager.get(request);

    loop.exec(); // Wait for the download to finish

    QPixmap pixmap;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        pixmap.loadFromData(data);
    }
    else {
        std::cerr << "No network connection\n";
    }
    if (pixmap.isNull()) { // No album cover
        QNetworkRequest requestNoImg(QUrl("https://static.vecteezy.com/system/resources/thumbnails/022/059/000/small/no-image-available-icon-vector.jpg"));
        QNetworkReply *replyNoImg = manager.get(requestNoImg);
        loop.exec();

        if (replyNoImg->error() == QNetworkReply::NoError) {
            QByteArray dataNoImg = replyNoImg->readAll();
            pixmap.loadFromData(dataNoImg);
        }
    }

    delete reply;
    return pixmap.scaled(130, 130, Qt::KeepAspectRatio);
}


void MainWindow::on_searchRecord_ToMyRecordsButton_clicked() // change screen to my records
{
    ui->pages->setCurrentIndex(0);
}


void MainWindow::on_myRecord_ToSearchRecordsButton_clicked() // Change screen to search records
{
    ui->pages->setCurrentIndex(1);
    ui->searchRecord_SuggestedTagsList->clear();
    suggestedTags.clear();
    ui->searchRecord_InfoLabel->setText("");
}


void MainWindow::on_searchRecord_SearchBar_returnPressed() // Search last.fm records
{
    ui->searchRecord_InfoLabel->setText("");
    ui->searchRecord_Table->clear();
    ui->searchRecord_Table->setHorizontalHeaderLabels({"Cover", "Record", "Artist"});
    results = Json.searchRecords(ui->searchRecord_SearchBar->text(), 10);
    ui->searchRecord_Table->setRowCount(results.size());

    for (int recordNum = 0; recordNum < results.size(); recordNum++){ // Insert record's text info into table
        QTableWidgetItem *nameItem = new QTableWidgetItem(results.at(recordNum).getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(results.at(recordNum).getArtist());

        ui->searchRecord_Table->setItem(recordNum, 1, nameItem);
        ui->searchRecord_Table->setItem(recordNum, 2, artistItem);
        ui->searchRecord_Table->setRowHeight(recordNum, 130);
    }

    for (int recordNum = 0; recordNum < results.size(); recordNum++){ // Insert record's cover into table
        QTableWidgetItem *coverItem = new QTableWidgetItem();

        QUrl imageUrl(results.at(recordNum).getCover());
        QPixmap image(getPixmapFromUrl(imageUrl));

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(Qt::DecorationRole, image);
        ui->searchRecord_Table->setItem(recordNum, 0, item);
    }
}


void MainWindow::updateMyRecordsTable(){ // Update my records list
    ui->myRecord_Table->clear();
    ui->myRecord_Table->setHorizontalHeaderLabels({"Cover", "Record", "Artist", "Rating", "Tags"});
    ui->myRecord_Table->setRowCount(recordsList.size());

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){ // Insert record's text info into table
        QTableWidgetItem *nameItem = new QTableWidgetItem(recordsList.at(recordNum).getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(recordsList.at(recordNum).getArtist());
        QTableWidgetItem *ratingItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum).getRating()));

        QString tagString = "";
        for (QString tag : recordsList.at(recordNum).getTags()) {
            if (tagString.isEmpty()) tagString += tag;
            else tagString += ", " + tag;
        }

        QTableWidgetItem *tagsItem = new QTableWidgetItem(tagString);

        ui->myRecord_Table->setItem(recordNum, 1, nameItem);
        ui->myRecord_Table->setItem(recordNum, 2, artistItem);
        ui->myRecord_Table->setItem(recordNum, 3, ratingItem);
        ui->myRecord_Table->setItem(recordNum, 4, tagsItem);
        ui->myRecord_Table->setRowHeight(recordNum, 130);
    }

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){ // Insert record's covers into table
        QPixmap image(dir.absolutePath() + "/resources/covers/" + recordsList.at(recordNum).getCover());
        if (image.isNull()) image = QPixmap(dir.absolutePath() + "/resources/images/missingImg.jpg");
        image = image.scaled(130, 130, Qt::KeepAspectRatio);

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(Qt::DecorationRole, image);
        ui->myRecord_Table->setItem(recordNum, 0, item);

        ui->myRecord_Table->setRowHeight(recordNum, 130);
    }

    // Set record count label text
    ui->myRecord_RecordCountLabel->setText("Showing " + QString::number(recordsList.size()) + "/" + QString::number(allMyRecords.size()) + " Records");
}


void MainWindow::updateMyRecordsInfo(){ // Update my records list (not images)

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){
        QTableWidgetItem *nameItem = new QTableWidgetItem(recordsList.at(recordNum).getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(recordsList.at(recordNum).getArtist());
        QTableWidgetItem *ratingItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum).getRating()));

        QString tagString = "";
        for (QString tag : recordsList.at(recordNum).getTags()) {
            if (tagString.isEmpty()) tagString += tag;
            else tagString += ", " + tag;
        }

        QTableWidgetItem *tagsItem = new QTableWidgetItem(tagString);

        ui->myRecord_Table->setItem(recordNum, 1, nameItem);
        ui->myRecord_Table->setItem(recordNum, 2, artistItem);
        ui->myRecord_Table->setItem(recordNum, 3, ratingItem);
        ui->myRecord_Table->setItem(recordNum, 4, tagsItem);
    }
}


void MainWindow::on_searchRecord_AddToMyRecordButton_clicked() // Add searched record to my collection
{
    bool copy = false;
    for (Record record : allMyRecords){ // Check all my records to see if cover matches requested add
        QString searchPageRecordCover = results.at(ui->searchRecord_Table->currentRow()).getCover();
        for (int i = searchPageRecordCover.size()-1; i >= 0; i--) {
            if (searchPageRecordCover[i] == '/') {
                searchPageRecordCover.remove(0,i+1);
                break;
            }
        }
        if (record.getCover().compare(searchPageRecordCover) == 0){ // Match with new record and one already added
            copy = true;
            break;
        }
    }
    if (!copy){ // Record is new, add to allMyRecords
        ui->searchRecord_InfoLabel->setText("");
        Record addRecord = results.at(ui->searchRecord_Table->currentRow()); // Get the selected record to add
        addRecord.setCover(Json.downloadCover(addRecord.getCover())); // Change the new records cover address from URL to file name
        addRecord.setRating(ui->searchRecord_RatingSlider->sliderPosition()); // Set records rating

        for (int i = 0; i < suggestedTags.size(); i++){
            if (suggestedTags.at(i).getChecked()) {
                addRecord.addTag(ui->searchRecord_SuggestedTagsList->item(i)->text());
            }
        }

        // Create new tags if they don't exist
        for (QString newTag : addRecord.getTags()){
            if (newTag.isEmpty()){ // Don't add a blank tag
                // nothing
            }
            else {
                bool dup = false;
                for (ListTag oldTag : tags){ // check if tag already exists
                    if (oldTag.getName().compare(newTag) == 0){
                        dup = true;
                        break;
                    }
                }
                if (!dup) { // Add tag
                    tags.push_back(ListTag(newTag));
                    sortTagsAlpha(&tags);
                    Json.writeTags(&tags);
                }
            }
        }

        updateTagsList();
        Json.writeTags(&tags);

        allMyRecords.push_back(addRecord);
        on_myRecord_SearchBar_textChanged();
        updateMyRecordsTable();
        Json.writeRecords(&allMyRecords);
        ui->searchRecord_InfoLabel->setText("Added to My Collection");
    } else { // New record is duplicate
        ui->searchRecord_InfoLabel->setText("Record already in library");
    }
}


void MainWindow::on_myRecord_RemoveRecordButton_clicked() // Remove record from my collection
{
    if (selectedMyRecord){
        deleteCover(recordsList.at(ui->myRecord_Table->currentRow()).getCover());

        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).getCover().compare(recordsList.at(ui->myRecord_Table->currentRow()).getCover()) == 0){ // Remove from allMyRecords and recordsList
                allMyRecords.erase(allMyRecords.begin() + i);
                recordsList.erase(recordsList.begin() + ui->myRecord_Table->currentRow());
                break;
            }
        }

        if (ui->myRecord_Table->currentRow() == ui->myRecord_Table->rowCount()-2){ // Update myRecords table (second last row deleted, full update of table)
            updateMyRecordsTable();
        }
        else {
            ui->myRecord_Table->removeRow(ui->myRecord_Table->currentRow()); // Update myRecords table (delete one table row)
        }

        Json.writeRecords(&allMyRecords); // Update saved records

        // Set record count label text
        ui->myRecord_RecordCountLabel->setText("Showing " + QString::number(recordsList.size()) + "/" + QString::number(allMyRecords.size()) + " Records");
    }
}


void MainWindow::on_myRecord_Table_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) // My collection record selected
{
    if (currentRow >= 0 || currentRow < recordsList.size()) { // Valid selection made of myRecords
        ui->myRecord_RemoveRecordButton->setDisabled(false);
        ui->myRecord_RatingSlider->setDisabled(false);
        ui->myRecord_RatingSlider->setSliderPosition(recordsList.at(ui->myRecord_Table->currentRow()).getRating());
        ui->myRecord_EditTagsList->setDisabled(false);
        selectedMyRecord = true;
    }
    else { // Invalid selection made of myRecords
        ui->myRecord_RemoveRecordButton->setDisabled(true);
        ui->myRecord_RatingSlider->setDisabled(true);
        ui->myRecord_EditTagsList->setDisabled(true);
        selectedMyRecord = false;
    }
    if (recordsList.empty()) { // myRecords list empty
        ui->myRecord_RemoveRecordButton->setDisabled(true);
        ui->myRecord_RatingSlider->setDisabled(true);
        ui->myRecord_EditTagsList->setDisabled(true);
        selectedMyRecord = false;
    }
}


void MainWindow::on_searchRecord_Table_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) // Search records page record selcted
{
    if (currentRow >= 0 || currentRow < results.size()) ui->searchRecord_AddToMyRecordButton->setDisabled(false); // Valid selection made of searchRecords
    else ui->searchRecord_AddToMyRecordButton->setDisabled(true); // Invalid selection made
    if (results.empty()) ui->searchRecord_AddToMyRecordButton->setDisabled(true); // Search list is empty
}


void MainWindow::on_myRecord_RatingSlider_valueChanged(int value) // Change record rating
{
    if (selectedMyRecord){
        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).getCover().compare(recordsList.at(ui->myRecord_Table->currentRow()).getCover()) == 0){
                // If selected record in list matches record in allMyRecords
                QString saveCover = allMyRecords.at(i).getCover();
                allMyRecords.at(i).setRating(value); // Set its rating in allMyRecords
                recordsList.at(ui->myRecord_Table->currentRow()).setRating(value); // Set its rating in shown records
                updateRecordsListOrder(); // Update tables
                updateMyRecordsTable();
                for (int j = 0; j < recordsList.size(); j++){ // Set the current row to the already selected record
                    if (recordsList.at(j).getCover().compare(saveCover) == 0){
                        ui->myRecord_Table->setCurrentCell(j, 0);
                        break;
                    }
                }
                break;
            }
        }
        Json.writeRecords(&allMyRecords); // Save new rating to json
    }
}


void MainWindow::on_myRecord_SearchBar_textChanged() // Search my records
{
    if (ui->myRecord_SearchBar->text().isEmpty()){
        recordsList = allMyRecords;
    }
    else {
        std::vector<Record> newRecords;
        for (Record record : allMyRecords){
            if (record.contains(ui->myRecord_SearchBar->text())) {
                newRecords.push_back(record);
            }
            recordsList = newRecords;
        }
    }
    on_myRecord_SortBox_activated(ui->myRecord_SortBox->currentIndex());
}


void MainWindow::on_myRecord_SortBox_activated(int index) // Sort my records based on combo box
{
    updateRecordsListOrder();
    updateMyRecordsTable();
}


void MainWindow::updateTagsList(){ // Update the edit and sort tag lists
    ui->myRecord_EditTagsList->clear();
    ui->myRecord_SortTagsList->clear();
    for (ListTag tag : tags){
        ui->myRecord_EditTagsList->addItem(new QListWidgetItem(tag.getName()));
        if (tag.getChecked()){
            ui->myRecord_SortTagsList->addItem(new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tag.getName()));
        }
        else ui->myRecord_SortTagsList->addItem(new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tag.getName()));
    }
}


void MainWindow::on_myRecord_EditTagsList_itemClicked(QListWidgetItem *item) // Add or remove a tag from a record
{
    if (selectedMyRecord){
        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).getCover().compare(recordsList.at(ui->myRecord_Table->currentRow()).getCover()) == 0){
                // If selected record in list matches record in allMyRecords
                std::vector<QString> recordTags = allMyRecords.at(i).getTags();
                if (allMyRecords.at(i).addTag(item->text()) == 1){
                    allMyRecords.at(i).removeTag(item->text());
                }
                if (recordsList.at(ui->myRecord_Table->currentRow()).addTag(item->text()) == 1){
                    recordsList.at(ui->myRecord_Table->currentRow()).removeTag(item->text());
                }
                break;
            }
        }
        updateMyRecordsInfo(); // Update only the text info in list
        ui->myRecord_EditTagsList->setCurrentRow(-1);
        Json.writeRecords(&allMyRecords); // Save new tags to json
    }
}


void MainWindow::on_myRecord_SortTagsList_itemClicked(QListWidgetItem *item) // Sort my records by a tag (sort tag list clicked)
{
    if (tags.at(ui->myRecord_SortTagsList->currentRow()).getChecked()){ // Tag is checked, set to unchecked
        QString tag = ui->myRecord_SortTagsList->currentItem()->text();
        tags.at(ui->myRecord_SortTagsList->currentRow()).setChecked(false);
        ui->myRecord_SortTagsList->takeItem(ui->myRecord_SortTagsList->currentRow());
        ui->myRecord_SortTagsList->insertItem(ui->myRecord_SortTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tag));
        ui->myRecord_SortTagsList->sortItems();
    }
    else { // Tag is unchecked, set to checked
        QString tag = ui->myRecord_SortTagsList->currentItem()->text();
        tags.at(ui->myRecord_SortTagsList->currentRow()).setChecked(true);
        ui->myRecord_SortTagsList->takeItem(ui->myRecord_SortTagsList->currentRow());
        ui->myRecord_SortTagsList->insertItem(ui->myRecord_SortTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tag));
        ui->myRecord_SortTagsList->sortItems();
    }

    ui->myRecord_SortTagsList->setCurrentRow(-1); // Reset row selection

    updateRecordsListOrder();
    updateMyRecordsTable();
}


void MainWindow::on_myRecord_ManageTagButton_clicked() // Open and handle manage tags popup
{
    tagsWindow* popup = new tagsWindow(&tags);
    std::vector<ListTag> saveTags = tags;

    // Do after popup closes
    connect(popup, &QDialog::finished, this, [=]() {
        std::vector<ListTag> deletedTags;

        // Figure out what tags have been deleted
        for (ListTag oldTag : saveTags){
            bool deleted = true;
            for (ListTag newTag : tags){
                if (oldTag.getName().compare(newTag.getName()) == 0){
                    deleted = false;
                    break;
                }
            }
            if (deleted) deletedTags.push_back(oldTag);
        }

        // Remove deleted tag(s) from records
        for (int i = 0; i < allMyRecords.size(); i++){
            for (ListTag deleteTag : deletedTags){
                allMyRecords.at(i).removeTag(deleteTag.getName());
            }
        }

        updateTagsList();
        recordsList = allMyRecords;
        updateRecordsListOrder();
        if (!deletedTags.empty()) updateMyRecordsTable();
    });

    popup->setModal(true);
    popup->exec();
}


void MainWindow::updateRecordsListOrder(){ // Set recordsList to have the correct records and order based on the options given (tags and sort) (Does not visably update the table)

    // Only show records with selected tags
    std::vector<Record> newRecordsList; // New vector to hold records than have required tags
    for (Record record : allMyRecords){
        bool matches = true;
        for (int i = 0; i < tags.size(); i++){
            if (tags.at(i).getChecked() && !record.hasTag(tags.at(i).getName())){ // Record should have tag but does not
                matches = false;
            }
        }
        if ((ui->myRecord_SearchBar->text().isEmpty() && matches) || (record.contains(ui->myRecord_SearchBar->text()) && matches)) { // Record has required tags and matches search bar
            newRecordsList.push_back(record);
        }
    }

    std::vector<Record> sortedRecords;

    // Show records in order according to dropdown
    switch (ui->myRecord_SortBox->currentIndex()) {
    case 0: // Oldest to newest
        recordsList = newRecordsList;
        break;
    case 1: // Newest to oldest
        std::reverse(newRecordsList.begin(),newRecordsList.end());
        recordsList = newRecordsList;
        break;
    case 2: // Title ascending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest).getName().toLower().compare(newRecordsList.at(i).getName().toLower()) > 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList = sortedRecords;
        break;
    case 3: // Title descending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest).getName().toLower().compare(newRecordsList.at(i).getName().toLower()) < 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList = sortedRecords;
        break;
    case 4: // Artist ascending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest).getArtist().toLower().compare(newRecordsList.at(i).getArtist().toLower()) > 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList = sortedRecords;
        break;
    case 5: // Artist descending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest).getArtist().toLower().compare(newRecordsList.at(i).getArtist().toLower()) < 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList = sortedRecords;
        break;
    case 6: // Rating ascending
        for (int i = 0; i <= 10; i++){
            for (Record record : newRecordsList){
                if (record.getRating() == i) sortedRecords.push_back(record);
            }
        }
        recordsList = sortedRecords;
        break;
    case 7: // Rating ascending
        for (int i = 10; i >= 0; i--){
            for (Record record : newRecordsList){
                if (record.getRating() == i) sortedRecords.push_back(record);
            }
        }
        recordsList = sortedRecords;
        break;
    }
}


bool MainWindow::deleteCover(const QString& coverName) { // Delete album cover from covers subfolder
    QFile file(dir.absolutePath() + "/resources/covers/" + coverName);

    if (file.exists()) {
        if (file.remove()) {
            qDebug() << "File" << coverName << "deleted successfully";
            return true;
        } else {
            qDebug() << "Error: Could not delete file" << coverName;
            return false;
        }
    } else {
        qDebug() << "Error: File" << coverName << "does not exist";
        return false;
    }
}


void MainWindow::on_searchRecord_Table_cellClicked(int row, int column) // Click on search records table, show suggested tags
{
    ui->searchRecord_SuggestedTagsList->clear();
    suggestedTags.clear();
    suggestedTags = Json.wikiTags(results.at(ui->searchRecord_Table->currentRow()));
    sortTagsAlpha(&suggestedTags);
    ui->searchRecord_SuggestedTagsList->clear();
    for (ListTag tag : suggestedTags) {
        ui->searchRecord_SuggestedTagsList->addItem(new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tag.getName()));
    }
}


void MainWindow::on_searchRecord_SuggestedTagsList_itemClicked(QListWidgetItem *item) // Click on suggested tags list
{

    if (suggestedTags.at(ui->searchRecord_SuggestedTagsList->currentRow()).getChecked()){ // Tag is checked, set to unchecked
        QString tag = ui->searchRecord_SuggestedTagsList->currentItem()->text();
        suggestedTags.at(ui->searchRecord_SuggestedTagsList->currentRow()).setChecked(false);
        ui->searchRecord_SuggestedTagsList->takeItem(ui->searchRecord_SuggestedTagsList->currentRow());
        ui->searchRecord_SuggestedTagsList->insertItem(ui->searchRecord_SuggestedTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tag));
    }
    else { // Tag is unchecked, set to checked
        QString tag = ui->searchRecord_SuggestedTagsList->currentItem()->text();
        suggestedTags.at(ui->searchRecord_SuggestedTagsList->currentRow()).setChecked(true);
        ui->searchRecord_SuggestedTagsList->takeItem(ui->searchRecord_SuggestedTagsList->currentRow());
        ui->searchRecord_SuggestedTagsList->insertItem(ui->searchRecord_SuggestedTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tag));
    }

    ui->searchRecord_SuggestedTagsList->sortItems();
    ui->searchRecord_SuggestedTagsList->setCurrentRow(-1); // Reset row selection
}


void MainWindow::sortTagsAlpha(std::vector<ListTag> *list) { // Sort a vector of ListTag objects alphabetically
    std::vector<QString> names;
    std::vector<ListTag> copy = *list;

    for (ListTag tag : *list){
        names.push_back(tag.getName());
    }
    std::sort(names.begin(), names.end());

    list->clear();
    for (int i = 0; i < names.size(); i++) {
        for (int j = 0; j < copy.size(); j++) {
            if (names.at(i).compare(copy.at(j).getName()) == 0){
                list->push_back(copy.at(j));
                break;
            }
        }
    }
}

void MainWindow::on_myRecord_PickForMe_clicked() // Highlight a random record in the table
{
    if (ui->myRecord_Table->rowCount()>0) ui->myRecord_Table->setCurrentCell(rand() % recordsList.size(), 0);
}


void MainWindow::on_settings_actionImportDiscogs_triggered() // "Import Discogs Collection" menubar button clicked
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Discogs Collection"), "/", tr("CSV files (*.csv)")); // Open file selector popup
    if (!filePath.isEmpty()) { // If file is chosen...
        QFile myFile(filePath); // File of playlists JSON
        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly)){ // Erase all text in playlists json
                QString line = myFile.readLine();
                if (!line.startsWith("Catalog#,Artist,Title")){ // Not a discogs collection, invalid file
                    myFile.close();
                    std::cerr << "Inavlid collection file" << std::endl;
                    ui->myRecord_InfoLabel->setText("Invalid Discogs Collection");
                }
                else {
                    line = myFile.readLine(); // Get first discogs record
                    int tot = 0; // The total number of records in the discogs file
                    std::vector<QThread*> threads; // Thread for each record import
                    std::vector<ImportDiscogs*> imports; // The ImportDiscogs class for each record import, will be moved to thread in vector threads
                    while (!line.isEmpty()) { // Loop until all discogs record read in
                        imports.push_back(new ImportDiscogs(line, &allMyRecords)); // Create and add ImportDiscogs class to vector
                        threads.push_back(new QThread); // Create a QThread for record
                        imports.at(tot)->moveToThread(threads.at(tot)); // Move the ImportDiscogs to its won thread

                        // Connect for started and finished signals
                        QObject::connect(threads.at(tot), &QThread::started, imports.at(tot), &ImportDiscogs::run); // Run import method when thread starts
                        QObject::connect(imports.at(tot), &ImportDiscogs::finished, threads.at(tot), &QThread::quit); // When ImportDiscogs run finished, quit QThread
                        QObject::connect(threads.at(tot), &QThread::finished, threads.at(tot), &QThread::deleteLater); // When thread finished, delete thread

                        threads.at(tot)->start(); // Start thread
                        line = myFile.readLine(); // Get next discogs record
                        tot++;
                    }
                    for (int i = 0; i < tot; i++){ // Get results from each record
                        while (imports.at(i)->getProcessedRec() == nullptr); // Loop main until import is finished
                        Record *importedRec = imports.at(i)->getProcessedRec(); // Get the new Record
                        if (!(importedRec->getName().compare("") == 0 & importedRec->getArtist().compare("") == 0 && importedRec->getCover().compare("") == 0 && importedRec->getRating() == -1)){
                            allMyRecords.push_back(*imports.at(i)->getProcessedRec()); // Record is new, add to collection, don't add duplicates
                        }
                        imports.at(i)->deleteLater(); // Delete object
                    }
                    Json.writeRecords(&allMyRecords); // Write changes to json
                    updateRecordsListOrder(); // Update list orders
                    updateMyRecordsTable();
                }
            }
        }
    }
}

void MainWindow::on_actionImport_Discogs_Single_Threaded_triggered()
{
    QProgressDialog progress("Importing Discogs Collection...", "", 0, 0, this);
    progress.setMinimumDuration(0);
    ImportDiscogs import;
    progress.setWindowModality(Qt::WindowModal);
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Discogs Collection"), "/", tr("CSV files (*.csv)")); // Open file selector popup
    if (!filePath.isEmpty()) { // If file is chosen...
        import.importAll(filePath, &allMyRecords); // Import collection from file
        updateRecordsListOrder(); // Update the vector with the record collection table elements
        updateMyRecordsTable(); // Visably update the record collection table*/
    }
}


void MainWindow::on_settings_actionExit_triggered()
{
    abort();
}


void MainWindow::on_settings_actionToggleTheme_triggered()
{
    QFile *styleFile;
    if (dark){
        styleFile = new QFile(dir.absolutePath() + "/resources/lighttheme.qss");
    }
    else {
        styleFile = new QFile(dir.absolutePath() + "/resources/darktheme.qss");
    }
    styleFile->open(QFile::ReadOnly);
    QString style(styleFile->readAll());
    setStyleSheet(style);
    dark = !dark;
}

