#include "mainwindow.h"
#include "./ui_mainwindow.h"
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
#include "json.h"
#include "aboutwindow.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(QSize(1000, 810));
    setWindowTitle("My Record Collection");
    setFocus(this, Qt::ClickFocus); // Set all widgets to have click focus
    ui->myRecord_Table->setFocusPolicy(Qt::NoFocus); // Set tables to have NoFocus policy so there is no dotted line around the selected cell
    ui->searchRecord_Table->setFocusPolicy(Qt::NoFocus);

    QFont font("Arial");
    font.setPixelSize(14);

    // Fill the records vectors and tag vector
    allMyRecords = json.getRecords(0);
    for (Record& rec : allMyRecords){
        recordsList.push_back(&rec);
    }
    tags = json.getTags();
    prefs = json.getPrefs();
    sortTagsAlpha(&tags);

    // Set the style of the tables
    ui->myRecord_Table->setColumnWidth(0, 140);
    ui->myRecord_Table->setColumnWidth(1, 250);
    ui->myRecord_Table->setColumnWidth(2, 130);
    ui->myRecord_Table->setColumnWidth(3, 50);
    ui->myRecord_Table->setColumnWidth(4, 194);
    ui->myRecord_Table->verticalHeader()->hide();
    ui->myRecord_Table->setFont(font);

    ui->searchRecord_Table->setColumnWidth(0, 140);
    ui->searchRecord_Table->setColumnWidth(1, 385);
    ui->searchRecord_Table->setColumnWidth(2, 239);
    ui->searchRecord_Table->verticalHeader()->hide();
    ui->searchRecord_Table->setFont(font);

    // Set preferences
    ui->myRecord_SortBox->setCurrentIndex(prefs.getSort()); // Sort by rating decending
    updateRecordsListOrder();
    // Set the style sheet for the program
    setStyleSheet(prefs.getStyle());

    // Fill tables
    //QTimer::singleShot(1, [=](){ emit updateMyRecords(); }); // Fill my records table a second after program startup
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();

    // Start off with no record selected and unable to use some buttons
    ui->editRecord_EditTagsList->setDisabled(true);
    ui->editRecord_RatingSlider->setDisabled(true);
    ui->searchRecord_AddToMyRecordButton->setDisabled(true);
    ui->myRecord_Table->setCurrentCell(-1, -1);

    // Setup context menu for my records table
    myRecordContextMenu = new QMenu(this);
    QAction *contextActionEdit= new QAction("Edit", this);
    myRecordContextMenu->addAction(contextActionEdit);
    connect(contextActionEdit, &QAction::triggered, this, &MainWindow::toggleEditRecordFrame);
    QAction *contextActionRemove = new QAction("Remove", this);
    myRecordContextMenu->addAction(contextActionRemove);
    connect(contextActionRemove, &QAction::triggered, this, &MainWindow::removeTableRecord);
    connect(ui->myRecord_Table, &QTableWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);

    // Set editRecordsFrame options
    ui->editFrameBlockerFrame->setVisible(false);
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
    return pixmap.scaled(130, 130, Qt::IgnoreAspectRatio);
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
    results = json.searchRecords(ui->searchRecord_SearchBar->text(), 10);
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
        QTableWidgetItem *nameItem = new QTableWidgetItem(recordsList.at(recordNum)->getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(recordsList.at(recordNum)->getArtist());
        QTableWidgetItem *ratingItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum)->getRating()));

        QString tagString = "";
        for (QString tag : recordsList.at(recordNum)->getTags()) {
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
        QPixmap image(dir.absolutePath() + "/resources/user data/covers/" + recordsList.at(recordNum)->getCover());
        if (image.isNull()) image = QPixmap(dir.absolutePath() + "/resources/images/missingImg.jpg");
        image = image.scaled(130, 130, Qt::IgnoreAspectRatio);

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
        QTableWidgetItem *nameItem = new QTableWidgetItem(recordsList.at(recordNum)->getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(recordsList.at(recordNum)->getArtist());
        QTableWidgetItem *ratingItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum)->getRating()));

        QString tagString = "";
        for (QString tag : recordsList.at(recordNum)->getTags()) {
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
    for (Record record : allMyRecords){ // Check all my records to see if name matches requested add, possible duplicate
        if (record.getName().compare(results.at(ui->searchRecord_Table->currentRow()).getName()) == 0) {
            copy = true;
            break;
        }
    }
    ui->searchRecord_InfoLabel->setText("");
    Record addRecord = results.at(ui->searchRecord_Table->currentRow()); // Get the selected record to add
    addRecord.setCover(json.downloadCover(addRecord.getCover())); // Change the new records cover address from URL to file name
    addRecord.setRating(ui->searchRecord_RatingSlider->sliderPosition()); // Set records rating
    addRecord.setId(allMyRecords.size());

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
                json.writeTags(&tags);
            }
        }
    }

    updateTagCount();
    updateTagList();
    json.writeTags(&tags);

    allMyRecords.push_back(addRecord);
    on_myRecord_SearchBar_textChanged();
    updateMyRecordsTable();
    json.writeRecords(&allMyRecords);
    if (copy) ui->searchRecord_InfoLabel->setText("Added to My Collection\nRecord may be duplicate");
    else ui->searchRecord_InfoLabel->setText("Added to My Collection");
}


void MainWindow::removeTableRecord() // Remove record from my collection
{
    if (selectedMyRecord){
        json.deleteCover(recordsList.at(ui->myRecord_Table->currentRow())->getCover());
        qint64 deleteId = recordsList.at(ui->myRecord_Table->currentRow())->getId();

        for (int i = deleteId+1; i < allMyRecords.size(); i++){
            allMyRecords.at(i).setId(i-1);
        }

        allMyRecords.erase(allMyRecords.begin() + deleteId);
        recordsList.erase(recordsList.begin() + ui->myRecord_Table->currentRow());

        updateRecordsListOrder(); // Update the vector with the record collection table elements
        updateTagCount();
        updateTagList();
        updateMyRecordsTable();
        json.writeRecords(&allMyRecords); // Update saved records

        // Set record count label text
        ui->myRecord_RecordCountLabel->setText("Showing " + QString::number(recordsList.size()) + "/" + QString::number(allMyRecords.size()) + " Records");
    }
}


void MainWindow::on_myRecord_Table_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) // My collection record selected
{
    QFont font("Arial");
    font.setPixelSize(14);
    if (previousRow >= 0) { // If previous selection, set previous font back to regular
        ui->myRecord_Table->item(previousRow, 1)->setFont(font);
        ui->myRecord_Table->item(previousRow, 2)->setFont(font);
        ui->myRecord_Table->item(previousRow, 3)->setFont(font);
        ui->myRecord_Table->item(previousRow, 4)->setFont(font);
    }

    if (currentRow >= 0 && currentRow < recordsList.size()) { // Valid selection made of myRecords
        ui->editRecord_RatingSlider->setDisabled(false);
        ui->editRecord_RatingSlider->setSliderPosition(recordsList.at(ui->myRecord_Table->currentRow())->getRating());
        ui->editRecord_EditTagsList->setDisabled(false);
        selectedMyRecord = true;

        font.setBold(true); // Bold selection font
        ui->myRecord_Table->item(currentRow, 1)->setFont(font);
        ui->myRecord_Table->item(currentRow, 2)->setFont(font);
        ui->myRecord_Table->item(currentRow, 3)->setFont(font);
        ui->myRecord_Table->item(currentRow, 4)->setFont(font);

        // Set edit tags list checks
        ui->editRecord_EditTagsList->clear();
        QIcon checked(dir.absolutePath() + "/resources/images/check.png");
        QIcon unchecked(dir.absolutePath() + "/resources/images/uncheck.png");
        for (ListTag tag : tags){
            if (recordsList.at(ui->myRecord_Table->currentRow())->hasTag(tag.getName())){
                ui->editRecord_EditTagsList->addItem(new QListWidgetItem(checked, tag.getName()));
            }
            else {
                ui->editRecord_EditTagsList->addItem(new QListWidgetItem(unchecked, tag.getName()));
            }
        }

        // Update editRecordsFrame
        ui->editRecord_ArtistEdit->setText(recordsList.at(ui->myRecord_Table->currentRow())->getArtist());
        ui->editRecord_TitleEdit->setText(recordsList.at(ui->myRecord_Table->currentRow())->getName());

        QPixmap image(dir.absolutePath() + "/resources/user data/covers/" + recordsList.at(ui->myRecord_Table->currentRow())->getCover());
        if (image.isNull()) image = QPixmap(dir.absolutePath() + "/resources/images/missingImg.jpg");
        image = image.scaled(120, 120, Qt::IgnoreAspectRatio);
        ui->editRecord_CoverLabel->setPixmap(image);
    }
    else { // Invalid selection made of myRecords
        ui->editRecord_RatingSlider->setDisabled(true);
        ui->editRecord_EditTagsList->setDisabled(true);
        selectedMyRecord = false;
    }
    if (recordsList.empty()) { // myRecords list empty
        ui->editRecord_RatingSlider->setDisabled(true);
        ui->editRecord_EditTagsList->setDisabled(true);
        selectedMyRecord = false;
    }
}


void MainWindow::on_searchRecord_Table_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) // Search records page record selcted
{
    if (currentRow >= 0 || currentRow < results.size()) ui->searchRecord_AddToMyRecordButton->setDisabled(false); // Valid selection made of searchRecords
    else ui->searchRecord_AddToMyRecordButton->setDisabled(true); // Invalid selection made
    if (results.empty()) ui->searchRecord_AddToMyRecordButton->setDisabled(true); // Search list is empty
}


void MainWindow::on_myRecord_SearchBar_textChanged() // Search my records
{
    recordsList.clear();
    if (ui->myRecord_SearchBar->text().isEmpty()){
        for (Record& rec : allMyRecords){
            recordsList.push_back(&rec);
        }
    }
    else {
        for (int i = 0; i < allMyRecords.size(); i++){
            if (allMyRecords.at(i).contains(ui->myRecord_SearchBar->text())) {
                recordsList.push_back(&allMyRecords[i]);
            }
        }
    }
    on_myRecord_SortBox_activated(prefs.getSort());
}


void MainWindow::on_myRecord_SortBox_activated(int index) // Sort my records based on combo box
{
    prefs.setSort(index);
    json.writePrefs(&prefs);
    updateRecordsListOrder();
    updateMyRecordsTable();
}


void MainWindow::updateTagList(){ // Update the edit and sort tag lists
    ui->editRecord_EditTagsList->clear();
    ui->myRecord_FilterTagsList->clear();
    QIcon checked(dir.absolutePath() + "/resources/images/check.png");
    QIcon unchecked(dir.absolutePath() + "/resources/images/uncheck.png");
    for (ListTag tag : tags){
        ui->editRecord_EditTagsList->addItem(new QListWidgetItem(unchecked, tag.getName()));
        if (tag.getChecked()){
            ui->myRecord_FilterTagsList->addItem(new QListWidgetItem(checked, tag.getName() + " (" + QString::number(tag.getCount()) + ")"));
        }
        else {
            ui->myRecord_FilterTagsList->addItem(new QListWidgetItem(unchecked, tag.getName() + " (" + QString::number(tag.getCount()) + ")"));
        }
    }
    ui->myRecord_Table->setCurrentCell(-1, 0);
}


void MainWindow::on_editRecord_EditTagsList_itemClicked(QListWidgetItem *item) // Add or remove a tag from a record
{
    if (selectedMyRecord){
        int tableRow = ui->myRecord_Table->currentRow();
        int id = recordsList[tableRow]->getId();
        std::vector<QString> recordTags = allMyRecords.at(id).getTags();
        int tagRowSave = ui->editRecord_EditTagsList->currentRow();
        if (allMyRecords[id].addTag(item->text()) == 1){ // Try to add tag to record
            allMyRecords[id].removeTag(item->text()); // if record already has tag, REMOVE TAG
            ui->editRecord_EditTagsList->takeItem(tagRowSave);
            ui->editRecord_EditTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), item->text()));
            ui->editRecord_EditTagsList->sortItems();
            tags.at(tagRowSave).decCount();
            recordsList.at(ui->myRecord_Table->currentRow())->removeTag(item->text()); // Remove tag from record object
        }
        else { // Adding tag, set the check box to checked
            ui->editRecord_EditTagsList->takeItem(tagRowSave);
            ui->editRecord_EditTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), item->text()));
            ui->editRecord_EditTagsList->sortItems();
            tags.at(tagRowSave).incCount();
            recordsList.at(ui->myRecord_Table->currentRow())->addTag(item->text()); // Add tag to record object
        }
        ui->editRecord_EditTagsList->setCurrentRow(tagRowSave); // Set the edit tags list row back to what it was

        // Update the filter tags list number
        ui->myRecord_FilterTagsList->takeItem(tagRowSave); // Remove the filter item to be readded
        if (tags.at(tagRowSave).getChecked()) { // Add tag that is being filtered
            ui->myRecord_FilterTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tags.at(tagRowSave).getName() + " (" + QString::number(tags.at(tagRowSave).getCount()) + ")"));
        }
        else { // Add a tag that is not being filtered
            ui->myRecord_FilterTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tags.at(tagRowSave).getName() + " (" + QString::number(tags.at(tagRowSave).getCount()) + ")"));
        }
        ui->myRecord_FilterTagsList->setCurrentRow(-1); // Set filter record to not have anything selected
        ui->myRecord_Table->setCurrentCell(tableRow, 0); // Set the record table back to the current record
    }
}


void MainWindow::on_myRecord_FilterTagsList_itemClicked(QListWidgetItem *item) // Sort my records by a tag (sort tag list clicked)
{
    QString tag = ui->myRecord_FilterTagsList->currentItem()->text();
    if (tags.at(ui->myRecord_FilterTagsList->currentRow()).toggleCheck()){ // Tag was unchecked, set to checked
        ui->myRecord_FilterTagsList->takeItem(ui->myRecord_FilterTagsList->currentRow()); // Remove tag list item
        ui->myRecord_FilterTagsList->insertItem(ui->myRecord_FilterTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/check.png"), tag)); // Add tag back as checked
        ui->myRecord_FilterTagsList->sortItems();
    }
    else { // Tag is unchecked, set to checked
        ui->myRecord_FilterTagsList->takeItem(ui->myRecord_FilterTagsList->currentRow());
        ui->myRecord_FilterTagsList->insertItem(ui->myRecord_FilterTagsList->currentRow(), new QListWidgetItem(QIcon(dir.absolutePath() + "/resources/images/uncheck.png"), tag));
        ui->myRecord_FilterTagsList->sortItems();
    }

    ui->myRecord_FilterTagsList->setCurrentRow(-1); // Reset row selection

    updateRecordsListOrder();
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}


void MainWindow::on_editRecord_ManageTagButton_clicked() // Open and handle manage tags popup
{
    TagsWindow* popup = new TagsWindow(&tags, &prefs);
    std::vector<ListTag> saveTags = tags;
    int savedTableRow = ui->myRecord_Table->currentRow();

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

        recordsList.clear();
        for (Record& rec : allMyRecords){
            recordsList.push_back(&rec);
        }
        updateRecordsListOrder();
        updateTagList();
        if (!deletedTags.empty()) { // A tag was deleted
            updateMyRecordsTable();
            json.writeRecords(&allMyRecords);
        }
        ui->myRecord_Table->setCurrentCell(savedTableRow, 0); // Reset table selection to what it was before
    });

    popup->setModal(true);
    popup->exec();
}


void MainWindow::updateRecordsListOrder(){ // Set recordsList to have the correct records and order based on the options given (tags and sort) (Does not visably update the table)

    // Only show records with selected tags
    std::vector<Record*> newRecordsList; // New vector to hold records than have required tags
    for (Record& record : allMyRecords){
        bool matches = true;
        for (int i = 0; i < tags.size(); i++){
            if (tags.at(i).getChecked() && !record.hasTag(tags.at(i).getName())){ // Record should have tag but does not
                matches = false;
            }
        }
        if ((ui->myRecord_SearchBar->text().isEmpty() && matches) || (record.contains(ui->myRecord_SearchBar->text()) && matches)) { // Record has required tags and matches search bar
            newRecordsList.push_back(&record);
        }
    }

    std::vector<Record*> sortedRecords;

    // Show records in order according to dropdown
    switch (ui->myRecord_SortBox->currentIndex()) {
    case 0: // Oldest to newest
        recordsList.clear();
        for (Record *rec : newRecordsList){
            recordsList.push_back(rec);
        }
        break;
    case 1: // Newest to oldest
        std::reverse(newRecordsList.begin(),newRecordsList.end());
        recordsList.clear();
        for (Record *rec : newRecordsList){
            recordsList.push_back(rec);
        }
        break;
    case 2: // Title ascending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest)->getName().toLower().compare(newRecordsList.at(i)->getName().toLower()) > 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList.clear();
        for (Record *rec : sortedRecords){
            recordsList.push_back(rec);
        }
        break;
    case 3: // Title descending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest)->getName().toLower().compare(newRecordsList.at(i)->getName().toLower()) < 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList.clear();
        for (Record *rec : sortedRecords){
            recordsList.push_back(rec);
        }
        break;
    case 4: // Artist ascending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest)->getArtist().toLower().compare(newRecordsList.at(i)->getArtist().toLower()) > 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList.clear();
        for (Record *rec : sortedRecords){
            recordsList.push_back(rec);
        }
        break;
    case 5: // Artist descending
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (newRecordsList.at(littlest)->getArtist().toLower().compare(newRecordsList.at(i)->getArtist().toLower()) < 0){
                    littlest = i;
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList.clear();
        for (Record *rec : sortedRecords){
            recordsList.push_back(rec);
        }
        break;
    case 6: // Rating ascending
        for (int i = 0; i <= 10; i++){
            for (Record *record : newRecordsList){
                if (record->getRating() == i) sortedRecords.push_back(record);
            }
        }
        recordsList.clear();
        for (Record *rec : sortedRecords){
            recordsList.push_back(rec);
        }
        break;
    case 7: // Rating ascending
        for (int i = 10; i >= 0; i--){
            for (Record *record : newRecordsList){
                if (record->getRating() == i) sortedRecords.push_back(record);
            }
        }
        recordsList.clear();
        for (Record *rec : sortedRecords){
            recordsList.push_back(rec);
        }
        break;
    }
}


void MainWindow::on_searchRecord_Table_cellClicked(int row, int column) // Click on search records table, show suggested tags
{
    ui->searchRecord_SuggestedTagsList->clear();
    suggestedTags.clear();
    suggestedTags = json.wikiTags(results.at(ui->searchRecord_Table->currentRow()).getName(), results.at(ui->searchRecord_Table->currentRow()).getArtist());
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


void MainWindow::sortTagsAlpha(std::vector<ListTag> *list, bool delDups) { // Sort a vector of ListTag objects alphabetically
    std::vector<QString> names;
    std::vector<ListTag> copy = *list;

    for (ListTag tag : *list){
        names.push_back(tag.getName());
    }
    std::sort(names.begin(), names.end());

    if (delDups) names.erase(unique(names.begin(), names.end()), names.end());

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
    if (ui->myRecord_Table->rowCount()>0) {
        int randRow = rand() % recordsList.size();
        ui->myRecord_Table->setCurrentCell(randRow, 0);
    }
}


void MainWindow::on_settings_actionExit_triggered() // Exit program menu button pressed
{
    qApp->quit();
}


void MainWindow::on_settings_actionToggleTheme_triggered() // Toggle theme menu button clicked
{
    prefs.toggleTheme(); // Update the theme in the preferences object
    setStyleSheet(prefs.getStyle()); // Set the style
    json.writePrefs(&prefs); // Update the prefs json
}


void MainWindow::on_actionSelect_File_and_Import_triggered() // Import discogs file menu button clicked
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Discogs Collection"), "/", tr("CSV files (*.csv)")); // Open file selector popup

    // Import discogs threaded
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
                        if (ui->importDiscogsAddTagsOpt->isChecked()) imports.push_back(new ImportDiscogs(line, &allMyRecords, true)); // Create and add ImportDiscogs class to vector, add suggested tags
                        else imports.push_back(new ImportDiscogs(line, &allMyRecords, false)); // Dont add suggested tags to record
                        threads.push_back(new QThread); // Create a QThread for record
                        imports.at(tot)->moveToThread(threads.at(tot)); // Move the ImportDiscogs to its own thread

                        // Connect for started and finished signals
                        QObject::connect(threads.at(tot), &QThread::started, imports.at(tot), &ImportDiscogs::run); // Run import method when thread starts
                        QObject::connect(imports.at(tot), &ImportDiscogs::finished, threads.at(tot), &QThread::quit); // When ImportDiscogs run finished, quit QThread
                        QObject::connect(threads.at(tot), &QThread::finished, threads.at(tot), &QThread::deleteLater); // When thread finished, delete thread

                        threads.at(tot)->start(); // Start thread
                        line = myFile.readLine(); // Get next discogs record
                        tot++;
                    }
                    for (int i = 0; i < tot; i++){ // Get results from each record
                        while (!imports.at(i)->isDone()); // Loop main until import is finished
                        Record *importedRec = imports.at(i)->getProcessedRec(); // Get the new Record
                        if (!(importedRec->getName().compare("") == 0 & importedRec->getArtist().compare("") == 0 && importedRec->getCover().compare("") == 0 && importedRec->getRating() == -1)){
                            allMyRecords.push_back(*imports.at(i)->getProcessedRec()); // Record is new, add to collection, don't add duplicates
                            for (QString tag : imports.at(i)->getProcessedRec()->getTags()){
                                tags.push_back(ListTag(tag));
                            }
                        }
                        imports.at(i)->deleteLater(); // Delete object
                    }
                    sortTagsAlpha(&tags, true);
                    json.writeTags(&tags);
                    json.writeRecords(&allMyRecords); // Write changes to json
                }
            }
        }
    }

    updateRecordsListOrder(); // Update the vector with the record collection table elements
    updateTagCount();
    updateTagList();
    updateMyRecordsTable(); // Visably update the record collection table*/
}


void MainWindow::on_actionDelete_All_User_Data_triggered() // Delete all user data menu button clicked
{
    QMessageBox msgBox;
    msgBox.setStyleSheet(prefs.getMessageStyle());
    msgBox.setText("Are you sure you want to erase all user data?");
    msgBox.setInformativeText("This action cannot be undone.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowIcon(QIcon(dir.absolutePath() + "/resources/images/appico.ico"));
    msgBox.setWindowTitle("Erase User Data?");
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes){
        for (Record rec : allMyRecords) {
            json.deleteCover(rec.getCover());
        }
        allMyRecords.clear();
        tags.clear();
        updateRecordsListOrder(); // Update tables
        updateMyRecordsTable();
        ui->editRecord_EditTagsList->clear();
        ui->myRecord_FilterTagsList->clear();
        json.deleteUserData();
    }
}


void MainWindow::on_myRecord_ResetTagsFilterButton_clicked() // "Reset" tags filter button clicked
{
    for (int i = 0; i < tags.size(); i++){
        tags.at(i).setChecked(false);
    }
    updateRecordsListOrder();
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}


void MainWindow::on_help_actionAbout_triggered() // About menu button clicked
{
    AboutWindow* popup = new AboutWindow(&prefs);
    popup->setWindowIcon(QIcon(dir.absolutePath() + "/resources/images/appico.ico"));
    popup->setModal(true);
    popup->exec();
}


void MainWindow::on_help_actionContact_triggered() // Contact menu button clicked
{
    QMessageBox msgBox;
    msgBox.setStyleSheet(prefs.getMessageStyle());
    msgBox.setText("Thank you for using My Record Collection! If you have any feedback or ideas for the app, please email me at MyRecordCollectionContact@gmail.com.");
    msgBox.setStandardButtons(QMessageBox::Close);
    msgBox.setDefaultButton(QMessageBox::Close);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowIcon(QIcon(dir.absolutePath() + "/resources/images/appico.ico"));
    msgBox.setWindowTitle("Contact - My Record Collection");
    msgBox.exec();
}


void MainWindow::on_importDiscogsAddTagsOpt_triggered() // Add tags to discogs import menu button clicked
{
    ui->menuSettings->show(); // Keep the menu showing
    ui->menuImport_Discogs_Collection->show();
}


void MainWindow::updateTagCount(){ // Update the counts for all TagList objects
    for (int i = 0; i < tags.size(); i++){ // For each tag
        tags.at(i).setCount(0); // Reset count
        for (Record *record : recordsList){ // For each record, compare record tags and add to count if matching ListTag
            if (record->hasTag(tags.at(i).getName())) tags.at(i).incCount();
        }
    }
}


void MainWindow::showContextMenu(const QPoint &pos) // Show context menu when right clicking a record in My Collection
{
    QPoint globalPos = ui->myRecord_Table->viewport()->mapToGlobal(pos);

    if (ui->myRecord_Table->itemAt(pos)) {
        myRecordContextMenu->exec(globalPos);
    }
}

void MainWindow::setFocus(QWidget* parent, enum Qt::FocusPolicy focus) { // Recursively set all widgets to have a specific focus policy
    if (!parent) return;

    const QObjectList& children = parent->children();
    for (QObject* child : children) {
        QWidget* childWidget = qobject_cast<QWidget*>(child);
        if (childWidget) {
            childWidget->setFocusPolicy(focus);
            setFocus(childWidget, focus); // Recursively set
        }
    }
}

void MainWindow::toggleEditRecordFrame() // Toggle view of edit record frame
{
    if (ui->editRecordFrame->isVisible()) {
        ui->editFrameBlockerFrame->setVisible(false);
        ui->myRecord_Table->setFocus(); // So that "Done" button cannot be pressed while it is not on screen
        setFocus(ui->editRecordFrame, Qt::ClickFocus); // Set everything back to click focus
    }
    else {
        ui->editFrameBlockerFrame->setVisible(true);
        ui->editRecord_TitleEdit->setFocusPolicy(Qt::StrongFocus); // Set these to strong focus so tab works
        ui->editRecord_ArtistEdit->setFocusPolicy(Qt::StrongFocus);
        ui->editRecord_EditTagsList->setFocusPolicy(Qt::StrongFocus);
        ui->editRecord_ManageTagButton->setFocusPolicy(Qt::StrongFocus);
        ui->editRecord_RatingSlider->setFocusPolicy(Qt::StrongFocus);
        ui->editRecord_TitleEdit->setFocus(); // So that arrow keys cannot change selection on My Record table
    }
}

void MainWindow::on_editRecord_DoneButton_clicked() // Close edit record frame
{
    ui->editFrameBlockerFrame->setVisible(false);
    ui->myRecord_Table->setFocus(); // Prevent done button still being focused
    setFocus(ui->editRecordFrame, Qt::ClickFocus);
    qint64 id = recordsList.at(ui->myRecord_Table->currentRow())->getId(); // id of record being edited

    // Update records info with new info
    allMyRecords[id].setName(ui->editRecord_TitleEdit->text());
    allMyRecords[id].setArtist(ui->editRecord_ArtistEdit->text());
    allMyRecords[id].setRating(ui->editRecord_RatingSlider->value());

    updateRecordsListOrder(); // Update record table order
    updateMyRecordsTable(); // and info
    json.writeRecords(&allMyRecords); // Save changes to json
    for (int i = 0; i < recordsList.size(); i++){ // Set table record selected to same record as was selected before
        if (recordsList[i]->getId() == id){
            ui->myRecord_Table->setCurrentCell(i, 0);
            break;
        }
    }
}

void MainWindow::on_myRecord_customRecordButton_clicked() // Create a custom record
{
    allMyRecords.push_back(Record("Custom Record", "", "", 0, allMyRecords.size())); // Add new record to allMyRecords vector
    updateRecordsListOrder();
    updateMyRecordsTable();
    json.writeRecords(&allMyRecords);
    for (int i = 0; i < recordsList.size(); i++){ // Set table to same record that was selected
        if (recordsList[i]->getId() == recordsList.size()-1){
            ui->myRecord_Table->setCurrentCell(i, 0);
            break;
        }
    }
}


void MainWindow::on_editRecord_ArtistEdit_returnPressed() // album artist edit line edit return pressed
{
    on_editRecord_DoneButton_clicked();
}


void MainWindow::on_editRecord_TitleEdit_returnPressed() // album title edit line edit return pressed
{
    on_editRecord_DoneButton_clicked();
}


void MainWindow::on_editRecord_CoverEdit_clicked()
{
    QFileInfo fileInfo = QFileInfo(QFileDialog::getOpenFileName(this, tr("Change Record Cover"), "/", tr("Image files (*.jpg *.jpeg *.png)"))); // Open file selector popup

    if (fileInfo.isFile()) { // If file is chosen...
        int fileNum = 0; // Keep track of number of duplicate names
        if (QFile::copy(fileInfo.absoluteFilePath(), dir.absolutePath() + "/resources/user data/covers/" + fileInfo.fileName())); // If file is copied, all good
        else { // If file cannot be copied, add (1), (2)... until it can be
            while (!QFile::copy(fileInfo.absoluteFilePath(), dir.absolutePath() + "/resources/user data/covers/" + fileInfo.baseName() + " (" + QString::number(++fileNum) + ")." + fileInfo.suffix()));
        }

        qint64 id = recordsList.at(ui->myRecord_Table->currentRow())->getId(); // id of record being edited
        json.deleteCover(allMyRecords[id].getCover()); // Delete old cover
        if (fileNum == 0) { // Set record object to have new cover file
            allMyRecords[id].setCover(fileInfo.fileName());
        }
        else {
            allMyRecords[id].setCover(fileInfo.baseName() + " (" + QString::number(fileNum) + ")." + fileInfo.suffix());
        }

        // Update the preview cover image
        QPixmap image(dir.absolutePath() + "/resources/user data/covers/" + allMyRecords[id].getCover());
        if (image.isNull()) image = QPixmap(dir.absolutePath() + "/resources/images/missingImg.jpg");
        image = image.scaled(120, 120, Qt::IgnoreAspectRatio);
        ui->editRecord_CoverLabel->setPixmap(image);
    }
}

