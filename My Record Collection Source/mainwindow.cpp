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
#include <QHBoxLayout>
#include "sizechangefilter.h"
#include "threadedcover.h"
#include <QMovie>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("My Record Collection");
    setFocus(this, Qt::ClickFocus); // Set all widgets to have click focus
    ui->myRecord_Table->setFocusPolicy(Qt::NoFocus); // Set tables to have NoFocus policy so there is no dotted line around the selected cell
    ui->searchRecord_Table->setFocusPolicy(Qt::NoFocus);

    QFont font("Arial");
    font.setPixelSize(14);

    // Fill the records vectors and tag vector
    allMyRecords = json.getRecords(0, QCoreApplication::applicationDirPath() + "/resources/user data/records.json", false);
    for (Record& rec : allMyRecords){
        recordsList.push_back(&rec);
    }
    tags = json.getTags(QCoreApplication::applicationDirPath() + "/resources/user data/tags.json");
    prefs = json.getPrefs();
    sortTagsAlpha(&tags);

    // Set the style of the tables
    ui->myRecord_Table->verticalHeader()->hide();
    ui->myRecord_Table->setFont(font);

    ui->searchRecord_Table->verticalHeader()->hide();
    ui->searchRecord_Table->setFont(font);

    // Setup sorting combobox with icons for ascending and descending
    if (prefs.getAsc()){
        ui->myRecord_SortBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"), "Ascending");
        ui->myRecord_SortBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), "Descending");
    }
    else {
        ui->myRecord_SortBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), "Ascending");
        ui->myRecord_SortBox->addItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"), "Descending");
    }

    // Set preferences
    ui->myRecord_SortBox->setCurrentIndex(prefs.getSort()); // Sort by rating decending
    updateRecordsListOrder();
    setStyleSheet(prefs.getStyle()); // Set the style sheet
    // Set window preferences
    ui->actionCover->setChecked(prefs.getCover());
    ui->actionRating->setChecked(prefs.getRating());
    ui->actionRelease->setChecked(prefs.getRelease());
    ui->actionAdded_Date->setChecked(prefs.getAdded());
    this->resize(prefs.getSize());

    // Fill tables
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
    setupContextMenuActions(); // Setup context menu
    connect(ui->myRecord_Table, &QTableWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);

    // Set editRecordsFrame options
    ui->editFrameBlockerFrame->setVisible(false);

    // Setup the progress bar to be hiden
    ui->myRecord_DiscogsProgressBar->setVisible(false);
    connect(ui->myRecord_DiscogsProgressBar, &QProgressBar::valueChanged, this, &MainWindow::onProgressBarValueChanged);

    // Set release filter to min/max values
    ui->myRecord_FilterReleaseMaxSpinBox->setValue(releaseMax);
    ui->myRecord_FilterReleaseMinSpinBox->setValue(releaseMin);

    // Setup window resizing
    this->setMinimumSize(1000, 600);
    SizeChangeFilter* sizeFilter = new SizeChangeFilter;
    this->installEventFilter(sizeFilter);
    connect(sizeFilter, &SizeChangeFilter::newSize, this, [=](QSize size) { // When window resizes, move and resize widgets
        // Set record page geometry X Y Width Height
        ui->pages->setGeometry(0, -10, size.width(), size.height()-10);
        ui->myRecord_PickForMe->setGeometry(size.width()-360, size.height()-52, ui->myRecord_PickForMe->width(), ui->myRecord_PickForMe->height());
        ui->myRecord_RecordCountLabel->setGeometry(ui->myRecord_RecordCountLabel->x(), size.height()-52, ui->myRecord_RecordCountLabel->width(), ui->myRecord_RecordCountLabel->height());
        ui->myRecord_Table->setGeometry(ui->myRecord_Table->x(), ui->myRecord_Table->y(), size.width()-219, size.height()-110);
        ui->myRecord_ToSearchRecordsButton->setGeometry(size.width()-200, size.height()-52, ui->myRecord_ToSearchRecordsButton->width(), ui->myRecord_ToSearchRecordsButton->height());
        ui->myRecord_customRecordButton->setGeometry(size.width()-200, size.height()-90, ui->myRecord_customRecordButton->width(), ui->myRecord_customRecordButton->height());
        ui->myRecord_RightAlignedFrame->setGeometry(size.width()-ui->myRecord_RightAlignedFrame->width(), ui->myRecord_RightAlignedFrame->y(), ui->myRecord_RightAlignedFrame->width(), ui->myRecord_RightAlignedFrame->height());

        // Resize my record table columns, 17px for scrollbar
        // 252 for other things
        resizeRecordTable();

        // Set search page geometry
        ui->searchRecord_InfoLabel->setGeometry(size.width()-200, ui->searchRecord_InfoLabel->y(), ui->searchRecord_InfoLabel->width(), ui->searchRecord_InfoLabel->height());
        ui->searchRecord_Table->setGeometry(ui->searchRecord_Table->x(), ui->searchRecord_Table->y(), size.width()-219, size.height()-110);
        ui->searchRecord_ToMyRecordsButton->setGeometry(size.width()-200, size.height()-52, ui->searchRecord_ToMyRecordsButton->width(), ui->searchRecord_ToMyRecordsButton->height());
        ui->searchRecord_RightAlignedFrame->setGeometry(size.width()-ui->searchRecord_RightAlignedFrame->width(), ui->searchRecord_RightAlignedFrame->y(), ui->searchRecord_RightAlignedFrame->width(), ui->searchRecord_RightAlignedFrame->height());

        // Resize search record table columns, 17px for scrollbar
        ui->searchRecord_Table->setColumnWidth(0, 135);
        int leftOverSearch = ui->searchRecord_Table->width() - 17 - ui->searchRecord_Table->columnWidth(0);
        ui->searchRecord_Table->setColumnWidth(1, leftOverSearch * 0.62);
        leftOverSearch -= ui->searchRecord_Table->columnWidth(1);
        ui->searchRecord_Table->setColumnWidth(2, leftOverSearch);

        // Set edit popup geometry
        ui->editFrameBlockerFrame->setGeometry(0, 0, size.width(), size.height());
        ui->editRecordFrame->setGeometry(size.width()/2-ui->editRecordFrame->width()/2, size.height()/2-ui->editRecordFrame->height()/2, ui->editRecordFrame->width(), ui->editRecordFrame->height());

        prefs.setSize(size);
        json.writePrefs(&prefs);
    });

    // Starting loading symbol movie
    loadingSymbol = new QMovie(QCoreApplication::applicationDirPath() + "/resources/images/loading.gif");
    loadingSymbol->setScaledSize(QSize(90, 90));
    loadingSymbol->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_searchRecord_ToMyRecordsButton_clicked() // change screen to my records
{
    ui->pages->setCurrentIndex(0);
}


void MainWindow::on_myRecord_ToSearchRecordsButton_clicked() // Change screen to search records
{
    ui->pages->setCurrentIndex(1);
    ui->searchRecord_InfoLabel->setText("");
}


void MainWindow::on_searchRecord_SearchBar_returnPressed() // Search last.fm records
{
    // Reset page
    ui->searchRecord_InfoLabel->setText("");
    ui->searchRecord_Table->clear();
    ui->searchRecord_SuggestedTagsList->clear();
    suggestedTags.clear();
    ui->searchRecord_ReleaseEdit->setValue(1900);
    ui->searchRecord_AddToMyRecordButton->setEnabled(false);
    ui->searchRecord_Table->setHorizontalHeaderLabels({"Cover", "Record", "Artist"});

    for (ThreadedCover *coverThread : searchThreadedCovers) { // Set previous ThreadedCover objects to not emit finished flag
        if (coverThread) coverThread->setStopFlag(true);
    }
    searchThreadedCovers.clear(); // clear all threadedcover objects from previous search

    if (ui->searchRecord_SearchBar->text().isEmpty()) { // No search text, do nothing
        ui->searchRecord_Table->setRowCount(0);
        return;
    }

    // Get search results and setup table
    results = json.searchRecords(ui->searchRecord_SearchBar->text(), ui->searchRecord_ResultsSpin->value());
    ui->searchRecord_Table->setRowCount(results.size());

    if (!results.empty() && results[0].getRating() == -1) { // Network error with search records
        QTableWidgetItem *nameItem = new QTableWidgetItem("Network Error");
        ui->searchRecord_Table->setItem(0, 1, nameItem);
        ui->searchRecord_Table->setRowHeight(0, 140);
    }
    else { // No network error, search record returned
        ui->searchRecord_Table->setEnabled(true);
        for (int recordNum = 0; recordNum < results.size(); recordNum++){ // Insert record's text info into table
            QTableWidgetItem *nameItem = new QTableWidgetItem(results.at(recordNum).getName());
            QTableWidgetItem *artistItem = new QTableWidgetItem(results.at(recordNum).getArtist());

            ui->searchRecord_Table->setItem(recordNum, 1, nameItem);
            ui->searchRecord_Table->setItem(recordNum, 2, artistItem);
            ui->searchRecord_Table->setRowHeight(recordNum, 140);
        }

        for (int recordNum = 0; recordNum < results.size(); recordNum++){ // Insert record's cover into table
            searchThreadedCovers.push_back(new ThreadedCover(results.at(recordNum).getCover(), recordNum)); // Create ThreadedCover objects
            QThread *thread = new QThread; // Create a thread
            searchThreadedCovers[recordNum]->moveToThread(thread); // Move object to thread

            // Add loading gif to table
            // Create label for loading gif and add gif
            QLabel *label = new QLabel();
            label->setMovie(loadingSymbol);
            label->setAlignment(Qt::AlignCenter);

            // Create a container widget with a layout
            QWidget *containerWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(containerWidget);
            layout->addWidget(label);
            layout->setAlignment(label, Qt::AlignCenter); // Align the QLabel right
            layout->setContentsMargins(0, 0, 5, 0); // Remove margins with 5px on right still

            // Set the container widget as the cell widget
            ui->searchRecord_Table->setCellWidget(recordNum, 0, containerWidget);

            // Setup thread connections for starting and ending
            connect(thread, &QThread::started, searchThreadedCovers[recordNum], &ThreadedCover::run);
            connect(searchThreadedCovers[recordNum], &ThreadedCover::finished, this, &MainWindow::handleCoverThreadFinished);
            connect(searchThreadedCovers[recordNum], &ThreadedCover::finished, thread, &QThread::quit);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);

            thread->start(); // Start thread
        }
    }
}


void MainWindow::handleCoverThreadFinished(QPixmap pixmap, int pos) {
    pixmap = pixmap.scaled(120, 120, Qt::IgnoreAspectRatio); // Scale pixmap

    // Create label for pixmap and add cover
    QLabel *label = new QLabel();
    label->setPixmap(pixmap);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Create a container widget with a layout
    QWidget *containerWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(containerWidget);
    layout->addWidget(label);
    layout->setAlignment(label, Qt::AlignRight); // Align the QLabel right
    layout->setContentsMargins(0, 0, 5, 0); // Remove margins with 5px on right still

    // Set the container widget as the cell widget
    ui->searchRecord_Table->setCellWidget(pos, 0, containerWidget);
}


void MainWindow::updateMyRecordsTable(){ // Update my records list
    ui->myRecord_Table->clear();
    ui->myRecord_Table->setHorizontalHeaderLabels({"Cover", "Record", "Artist", "Rating", "Tags", "Release", "Date Added"});
    ui->myRecord_Table->setRowCount(recordsList.size());

    // Figure out max and min release years
    releaseMin = 9999;
    releaseMax = 1;
    for (Record &rec : allMyRecords) {
        if (rec.getRelease() > releaseMax) releaseMax = rec.getRelease();
        if (rec.getRelease() < releaseMin) releaseMin = rec.getRelease();
    }

    if (releaseMax < releaseMin) { // If max is still less than min, no records exist
        ui->myRecord_FilterReleaseMaxSpinBox->setValue(9999);
        ui->myRecord_FilterReleaseMinSpinBox->setValue(1);
    }

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){ // Insert record's text info into table
        QTableWidgetItem *nameItem = new QTableWidgetItem(recordsList.at(recordNum)->getName());
        QTableWidgetItem *artistItem = new QTableWidgetItem(recordsList.at(recordNum)->getArtist());
        QTableWidgetItem *ratingItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum)->getRating()));
        ratingItem->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *releaseItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum)->getRelease()));
        releaseItem->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *addedItem = new QTableWidgetItem(recordsList.at(recordNum)->getAdded().toString("yyyy-MM-dd"));
        addedItem->setTextAlignment(Qt::AlignCenter);

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
        ui->myRecord_Table->setItem(recordNum, 5, releaseItem);
        ui->myRecord_Table->setItem(recordNum, 6, addedItem);
        ui->myRecord_Table->setRowHeight(recordNum, 140);
    }

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){ // Insert records covers into table
        QPixmap image(QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + recordsList.at(recordNum)->getCover());
        if (image.isNull()) image = QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/missingImg.jpg");
        image = image.scaled(120, 120, Qt::IgnoreAspectRatio);

        QLabel *label = new QLabel();
        label->setPixmap(image);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Create a container widget with a layout
        QWidget *containerWidget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(containerWidget);
        layout->addWidget(label);
        layout->setAlignment(label, Qt::AlignRight); // Align the QLabel right
        layout->setContentsMargins(0, 0, 5, 0); // Remove margins with 5px on right still

        // Set the container widget as the cell widget
        ui->myRecord_Table->setCellWidget(recordNum, 0, containerWidget);
    }

    // Set record count label text
    ui->myRecord_RecordCountLabel->setText("Showing " + QString::number(recordsList.size()) + "/" + QString::number(allMyRecords.size()) + " Records");
}


void MainWindow::updateMyRecordsInfo(){ // Update my records list (not images) DEPRECIATED (need to update cover set)

    for (int recordNum = 0; recordNum < recordsList.size(); recordNum++){
        QTableWidgetItem *nameItem = new QTableWidgetItem(recordsList.at(recordNum)->getName());
        QTableWidgetItem *releaseItem = new QTableWidgetItem(recordsList.at(recordNum)->getRelease());
        QTableWidgetItem *artistItem = new QTableWidgetItem(recordsList.at(recordNum)->getArtist());
        QTableWidgetItem *ratingItem = new QTableWidgetItem(QString::number(recordsList.at(recordNum)->getRating()));

        QString tagString = "";
        for (QString tag : recordsList.at(recordNum)->getTags()) {
            if (tagString.isEmpty()) tagString += tag;
            else tagString += ", " + tag;
        }

        QTableWidgetItem *tagsItem = new QTableWidgetItem(tagString);

        ui->myRecord_Table->setItem(recordNum, 1, releaseItem);
        ui->myRecord_Table->setItem(recordNum, 2, nameItem);
        ui->myRecord_Table->setItem(recordNum, 3, artistItem);
        ui->myRecord_Table->setItem(recordNum, 4, ratingItem);
        ui->myRecord_Table->setItem(recordNum, 5, tagsItem);
    }
}


void MainWindow::on_searchRecord_AddToMyRecordButton_clicked() // Add searched record to my collection
{
    if (ui->searchRecord_Table->currentRow() < 0) return; // If no records in list, do not do anything
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

    addRecord.setRelease(ui->searchRecord_ReleaseEdit->value()); // Get release year from QSpinBox

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

    allMyRecords.push_back(addRecord); // Add and write data
    json.writeRecords(&allMyRecords);
    json.writeTags(&tags);

    // If adding a record and not sorting my records by release, change release filter to include new record
    if (addRecord.getRelease() > releaseMax && !isSortingByRelease()) ui->myRecord_FilterReleaseMaxSpinBox->setValue(addRecord.getRelease()); // Change max spin box
    else if (addRecord.getRelease() < releaseMin && !isSortingByRelease()) ui->myRecord_FilterReleaseMinSpinBox->setValue(addRecord.getRelease()); // Change min spin box
    else { // As changing spin box would update the table already, only need to do if not changing either spin box
        updateRecordsListOrder(); // Update record table order
    }

    updateTagCount();
    updateTagList();
    updateMyRecordsTable();

    if (copy) ui->searchRecord_InfoLabel->setText("Added to My Collection\nRecord may be duplicate");
    else ui->searchRecord_InfoLabel->setText("Added to My Collection");
}


void MainWindow::removeTableRecord() // Remove record from my collection
{
    if (selectedRec){
        json.deleteCover(selectedRec->getCover());

        for (int i = selectedRec->getId()+1; i < allMyRecords.size(); i++){
            allMyRecords.at(i).setId(i-1);
        }

        allMyRecords.erase(allMyRecords.begin() + selectedRec->getId());
        recordsList.erase(recordsList.begin() + ui->myRecord_Table->currentRow());

        selectedRec = nullptr;
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
        selectedRec = recordsList[currentRow];
        font.setBold(true); // Bold selection font
        ui->myRecord_Table->item(currentRow, 1)->setFont(font);
        ui->myRecord_Table->item(currentRow, 2)->setFont(font);
        ui->myRecord_Table->item(currentRow, 3)->setFont(font);
        ui->myRecord_Table->item(currentRow, 4)->setFont(font);

        updateEditPopup();
    }
    else { // Invalid selection made of myRecords
        ui->editRecord_RatingSlider->setDisabled(true);
        ui->editRecord_EditTagsList->setDisabled(true);
        selectedRec = NULL;
    }
    if (recordsList.empty()) { // myRecords list empty
        ui->editRecord_RatingSlider->setDisabled(true);
        ui->editRecord_EditTagsList->setDisabled(true);
        selectedRec = NULL;
    }
}

void MainWindow::updateEditPopup(){
    ui->editRecord_RatingSlider->setDisabled(false);
    ui->editRecord_RatingSlider->setSliderPosition(selectedRec->getRating());
    ui->editRecord_EditTagsList->setDisabled(false);

    // Set edit tags list checks
    ui->editRecord_EditTagsList->clear();
    QIcon checked(QCoreApplication::applicationDirPath() + "/resources/images/check.png");
    QIcon unchecked(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png");
    for (ListTag tag : tags){
        if (selectedRec->hasTag(tag.getName())){
            ui->editRecord_EditTagsList->addItem(new QListWidgetItem(checked, tag.getName()));
        }
        else {
            ui->editRecord_EditTagsList->addItem(new QListWidgetItem(unchecked, tag.getName()));
        }
    }

    // Update editRecordsFrame
    ui->editRecord_ArtistEdit->setText(selectedRec->getArtist());
    ui->editRecord_TitleEdit->setText(selectedRec->getName());
    ui->editRecord_ReleaseEdit->setValue(selectedRec->getRelease());
    ui->editRecord_AddedEdit->setDate(selectedRec->getAdded());

    QPixmap image(QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + selectedRec->getCover());
    if (image.isNull()) image = QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/missingImg.jpg");
    image = image.scaled(115, 115, Qt::IgnoreAspectRatio);
    ui->editRecord_CoverLabel->setPixmap(image);
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
    updateRecordsListOrder();
    updateMyRecordsTable();
}


void MainWindow::on_myRecord_SortBox_activated(int index) // Sort my records based on combo box
{
    if (ui->myRecord_SortBox->currentText().compare("Ascending") == 0) {
        ui->myRecord_SortBox->setCurrentIndex(prefs.getSort());
        prefs.setAsc(true);
        ui->myRecord_SortBox->setItemIcon(index, QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"));
        ui->myRecord_SortBox->setItemIcon(index+1, QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"));
    }
    else if (ui->myRecord_SortBox->currentText().compare("Descending") == 0){
        ui->myRecord_SortBox->setCurrentIndex(prefs.getSort());
        prefs.setAsc(false);
        ui->myRecord_SortBox->setItemIcon(index-1, QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"));
        ui->myRecord_SortBox->setItemIcon(index, QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"));
    }
    else {
        prefs.setSort(index);
    }
    updateRecordsListOrder();
    updateMyRecordsTable();
    json.writePrefs(&prefs);
}


void MainWindow::updateTagList(){ // Update the edit and sort tag lists
    ui->editRecord_EditTagsList->clear();
    ui->myRecord_FilterTagsList->clear();
    QIcon checked(QCoreApplication::applicationDirPath() + "/resources/images/check.png");
    QIcon unchecked(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png");
    for (ListTag tag : tags){
        if (selectedRec && selectedRec->hasTag(tag.getName())) {
            ui->editRecord_EditTagsList->addItem(new QListWidgetItem(checked, tag.getName()));
        }
        else {
            ui->editRecord_EditTagsList->addItem(new QListWidgetItem(unchecked, tag.getName()));
        }
        if (tag.getChecked()){
            ui->myRecord_FilterTagsList->addItem(new QListWidgetItem(checked, tag.getName() + " (" + QString::number(tag.getCount()) + ")"));
        }
        else {
            ui->myRecord_FilterTagsList->addItem(new QListWidgetItem(unchecked, tag.getName() + " (" + QString::number(tag.getCount()) + ")"));
        }
    }
    //ui->myRecord_Table->setCurrentCell(-1, 0); Not entirely sure if this is important but it was breaking the add suggested tags feature 0on the edit popup
}


void MainWindow::on_editRecord_EditTagsList_itemClicked(QListWidgetItem *item) // Add or remove a tag from a record
{
    if (selectedRec){
        int tableRow = ui->myRecord_Table->currentRow();
        std::vector<QString> recordTags = selectedRec->getTags();
        int tagRowSave = ui->editRecord_EditTagsList->currentRow();
        if (selectedRec->addTag(item->text()) == 1){ // Try to add tag to record
            selectedRec->removeTag(item->text()); // if record already has tag, REMOVE TAG
            ui->editRecord_EditTagsList->takeItem(tagRowSave);
            ui->editRecord_EditTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), item->text()));
            ui->editRecord_EditTagsList->sortItems();
            tags.at(tagRowSave).decCount();
            selectedRec->removeTag(item->text()); // Remove tag from record object
        }
        else { // Adding tag, set the check box to checked
            ui->editRecord_EditTagsList->takeItem(tagRowSave);
            ui->editRecord_EditTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"), item->text()));
            ui->editRecord_EditTagsList->sortItems();
            tags.at(tagRowSave).incCount();
            selectedRec->addTag(item->text()); // Add tag to record object
        }
        ui->editRecord_EditTagsList->setCurrentRow(tagRowSave); // Set the edit tags list row back to what it was

        // Update the filter tags list number
        ui->myRecord_FilterTagsList->takeItem(tagRowSave); // Remove the filter item to be readded
        if (tags.at(tagRowSave).getChecked()) { // Add tag that is being filtered
            ui->myRecord_FilterTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"), tags.at(tagRowSave).getName() + " (" + QString::number(tags.at(tagRowSave).getCount()) + ")"));
        }
        else { // Add a tag that is not being filtered
            ui->myRecord_FilterTagsList->insertItem(tagRowSave, new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), tags.at(tagRowSave).getName() + " (" + QString::number(tags.at(tagRowSave).getCount()) + ")"));
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
        ui->myRecord_FilterTagsList->insertItem(ui->myRecord_FilterTagsList->currentRow(), new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"), tag)); // Add tag back as checked
        ui->myRecord_FilterTagsList->sortItems();
    }
    else { // Tag is unchecked, set to checked
        ui->myRecord_FilterTagsList->takeItem(ui->myRecord_FilterTagsList->currentRow());
        ui->myRecord_FilterTagsList->insertItem(ui->myRecord_FilterTagsList->currentRow(), new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), tag));
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
    std::vector<Record*> newRecordsList; // New vector to hold records that have required tags
    for (Record& record : allMyRecords){
        bool matches = true;

        for (int i = 0; i < tags.size(); i++){
            if (tags.at(i).getChecked() && !record.hasTag(tags.at(i).getName())){ // Record should have tag but does not
                matches = false;
            }
        }
        if (record.getRating() > ui->myRecord_FilterRatingMaxSpinBox->value() || record.getRating() < ui->myRecord_FilterRatingMinSpinBox->value()) continue; // If rating is not in range of rating filter, skip
        if (record.getRelease() > ui->myRecord_FilterReleaseMaxSpinBox->value() || record.getRelease() < ui->myRecord_FilterReleaseMinSpinBox->value()) continue; // If release is not in range of release filter, skip
        if ((ui->myRecord_SearchBar->text().isEmpty() && matches) || (record.contains(ui->myRecord_SearchBar->text()) && matches)) { // Record has required tags and matches search bar
            newRecordsList.push_back(&record);
        }
    }

    std::vector<Record*> sortedRecords;

    // Show records in order according to dropdown
    switch (ui->myRecord_SortBox->currentIndex()) {
    case 0: // Date added
        if (prefs.getAsc()) {
            std::sort(newRecordsList.begin(), newRecordsList.end(), [](const Record* a, const Record* b) {
                return a->getAdded() < b->getAdded();
            });
        }
        else {
            std::sort(newRecordsList.begin(), newRecordsList.end(), [](const Record* a, const Record* b) {
                return a->getAdded() > b->getAdded();
            });
        }
        recordsList = newRecordsList;
        break;
    case 1: // Title
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (prefs.getAsc()) {
                    if (newRecordsList.at(littlest)->getName().toLower().compare(newRecordsList.at(i)->getName().toLower()) > 0){
                        littlest = i;
                    }
                }
                else {
                    if (newRecordsList.at(littlest)->getName().toLower().compare(newRecordsList.at(i)->getName().toLower()) < 0){
                        littlest = i;
                    }
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList = sortedRecords;
        break;
    case 2: // Artist
        while (!newRecordsList.empty()){
            int littlest = 0;

            for (int i = 0; i < newRecordsList.size(); i++){
                if (prefs.getAsc()) {
                    if (newRecordsList.at(littlest)->getArtist().toLower().compare(newRecordsList.at(i)->getArtist().toLower()) > 0){
                        littlest = i;
                    }
                }
                else {
                    if (newRecordsList.at(littlest)->getArtist().toLower().compare(newRecordsList.at(i)->getArtist().toLower()) < 0){
                        littlest = i;
                    }
                }
            }
            sortedRecords.push_back(newRecordsList.at(littlest));
            newRecordsList.erase(newRecordsList.begin()+littlest);
        }
        recordsList = sortedRecords;
        break;
    case 3: // Rating
        if (prefs.getAsc()) {
            std::sort(newRecordsList.begin(), newRecordsList.end(), [](const Record* a, const Record* b) {
                return a->getRating() < b->getRating();
            });
        }
        else {
            std::sort(newRecordsList.begin(), newRecordsList.end(), [](const Record* a, const Record* b) {
                return a->getRating() > b->getRating();
            });
        }
        recordsList = newRecordsList;
        break;
    case 4: // Release
        if (prefs.getAsc()) {
            std::sort(newRecordsList.begin(), newRecordsList.end(), [](const Record* a, const Record* b) {
                return a->getRelease() < b->getRelease();
            });
        }
        else {
            std::sort(newRecordsList.begin(), newRecordsList.end(), [](const Record* a, const Record* b) {
                return a->getRelease() > b->getRelease();
            });
        }
        recordsList = newRecordsList;
        break;
    }
}


void MainWindow::on_searchRecord_Table_cellPressed(int row, int column) // Click on search records table, show suggested tags
{
    ui->searchRecord_SuggestedTagsList->clear();
    suggestedTags.clear();
    ui->searchRecord_SuggestedTagsList->setEnabled(false);
    ui->searchRecord_ReleaseEdit->setEnabled(false);
    ui->searchRecord_AddToMyRecordButton->setEnabled(false);
    ui->searchRecord_SuggestedTagsList->addItem(new QListWidgetItem("Loading tags"));
    ui->searchRecord_InfoLabel->setText("");
    suggestedTags = json.wikiTags(results.at(ui->searchRecord_Table->currentRow()).getName(), results.at(ui->searchRecord_Table->currentRow()).getArtist(), true);

    ui->searchRecord_SuggestedTagsList->clear();
    if (suggestedTags.empty()){
        ui->searchRecord_SuggestedTagsList->addItem(new QListWidgetItem("No tags found"));
    }
    else {
        if (suggestedTags[0].getName().toInt() != 0) {
            ui->searchRecord_ReleaseEdit->setValue(suggestedTags[0].getName().toInt());
        }
        else {
            ui->searchRecord_ReleaseEdit->setValue(1900);
            ui->searchRecord_InfoLabel->setText("Error fetching release year");
        }
        sortTagsAlpha(&suggestedTags);
        for (int i = 1; i < suggestedTags.size(); i++) {
            ui->searchRecord_SuggestedTagsList->addItem(new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), suggestedTags[i].getName()));
        }
        ui->searchRecord_SuggestedTagsList->setEnabled(true);
        ui->searchRecord_ReleaseEdit->setEnabled(true);
        ui->searchRecord_AddToMyRecordButton->setEnabled(true);
    }
}


void MainWindow::on_searchRecord_SuggestedTagsList_itemClicked(QListWidgetItem *item) // Click on suggested tags list
{
    if (suggestedTags.at(ui->searchRecord_SuggestedTagsList->currentRow()).getChecked()){ // Tag is checked, set to unchecked
        QString tag = ui->searchRecord_SuggestedTagsList->currentItem()->text();
        suggestedTags.at(ui->searchRecord_SuggestedTagsList->currentRow()).setChecked(false);
        ui->searchRecord_SuggestedTagsList->takeItem(ui->searchRecord_SuggestedTagsList->currentRow());
        ui->searchRecord_SuggestedTagsList->insertItem(ui->searchRecord_SuggestedTagsList->currentRow(), new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/uncheck.png"), tag));
    }
    else { // Tag is unchecked, set to checked
        QString tag = ui->searchRecord_SuggestedTagsList->currentItem()->text();
        suggestedTags.at(ui->searchRecord_SuggestedTagsList->currentRow()).setChecked(true);
        ui->searchRecord_SuggestedTagsList->takeItem(ui->searchRecord_SuggestedTagsList->currentRow());
        ui->searchRecord_SuggestedTagsList->insertItem(ui->searchRecord_SuggestedTagsList->currentRow(), new QListWidgetItem(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/check.png"), tag));
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
    setupContextMenuActions(); // Update colours of context menu icons
    json.writePrefs(&prefs); // Update the prefs json
}


void MainWindow::on_actionSelect_File_and_Import_triggered() // Import discogs file menu button clicked
{
    if (ui->editRecordFrame->isVisible()) {
        toggleEditRecordFrame();
    }
    QString filePath = QFileDialog::getOpenFileName(this, tr("Import Discogs Collection"), "/", tr("CSV files (*.csv)")); // Get discogs file

    if (!filePath.isEmpty()) { // File is selected
        ui->pages->setCurrentIndex(0); // Change to my records page
        QFile myFile(filePath);
        if (myFile.exists() && myFile.open(QIODevice::ReadOnly)) { // Open the file
            totalImports = 0; // Reset/initialize two vars
            totalSkipped = 0;
            ui->myRecord_InfoLabel->setText("Importing Discogs collection\n\n\nSkipped: " + QString::number(totalSkipped) + " / ..."); // Setup the gui to show progress
            ui->myRecord_DiscogsProgressBar->setValue(0);
            ui->myRecord_DiscogsProgressBar->setVisible(true);
            this->repaint();
            QString line = myFile.readLine(); // Read in the first line
            if (!line.startsWith("Catalog#,Artist,Title")) { // Make sure it is what it should be, if not, stop
                myFile.close();
                std::cerr << "Invalid collection file" << std::endl;
                ui->myRecord_InfoLabel->setText("Invalid Discogs Collection");
                setInfoTimed(ui->myRecord_InfoLabel, 5000, "");
            } else { // File is a discogs csv
                line = myFile.readLine(); // Get first record
                finishedImportsCount = 0;
                while (!line.isEmpty()) {
                    ImportDiscogs *importer = new ImportDiscogs(line, &allMyRecords, ui->importDiscogsAddTagsOpt->isChecked(), ui->importDiscogsAddAddedOpt->isChecked(), ui->actionDiscogsSkipDuplicates->isChecked()); // Create importer with proper options

                    QThread *thread = new QThread; // Create a thread and move object to thread
                    importer->moveToThread(thread);

                    // Setup signals and slots
                    connect(thread, &QThread::started, importer, &ImportDiscogs::run);
                    connect(importer, &ImportDiscogs::finished, this, &MainWindow::handleImportFinished);
                    connect(importer, &ImportDiscogs::finished, thread, &QThread::quit);
                    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
                    connect(thread, &QThread::finished, importer, &ImportDiscogs::deleteLater);

                    thread->start(); // Start thread
                    line = myFile.readLine(); // Get next record
                    totalImports++;
                }

                ui->myRecord_InfoLabel->setText("Importing Discogs collection\n\n\nSkipped: " + QString::number(totalSkipped) + " / " + QString::number(totalImports)); // Update the progress label with total import number
                ui->myRecord_DiscogsProgressBar->setMaximum(totalImports);
            }
        }
    }
}


void MainWindow::handleImportFinished(Record *importedRec, bool skipped)
{
    if (importedRec && !(importedRec->getName().isEmpty() && importedRec->getArtist().isEmpty() && importedRec->getCover().isEmpty() && importedRec->getRating() == -1)) {
        importedRec->setId(allMyRecords.size()); // Record is new, add to collection, don't add duplicates
        allMyRecords.push_back(*importedRec);
        for (const QString &tag : importedRec->getTags()) {
            tags.push_back(ListTag(tag));
        }
    }

    // Update vars and progress ui
    finishedImportsCount++;
    if (skipped) totalSkipped++;
    ui->myRecord_InfoLabel->setText("Importing Discogs collection\n\n\nSkipped: " + QString::number(totalSkipped) + " / " + QString::number(totalImports));
    ui->myRecord_DiscogsProgressBar->setValue(finishedImportsCount);
    repaint();  // Force UI update

    if (finishedImportsCount == totalImports) { // Once all records have been processed
        sortTagsAlpha(&tags, true);
        json.writeTags(&tags); // Write tags and records to json
        json.writeRecords(&allMyRecords);
        ui->myRecord_InfoLabel->setText("Import completed\n\n\nSkipped: " + QString::number(totalSkipped) + " / " + QString::number(totalImports));

        // Update tags and lists
        updateRecordsListOrder();
        updateTagCount();
        updateTagList();
        updateMyRecordsTable();

        setInfoTimed(ui->myRecord_InfoLabel, 5000, "");
    }
}


void MainWindow::on_myRecord_ResetFiltersButton_clicked() // "Reset" tags filter button clicked
{
    for (int i = 0; i < tags.size(); i++){
        tags.at(i).setChecked(false);
    }
    ui->myRecord_FilterRatingMaxSpinBox->setValue(10);
    ui->myRecord_FilterRatingMinSpinBox->setValue(0);
    ui->myRecord_FilterReleaseMaxSpinBox->setValue(releaseMax);
    ui->myRecord_FilterReleaseMinSpinBox->setValue(releaseMin);
    updateRecordsListOrder();
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}


void MainWindow::on_help_actionAbout_triggered() // About menu button clicked
{
    AboutWindow* popup = new AboutWindow(&prefs);
    popup->setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/appico.ico"));
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
    msgBox.setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/appico.ico"));
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
    if (ui->editRecordFrame->isVisible()) { // Hide edit menu
        ui->editFrameBlockerFrame->setVisible(false);
        ui->myRecord_Table->setFocus(); // So that "Done" button cannot be pressed while it is not on screen
        setFocus(ui->editRecordFrame, Qt::ClickFocus); // Set everything back to click focus
    }
    else {
        ui->editFrameBlockerFrame->setVisible(true); // Show edit menu
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
    // Update records info with new info
    selectedRec->setName(ui->editRecord_TitleEdit->text());
    selectedRec->setArtist(ui->editRecord_ArtistEdit->text());
    selectedRec->setRating(ui->editRecord_RatingSlider->value());
    selectedRec->setRelease(ui->editRecord_ReleaseEdit->value());
    selectedRec->setAdded(ui->editRecord_AddedEdit->date());

    Record* savedSelected = selectedRec;

    // If new release is less than min or greater than max and filter is filtering all, change filter in include
    if (ui->editRecord_ReleaseEdit->value() > releaseMax && !isSortingByRelease()) ui->myRecord_FilterReleaseMaxSpinBox->setValue(ui->editRecord_ReleaseEdit->value()); // Change max spin box
    else if (ui->editRecord_ReleaseEdit->value() < releaseMin && !isSortingByRelease()) ui->myRecord_FilterReleaseMinSpinBox->setValue(ui->editRecord_ReleaseEdit->value()); // Change min spin box
    else { // As changing spin box would update the table already, only need to do if not changing either spin box
        updateRecordsListOrder(); // Update record table order
        updateMyRecordsTable(); // and info
    }
    json.writeRecords(&allMyRecords); // Save changes to json

    ui->editFrameBlockerFrame->setVisible(false);
    ui->myRecord_Table->setFocus(); // Prevent done button still being focused
    setFocus(ui->editRecordFrame, Qt::ClickFocus);

    selectedRec = savedSelected;
    for (int i = 0; i < recordsList.size(); i++){ // Set table record selected to same record as was selected before
        if (recordsList[i]->getId() == selectedRec->getId()){
            ui->myRecord_Table->setCurrentCell(i, 0);
            break;
        }
    }
}

void MainWindow::on_myRecord_customRecordButton_clicked() // Create a custom record
{
    allMyRecords.push_back(Record("Custom Record", "", "", 0, allMyRecords.size(), 1900, QDate::currentDate())); // Add new record to allMyRecords vector
    selectedRec = &allMyRecords[allMyRecords.size()-1];
    updateEditPopup();
    toggleEditRecordFrame();
}


void MainWindow::on_editRecord_ArtistEdit_returnPressed() // album artist edit line edit return pressed
{
    on_editRecord_DoneButton_clicked();
}


void MainWindow::on_editRecord_TitleEdit_returnPressed() // album title edit line edit return pressed
{
    on_editRecord_DoneButton_clicked();
}


void MainWindow::on_editRecord_CoverEdit_clicked() // Change record cover on edit popup
{
    QFileInfo fileInfo = QFileInfo(QFileDialog::getOpenFileName(this, tr("Change Record Cover"), "/", tr("Image files (*.jpg *.jpeg *.png)"))); // Open file selector popup

    if (fileInfo.isFile()) { // If file is chosen...
        int fileNum = 0; // Keep track of number of duplicate names
        if (QFile::copy(fileInfo.absoluteFilePath(), QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + fileInfo.fileName())); // If file is copied, all good
        else { // If file cannot be copied, add (1), (2)... until it can be
            while (!QFile::copy(fileInfo.absoluteFilePath(), QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + fileInfo.baseName() + " (" + QString::number(++fileNum) + ")." + fileInfo.suffix()));
        }

        json.deleteCover(selectedRec->getCover()); // Delete old cover
        if (fileNum == 0) { // Set record object to have new cover file
            selectedRec->setCover(fileInfo.fileName());
        }
        else {
            selectedRec->setCover(fileInfo.baseName() + " (" + QString::number(fileNum) + ")." + fileInfo.suffix());
        }

        // Update the preview cover image
        QPixmap image(QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + selectedRec->getCover());
        if (image.isNull()) image = QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/missingImg.jpg");
        image = image.scaled(120, 120, Qt::IgnoreAspectRatio);
        ui->editRecord_CoverLabel->setPixmap(image);
    }
}

void MainWindow::onProgressBarValueChanged(int value) // When the progress bar gets updated
{
    value = (float)value/(float)totalImports*100.0; // Turn value into a percentage of the completed records
    int radius = qBound(1, value, 14); // Get the radius that the chunk of the progress bar should be, unto 14
    if (value < 15) radius--; // If its less than 15, subtraact it by one to avoid random square chunks
    QString styleSheet = QString( // Update the progress bars style
                             "QProgressBar::chunk {"
                             "    border-radius: %1px;"
                             "}"
                             ).arg(radius);

    ui->myRecord_DiscogsProgressBar->setStyleSheet(styleSheet);
}


void MainWindow::on_actionDelete_All_User_Data_triggered() // Delete all user data clicked
{
    deleteUserData(true, true);
}


void MainWindow::on_actionDelete_All_Records_triggered() // Delete all records clicked
{
    deleteUserData(true, false);
}


void MainWindow::on_actionDelete_All_Tags_triggered() // Delete all tags clicked
{
    deleteUserData(false, true);
}


void MainWindow::deleteUserData(bool delRecords, bool delTags){ // Delete records and/or tags
    if (ui->editRecordFrame->isVisible()) {
        toggleEditRecordFrame();
    }
    QMessageBox msgBox;
    msgBox.setStyleSheet(prefs.getMessageStyle());
    if (delRecords) {
        if (delTags) {
            msgBox.setText("Are you sure you want to erase all user data?");
        } else {
            msgBox.setText("Are you sure you want to erase all records?");
        }
    } else {
        msgBox.setText("Are you sure you want to erase all tags?");
    }
    msgBox.setInformativeText("This action cannot be undone.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/appico.ico"));
    msgBox.setWindowTitle("Erase User Data?");
    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes){
        if (delRecords) {
            QDir dir(QCoreApplication::applicationDirPath() + "/resources/user data/covers/");

            if (!dir.exists()) {
                qWarning() << "Record cover directory does not exist";
            }

            // Get list of all files in the directory
            QStringList files = dir.entryList(QDir::Files);

            foreach (QString file, files) {
                QString filePath = dir.absoluteFilePath(file);
                QFile::remove(filePath);
            }
            allMyRecords.clear();
            recordsList.clear();
            if (!delTags) {
                updateTagCount();
                updateTagList();
            }
        }
        if (delTags) {
            tags.clear();
            ui->editRecord_EditTagsList->clear();
            ui->myRecord_FilterTagsList->clear();
            for (Record &rec : allMyRecords){
                rec.removeAllTags();
            }
            if (!delRecords) json.writeRecords(&allMyRecords);
        }
        updateRecordsListOrder(); // Update tables
        updateMyRecordsTable();
        json.deleteUserData(delRecords, delTags);
    }
}

void MainWindow::on_myRecord_FilterRatingMaxSpinBox_valueChanged(int arg1) // Filter rating max box changed
{
    if (arg1 < ui->myRecord_FilterRatingMinSpinBox->value()) {
        ui->myRecord_FilterRatingMaxSpinBox->setValue(ui->myRecord_FilterRatingMinSpinBox->value());
    }
    updateRecordsListOrder(); // Update tables
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}


void MainWindow::on_myRecord_FilterRatingMinSpinBox_valueChanged(int arg1) // Filter rating min box changed
{
    if (arg1 > ui->myRecord_FilterRatingMaxSpinBox->value()) {
        ui->myRecord_FilterRatingMinSpinBox->setValue(ui->myRecord_FilterRatingMaxSpinBox->value());
    }
    updateRecordsListOrder(); // Update tables
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}

void MainWindow::on_actionExport_MRC_Collection_triggered() // Export MRC record collection to folder
{
    QString sourceDir = QCoreApplication::applicationDirPath() + "/resources/user data";
    QString destinationDir = QFileDialog::getExistingDirectory(this, tr("Select Destination Folder"), QDir::homePath());

    if (destinationDir.isEmpty()) {
        return; // User canceled the dialog
    }

    if (copyDirectory(sourceDir, destinationDir)) { // Export successful
        ui->myRecord_InfoLabel->setText("Collection exported successfully");
    } else { // Export not successful
        ui->myRecord_InfoLabel->setText("Error exporting collection");
    }
    setInfoTimed(ui->myRecord_InfoLabel, 5000, "");
}

// Function to recursively copy a directory
bool MainWindow::copyDirectory(const QString &sourceDir, const QString &destinationDir) { // Helper method, copy directory to another
    QDir dir(sourceDir);
    if (!dir.exists()) {
        qDebug() << "Source directory does not exist:" << sourceDir;
        return false;
    }

    QDir destDir(destinationDir);
    if (!destDir.exists()) { // Create directory if it doesn't exist
        destDir.mkpath(destinationDir);
    }

    foreach (QString file, dir.entryList(QDir::Files)) { // Copy each file
        QString srcName = sourceDir + "/" + file;
        QString destName = destinationDir + "/" + file;
        if (!QFile::copy(srcName, destName)) {
            qDebug() << "Failed to copy file:" << srcName << "to" << destName;
            return false;
        }
    }

    foreach (QString subDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) { // Copy each directory (recursively)
        QString srcName = sourceDir + "/" + subDir;
        QString destName = destinationDir + "/" + subDir;
        if (!copyDirectory(srcName, destName)) {
            qDebug() << "Failed to copy directory:" << srcName << "to" << destName;
            return false;
        }
    }

    return true;
}

void MainWindow::on_actionImport_MRC_Collection_triggered() // Import a MRC record collection
{
    if (ui->editRecordFrame->isVisible()) { // Close edit frame if its open
        toggleEditRecordFrame();
    }
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Import MRC Collection"), QDir::homePath()); // Get folder from user

    if (dirPath.isEmpty()) {
        return; // User canceled the dialog
    }

    ui->pages->setCurrentIndex(0); // Change to my records page
    QDir dir(dirPath); // Make sure directory has all required files, if not, give error message
    if (!dir.exists("records.json") && !dir.exists("tags.json") && !dir.exists("covers")) {
        std::cerr << "Could not find required files for import" << std::endl;
        ui->myRecord_InfoLabel->setText("Could not find import files.\nPlease select folder that\ncontains \'records.json\',\n\'tags.json\', and \'covers\' folder.");
        setInfoTimed(ui->myRecord_InfoLabel, 7000, "");
        return;
    }

    std::vector<Record> newRecs = json.getRecords(allMyRecords.size(), dirPath + "/records.json", true); // Get vector of imported records
    QDir coverDir(QCoreApplication::applicationDirPath() + "/covers"); // Get directory of record covers

    QDir destCoversDir(QCoreApplication::applicationDirPath() + "/resources/user data/covers");
    if (!destCoversDir.exists()) { // Create covers directory if it doesn't exist
        destCoversDir.mkpath(QCoreApplication::applicationDirPath() + "/resources/user data/covers");
    }

    for (int i = 0; i < coverDir.entryList(QDir::Files).size(); i++) { // Copy each cover file to resoruces
        QString file = coverDir.entryList(QDir::Files)[i];
        QString srcName = coverDir.absolutePath() + "/" + file;
        QFileInfo destFile(QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + file);
        int fileNum = 0; // Keep track of number of duplicate names
        if (QFile::copy(srcName, destFile.filePath())); // If file is copied, all good
        else { // If file cannot be copied, add (1), (2)... until it can be
            while (!QFile::copy(srcName, QCoreApplication::applicationDirPath() + "/resources/user data/covers/" + destFile.baseName() + " (" + QString::number(++fileNum) + ")." + destFile.suffix()));
            for (Record &rec : newRecs) {
                if (rec.getCover().compare(destFile.fileName()) == 0) { // Rename cover in record object if it already exists in resources
                    rec.setCover(destFile.baseName() + " (" + QString::number(fileNum) + ")." + destFile.suffix());
                    break;
                }
            }
        }
    }

    std::vector<ListTag> newTags = json.getTags(dirPath + "/tags.json"); // Get vector of all imported tags

    allMyRecords.insert(allMyRecords.end(), newRecs.begin(), newRecs.end()); // Add new records to allmyrecords vector
    json.writeRecords(&allMyRecords); // Save records to json

    tags.insert(tags.end(), newTags.begin(), newTags.end()); // Add new tags to tags vector
    sortTagsAlpha(&tags, true); // Sort tags by alpha and delete duplicates
    json.writeTags(&tags); // Save tags to json

    ui->myRecord_InfoLabel->setText("Imported " + QString::number(newRecs.size()) + " records from\nMRC collection.");
    setInfoTimed(ui->myRecord_InfoLabel, 5000, "");

    updateRecordsListOrder(); // Update tables
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}

void MainWindow::setInfoTimed(QLabel *label, int ms, QString text) { // Hide info based on ms
    // Hide the progress ui after specified ms
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        label->setText(text);
        if (label == ui->myRecord_InfoLabel) ui->myRecord_DiscogsProgressBar->setVisible(false); // Hide progress bar if myRecord page progress label
        timer->deleteLater();
    });
    timer->start(ms);
}

void MainWindow::on_editRecord_ReleaseEdit_valueChanged(int arg1) // Enter 0 in edit record release year, get release from Wikipedia
{
    if (arg1 == 0) {
        ui->editRecord_ReleaseInfoLabel->setText("Searching for\nrelease year");
        int release = json.wikiRelease(ui->editRecord_TitleEdit->text(), ui->editRecord_ArtistEdit->text()); // Get release year
        if (release == 0) { // If year 0 is returned, error
            release = 1900;
            ui->editRecord_ReleaseInfoLabel->setText("Error fetching\nrelease year");
        }
        else if (release < 0) {
            release *= -1;
            ui->editRecord_ReleaseInfoLabel->setText("Found a release year\nwith low confidence");
        }
        else { // Release found
            ui->editRecord_ReleaseInfoLabel->setText("Found a release year");
        }
        ui->editRecord_ReleaseEdit->setValue(release); // Set release year in edit box
        setInfoTimed(ui->editRecord_ReleaseInfoLabel, 3500, "Tip: Enter '0' to get\nestimated release");
        ui->editRecord_ReleaseEdit->selectAll();
    }
}


void MainWindow::on_myRecord_FilterReleaseMinSpinBox_valueChanged(int arg1) // Filter release min box changed
{
    if (arg1 > ui->myRecord_FilterReleaseMaxSpinBox->value()) {
        ui->myRecord_FilterReleaseMinSpinBox->setValue(ui->myRecord_FilterReleaseMaxSpinBox->value());
    }
    updateRecordsListOrder(); // Update tables
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}


void MainWindow::on_myRecord_FilterReleaseMaxSpinBox_valueChanged(int arg1) // Filter release max box changed
{
    if (arg1 < ui->myRecord_FilterReleaseMinSpinBox->value()) {
        ui->myRecord_FilterReleaseMaxSpinBox->setValue(ui->myRecord_FilterReleaseMinSpinBox->value());
    }
    updateRecordsListOrder(); // Update tables
    updateTagCount();
    updateTagList();
    updateMyRecordsTable();
}

void MainWindow::resizeRecordTable()
{
    if (ui->actionCover->isChecked()) ui->myRecord_Table->setColumnWidth(0, 135); // Cover
    else ui->myRecord_Table->setColumnWidth(0, 0);
    if (ui->actionRating->isChecked()) ui->myRecord_Table->setColumnWidth(3, 40); // Rating
    else ui->myRecord_Table->setColumnWidth(3, 0);
    if (ui->actionRelease->isChecked()) ui->myRecord_Table->setColumnWidth(5, 60); // Release
    else ui->myRecord_Table->setColumnWidth(5, 0);
    if (ui->actionAdded_Date->isChecked()) ui->myRecord_Table->setColumnWidth(6, 100); // Added
    else ui->myRecord_Table->setColumnWidth(6, 0);
    int leftOverMy = ui->myRecord_Table->width() - 17 - ui->myRecord_Table->columnWidth(0) - ui->myRecord_Table->columnWidth(3) - ui->myRecord_Table->columnWidth(5) - ui->myRecord_Table->columnWidth(6);
    ui->myRecord_Table->setColumnWidth(1, leftOverMy * 0.41); // Record/name, was 220
    ui->myRecord_Table->setColumnWidth(2, ui->myRecord_Table->columnWidth(1)/2); // Artist, was 110
    ui->myRecord_Table->setColumnWidth(4, leftOverMy - ui->myRecord_Table->columnWidth(1) - ui->myRecord_Table->columnWidth(2)); // Tags, was 199

}
void MainWindow::on_actionCover_triggered() // Toggle view of "cover" column in my collection table
{
    resizeRecordTable();
    prefs.setCover(ui->actionCover->isChecked());
    json.writePrefs(&prefs);
}


void MainWindow::on_actionRating_triggered() // Toggle view of "rating" column in my collection table
{
    resizeRecordTable();
    prefs.setRating(ui->actionRating->isChecked());
    json.writePrefs(&prefs);
}


void MainWindow::on_actionRelease_triggered() // Toggle view of "release" column in my collection table
{
    resizeRecordTable();
    prefs.setRelease(ui->actionRelease->isChecked());
    json.writePrefs(&prefs);
}


void MainWindow::on_actionAdded_Date_triggered() // Toggle view of "added date" column in my collection table
{
    resizeRecordTable();
    prefs.setAdded(ui->actionAdded_Date->isChecked());
    json.writePrefs(&prefs);
}


void MainWindow::on_importDiscogsAddAddedOpt_triggered() // Keep discogs menu showing when option clicked
{
    ui->menuSettings->show(); // Keep the menu showing
    ui->menuImport_Discogs_Collection->show();
}


void MainWindow::setupContextMenuActions() // Setup context menu on My Collection page
{
    myRecordContextMenu->clear();

    // Create actions with iconc
    if (prefs.getDark()){
        contextActionEdit = new QAction(QIcon(QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/pencil_white.png")),"Edit", this);
        contextActionRemove = new QAction(QIcon(QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/trash_white.png")),"Remove", this);
        contextActionSearchSpotify = new QAction(QIcon(QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/spotify_white.png")), "Search in Spotify", this);
    }
    else {
        contextActionEdit = new QAction(QIcon(QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/pencil_black.png")),"Edit", this);
        contextActionRemove = new QAction(QIcon(QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/trash_black.png")),"Remove", this);
        contextActionSearchSpotify = new QAction(QIcon(QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/spotify_black.png")), "Search in Spotify", this);
    }

    // Add actions to menu
    myRecordContextMenu->addAction(contextActionEdit);
    myRecordContextMenu->addAction(contextActionRemove);
    myRecordContextMenu->addAction(contextActionSearchSpotify);

    // Setup click action
    connect(contextActionEdit, &QAction::triggered, this, &MainWindow::toggleEditRecordFrame);
    connect(contextActionRemove, &QAction::triggered, this, &MainWindow::removeTableRecord);
    connect(contextActionSearchSpotify, &QAction::triggered, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://open.spotify.com/search/" + selectedRec->getArtist() + " " + selectedRec->getName() + "/albums"));
    });
}


bool MainWindow::isSortingByRelease(){  // Return whether the user is sorting by release (if release spin boxes are not the min and max)
    return !(ui->myRecord_FilterReleaseMaxSpinBox->value() == releaseMax && ui->myRecord_FilterReleaseMinSpinBox->value() == releaseMin);
}

void MainWindow::on_editRecord_SuggestedTagButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setStyleSheet(prefs.getMessageStyle());
    std::vector<ListTag> recTags = json.wikiTags(ui->editRecord_TitleEdit->displayText(), ui->editRecord_ArtistEdit->displayText(), false);
    QString tagString = "";
    for (int i = 0; i < recTags.size(); i++) {
        if (i < recTags.size()-1) {
            tagString += " - " + recTags[i].getName() + "\n";
        }
        else {
            tagString += " - " + recTags[i].getName();
        }
    }
    msgBox.setText("The suggested tags for " + ui->editRecord_TitleEdit->displayText() + " by " + ui->editRecord_ArtistEdit->displayText() + " are:\n" + tagString);
    QPushButton *addTagsButton = msgBox.addButton(tr("Add All Tags"), QMessageBox::ActionRole);
    QPushButton *closeButton = msgBox.addButton(QMessageBox::Close);
    msgBox.setDefaultButton(closeButton);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/appico.ico"));
    msgBox.setWindowTitle("Suggested Tags - My Record Collection");
    msgBox.exec();

    if (msgBox.clickedButton() == addTagsButton){ // Add all tags to record
        for (ListTag newTag : recTags) { // Iterate through each new tag
            bool dup = false;
            for (ListTag &existingTag : tags){ // check if tag already exists in tags
                if (existingTag.getName().compare(newTag.getName()) == 0){ // Tag already in tags vector
                    dup = true;
                    if (!selectedRec->hasTag(existingTag.getName())) existingTag.incCount(); // If record does not already have tag, increase count
                    break;
                }
            }
            if (!dup) { // Only add new tag to tags
                newTag.incCount();
                tags.push_back(newTag);
            }
            selectedRec->addTag(newTag.getName());
        }
        sortTagsAlpha(&tags);
        json.writeTags(&tags);
        updateTagList();
    }
}

