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
#include "tagswindow.h"
#include "windows.h"


json json;
QDir dir;
std::vector<Record> results;
std::vector<Record> allMyRecords;
std::vector<Record> recordsList;
std::vector<QString> tags;
std::vector<bool> tagsSort;
bool selectedMyRecord = false;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Vinyl App 1.1");

    // Set the style sheet for the program
    QFile styleFile(dir.absolutePath() + "/resources/darktheme.qss");
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());
    setStyleSheet(style);

    QFont font("Arial");
    font.setPixelSize(14);

    // Fill the records vectors and tag vector
    allMyRecords = json.getRecords();
    recordsList = allMyRecords;
    tags = json.getTags();
    for (int i = 0; i < tags.size(); i++){
        tagsSort.push_back(false);
    }
    std::sort(tags.begin(),tags.end());

    // Set the style of the tables
    ui->myRecordTable->setColumnWidth(0, 135);
    ui->myRecordTable->setColumnWidth(1, 250);
    ui->myRecordTable->setColumnWidth(2, 130);
    ui->myRecordTable->setColumnWidth(3, 50);
    ui->myRecordTable->setColumnWidth(4, 199);
    ui->myRecordTable->verticalHeader()->hide();
    ui->myRecordTable->setFont(font);

    ui->searchRecordTable->setColumnWidth(0, 135);
    ui->searchRecordTable->setColumnWidth(1, 385);
    ui->searchRecordTable->setColumnWidth(2, 244);
    ui->searchRecordTable->verticalHeader()->hide();
    ui->searchRecordTable->setFont(font);

    ui->myRecordSortBox->setCurrentIndex(7); // Sort by rating decending
    updateRecordsListOrder();

    // Fill tables
    //QTimer::singleShot(1, [=](){ emit updateMyRecords(); }); // Fill my records table a second after program startup
    updateTagsList();
    updateMyRecords();

    // Start off with no record selected and unable to use some buttons
    ui->removeFromMyRecordButton->setDisabled(true);
    ui->myRecordEditTagsList->setDisabled(true);
    ui->myRecordRatingSlider->setDisabled(true);
    ui->addToMyRecordButton->setDisabled(true);
    ui->myRecordTable->setCurrentCell(-1, -1);
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


void MainWindow::on_toMyRecordsButton_clicked() // change screen to my records
{
    ui->pages->setCurrentIndex(0);
}


void MainWindow::on_toSearchRecordsButton_clicked() // Change screen to search records
{
    ui->pages->setCurrentIndex(1);
    ui->searchRecordsInfoLabel->setText("");
}

void MainWindow::on_searchRecordSearchBar_returnPressed() // Search last.fm records
{
    ui->searchRecordTable->clear();
    ui->searchRecordTable->setHorizontalHeaderLabels({"Cover", "Record", "Artist"});
    results = json.searchRecords(ui->searchRecordSearchBar->text());
    ui->searchRecordTable->setRowCount(results.size());

    for (int recordNum = 0; recordNum < results.size(); recordNum++){ // Insert record's text info into table
        QTableWidgetItem *nameItem = new QTableWidgetItem(results.at(recordNum).getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(results.at(recordNum).getArtist());

        ui->searchRecordTable->setItem(recordNum, 1, nameItem);
        ui->searchRecordTable->setItem(recordNum, 2, artistItem);
        ui->searchRecordTable->setRowHeight(recordNum, 130);
    }

    for (int recordNum = 0; recordNum < results.size(); recordNum++){ // Insert record's cover into table
        QTableWidgetItem *coverItem = new QTableWidgetItem();

        QUrl imageUrl(results.at(recordNum).getCover());
        QPixmap image(getPixmapFromUrl(imageUrl));

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(Qt::DecorationRole, image);
        ui->searchRecordTable->setItem(recordNum, 0, item);
    }

    ui->searchRecordsInfoLabel->setText("");
}

void MainWindow::updateMyRecords(){ // Update my records list
    ui->myRecordTable->clear();
    ui->myRecordTable->setHorizontalHeaderLabels({"Cover", "Record", "Artist", "Rating", "Tags"});
    ui->myRecordTable->setRowCount(recordsList.size());

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

        ui->myRecordTable->setItem(recordNum, 1, nameItem);
        ui->myRecordTable->setItem(recordNum, 2, artistItem);
        ui->myRecordTable->setItem(recordNum, 3, ratingItem);
        ui->myRecordTable->setItem(recordNum, 4, tagsItem);
        ui->myRecordTable->setRowHeight(recordNum, 130);
    }

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){ // Insert record's covers into table
        QPixmap image(dir.absolutePath() + "/resources/covers/" + recordsList.at(recordNum).getCover());
        image = image.scaled(130, 130, Qt::KeepAspectRatio);

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(Qt::DecorationRole, image);
        ui->myRecordTable->setItem(recordNum, 0, item);

        ui->myRecordTable->setRowHeight(recordNum, 130);
    }

    // Set record count label text
    ui->myRecordRecordCountLabel->setText("Showing " + QString::number(recordsList.size()) + "/" + QString::number(allMyRecords.size()) + " Records");
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

        ui->myRecordTable->setItem(recordNum, 1, nameItem);
        ui->myRecordTable->setItem(recordNum, 2, artistItem);
        ui->myRecordTable->setItem(recordNum, 3, ratingItem);
        ui->myRecordTable->setItem(recordNum, 4, tagsItem);
    }
}


void MainWindow::on_addToMyRecordButton_clicked() // Add searched record to my collection
{
    bool copy = false;
    for (Record record : allMyRecords){ // Check all my records to see if cover matches requested add
        QString searchPageRecordCover = results.at(ui->searchRecordTable->currentRow()).getCover();
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
        ui->searchRecordsInfoLabel->setText("");
        Record addRecord = results.at(ui->searchRecordTable->currentRow());
        addRecord.setCover(downloadCover(addRecord.getCover())); // Change the new records cover address from URL to file name
        allMyRecords.push_back(addRecord);
        on_myRecordSearchBar_textChanged();
        updateMyRecords();
        json.writeRecords(&allMyRecords);
        ui->searchRecordsInfoLabel->setText("Added to My Collection");
    } else { // New record is duplicate
        ui->searchRecordsInfoLabel->setText("Record already in library");
    }
}


void MainWindow::on_removeFromMyRecordButton_clicked() // Remove record from my collection
{
    if (selectedMyRecord){
        deleteCover(recordsList.at(ui->myRecordTable->currentRow()).getCover());

        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).getCover().compare(recordsList.at(ui->myRecordTable->currentRow()).getCover()) == 0){ // Remove from allMyRecords and recordsList
                allMyRecords.erase(allMyRecords.begin() + i);
                recordsList.erase(recordsList.begin() + ui->myRecordTable->currentRow());
                break;
            }
        }

        if (ui->myRecordTable->currentRow() == ui->myRecordTable->rowCount()-2){ // Update myRecords table (second last row deleted, full update of table)
            updateMyRecords();
        }
        else {
            ui->myRecordTable->removeRow(ui->myRecordTable->currentRow()); // Update myRecords table (delete one table row)
        }

        json.writeRecords(&allMyRecords); // Update saved records

        // Set record count label text
        ui->myRecordRecordCountLabel->setText("Showing " + QString::number(recordsList.size()) + "/" + QString::number(allMyRecords.size()) + " Records");
    }
}


void MainWindow::on_myRecordTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) // My collection record selected
{
    if (currentRow >= 0 || currentRow < recordsList.size()) { // Valid selection made of myRecords
        ui->removeFromMyRecordButton->setDisabled(false);
        ui->myRecordRatingSlider->setDisabled(false);
        ui->myRecordRatingSlider->setSliderPosition(recordsList.at(ui->myRecordTable->currentRow()).getRating());
        ui->myRecordEditTagsList->setDisabled(false);
        selectedMyRecord = true;
    }
    else { // Invalid selection made of myRecords
        ui->removeFromMyRecordButton->setDisabled(true);
        ui->myRecordRatingSlider->setDisabled(true);
        ui->myRecordEditTagsList->setDisabled(true);
        selectedMyRecord = false;
    }
    if (recordsList.empty()) { // myRecords list empty
        ui->removeFromMyRecordButton->setDisabled(true);
        ui->myRecordRatingSlider->setDisabled(true);
        ui->myRecordEditTagsList->setDisabled(true);
        selectedMyRecord = false;
    }
}


void MainWindow::on_searchRecordTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) // Search records page record selcted
{
    if (currentRow >= 0 || currentRow < results.size()) ui->addToMyRecordButton->setDisabled(false); // Valid selection made of searchRecords
    else ui->addToMyRecordButton->setDisabled(true); // Invalid selection made
    if (results.empty()) ui->addToMyRecordButton->setDisabled(true); // Search list is empty
}


void MainWindow::on_myRecordRatingSlider_valueChanged(int value) // Change record rating
{
    if (selectedMyRecord){
        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).getCover().compare(recordsList.at(ui->myRecordTable->currentRow()).getCover()) == 0){
                // If selected record in list matches record in allMyRecords
                allMyRecords.at(i).setRating(value); // Set its rating in allMyRecords
                recordsList.at(ui->myRecordTable->currentRow()).setRating(value); // Set its rating in shown records
                break;
            }
        }
        updateMyRecordsInfo(); // Update only the text info in list
        json.writeRecords(&allMyRecords); // Save new rating to json
    }
}


void MainWindow::on_myRecordSearchBar_textChanged() // Search my records, enter pressed
{
    if (ui->myRecordSearchBar->text().isEmpty()){
        recordsList = allMyRecords;
    }
    else {
        std::vector<Record> newRecords;
        for (Record record : allMyRecords){
            if (record.contains(ui->myRecordSearchBar->text())) {
                newRecords.push_back(record);
            }
            recordsList = newRecords;
        }
    }
    on_myRecordSortBox_activated(ui->myRecordSortBox->currentIndex());
}


void MainWindow::on_myRecordSearchBar_textChanged(const QString &arg1) // Search my records text changed
{
    if (ui->myRecordSearchBar->text().isEmpty()){ // If my record search bar is empty, fill list with all records
        recordsList = allMyRecords;
        on_myRecordSortBox_activated(ui->myRecordSortBox->currentIndex());
    }
}


void MainWindow::on_myRecordSortBox_activated(int index) // Sort my records based on combo box
{
    updateRecordsListOrder();
    updateMyRecords();
}


void MainWindow::updateTagsList(){ // Update the edit and sort tag lists
    ui->myRecordEditTagsList->clear();
    ui->myRecordSortTagsList->clear();
    for (int i = 0; i < tags.size(); i++){
        ui->myRecordEditTagsList->addItem(new QListWidgetItem(tags.at(i)));
        if (tagsSort.at(i)){
            ui->myRecordSortTagsList->addItem(new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tags.at(i)));
        }
        else ui->myRecordSortTagsList->addItem(new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tags.at(i)));
    }
}

void MainWindow::on_myRecordEditTagsList_itemClicked(QListWidgetItem *item) // Add or remove a tag from a record
{
    if (selectedMyRecord){
        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).getCover().compare(recordsList.at(ui->myRecordTable->currentRow()).getCover()) == 0){
                // If selected record in list matches record in allMyRecords
                std::vector<QString> recordTags = allMyRecords.at(i).getTags();
                if (allMyRecords.at(i).addTag(item->text()) == 1){
                    allMyRecords.at(i).removeTag(item->text());
                }
                if (recordsList.at(ui->myRecordTable->currentRow()).addTag(item->text()) == 1){
                    recordsList.at(ui->myRecordTable->currentRow()).removeTag(item->text());
                }
                break;
            }
        }
        updateMyRecordsInfo(); // Update only the text info in list
        ui->myRecordEditTagsList->setCurrentRow(-1);
        json.writeRecords(&allMyRecords); // Save new tags to json
    }
}


void MainWindow::on_myRecordSortTagsList_itemClicked(QListWidgetItem *item)
{
    if (tagsSort.at(ui->myRecordSortTagsList->currentRow())){ // Tag is true, set to false
        QString tag = ui->myRecordSortTagsList->currentItem()->text();
        tagsSort.erase(tagsSort.begin()+ui->myRecordSortTagsList->currentRow());
        tagsSort.insert(tagsSort.begin()+ui->myRecordSortTagsList->currentRow(), false);
        ui->myRecordSortTagsList->takeItem(ui->myRecordSortTagsList->currentRow());
        ui->myRecordSortTagsList->insertItem(ui->myRecordSortTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tag));
        ui->myRecordSortTagsList->sortItems();
    }
    else { // Tag is false, set to true
        QString tag = ui->myRecordSortTagsList->currentItem()->text();
        tagsSort.erase(tagsSort.begin()+ui->myRecordSortTagsList->currentRow());
        tagsSort.insert(tagsSort.begin()+ui->myRecordSortTagsList->currentRow(), true);
        ui->myRecordSortTagsList->takeItem(ui->myRecordSortTagsList->currentRow());
        ui->myRecordSortTagsList->insertItem(ui->myRecordSortTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tag));
        ui->myRecordSortTagsList->sortItems();
    }

    ui->myRecordSortTagsList->setCurrentRow(-1); // Reset row selection

    updateRecordsListOrder();
    updateMyRecords();
}


void MainWindow::on_myRecordManageTagButton_clicked()
{
    tagsWindow* popup = new tagsWindow(&tags);
    std::vector<QString> saveTags = tags;
    std::vector<QString> sortShow;
    for (int i = 0; i < tagsSort.size(); i++){ // Get the tags that should still being showing after tags are added or removed
        if (tagsSort.at(i)){
            sortShow.push_back(tags.at(i));
        }
    }
    // Do after popup closes
    connect(popup, &QDialog::finished, this, [=]() {
        std::vector<QString> deletedTags;
        tagsSort.clear();
        for (int i = 0; i < tags.size(); i++){
            bool show = false;
            for (int j = 0; j < sortShow.size(); j++){
                if (tags.at(i).compare(sortShow.at(j)) == 0) show = true;
            }
            if (show) tagsSort.push_back(true);
            else tagsSort.push_back(false);
        }

        // Figure out what tags have been deleted
        for (QString oldTag : saveTags){
            bool deleted = true;
            for (QString newTag : tags){
                if (oldTag.compare(newTag) == 0){
                    deleted = false;
                    break;
                }
            }
            if (deleted) deletedTags.push_back(oldTag);
        }

        // Remove deleted tag(s) from records
        for (int i = 0; i < allMyRecords.size(); i++){
            for (QString deleteTag : deletedTags){
                allMyRecords.at(i).removeTag(deleteTag);
            }
        }

        updateTagsList();
        recordsList = allMyRecords;
        updateRecordsListOrder();
        if (!deletedTags.empty()) updateMyRecords();
    });

    popup->setModal(true);
    popup->exec();
}


void MainWindow::updateRecordsListOrder(){ // Set recordsList to have the correct records and order based on the options given (tags and sort)

    // Only show records with selected tags
    std::vector<Record> newRecordsList; // New vector to hold records than have required tags
    for (Record record : allMyRecords){
        bool matches = true;
        for (int i = 0; i < tagsSort.size(); i++){
            if (tagsSort.at(i) && !record.hasTag(tags.at(i))){ // Record should have tag but does not
                matches = false;
            }
        }
        if ((ui->myRecordSearchBar->text().isEmpty() && matches) || (record.contains(ui->myRecordSearchBar->text()) && matches)) { // Record has required tags and matches search bar
            newRecordsList.push_back(record);
        }
    }

    std::vector<Record> sortedRecords;

    // Show records in order according to dropdown
    switch (ui->myRecordSortBox->currentIndex()) {
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
                if (newRecordsList.at(littlest).getName().compare(newRecordsList.at(i).getName()) > 0){
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
                if (newRecordsList.at(littlest).getName().compare(newRecordsList.at(i).getName()) < 0){
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
                if (newRecordsList.at(littlest).getArtist().compare(newRecordsList.at(i).getArtist()) > 0){
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
                if (newRecordsList.at(littlest).getArtist().compare(newRecordsList.at(i).getArtist()) < 0){
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

QString MainWindow::downloadCover(QUrl imageUrl) {
    QString fileName = imageUrl.toString();
    for (int i = fileName.size()-1; i >= 0; i--) {
        if (fileName[i] == '/') {
            fileName.remove(0,i+1);
            break;
        }
    }
    QString savePath = dir.absolutePath() + "/resources/covers/" + fileName;

    QNetworkAccessManager manager;

    QNetworkRequest request(imageUrl);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();

        // Ensure savePath directory exists
        QFileInfo fileInfo(savePath);
        QDir().mkpath(fileInfo.path());

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(imageData);
            file.close();
            qDebug() << "Image saved to" << savePath;
        } else {
            qDebug() << "Error: Could not open file for writing";
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
    }

    reply->deleteLater();
    return fileName;
}

bool MainWindow::deleteCover(const QString& coverName) {
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
