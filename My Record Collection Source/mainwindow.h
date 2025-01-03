#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "listtag.h"
#include "record.h"
#include "json.h"
#include <QResizeEvent>
#include "threadedcover.h"
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void on_searchRecord_ToMyRecordsButton_clicked();

    void on_myRecord_ToSearchRecordsButton_clicked();

    void on_searchRecord_SearchBar_returnPressed();

    void on_searchRecord_AddToMyRecordButton_clicked();

    void on_myRecord_Table_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_myRecord_SearchBar_textChanged();

    void on_myRecord_SortBox_activated(int index);

    void on_editRecord_EditTagsList_itemClicked(QListWidgetItem *item);

    void on_myRecord_FilterTagsList_itemClicked(QListWidgetItem *item);

    void on_editRecord_ManageTagButton_clicked();

    void on_searchRecord_Table_cellPressed(int row, int column);

    void on_searchRecord_SuggestedTagsList_itemClicked(QListWidgetItem *item);

    void on_myRecord_PickForMe_clicked();

    void on_settings_actionExit_triggered();

    void on_settings_actionToggleTheme_triggered();

    void on_actionSelect_File_and_Import_triggered();

    void on_myRecord_ResetFiltersButton_clicked();

    void on_help_actionAbout_triggered();

    void on_help_actionContact_triggered();

    void on_importDiscogsAddTagsOpt_triggered();

    void showContextMenu(const QPoint &pos);

    void on_editRecord_DoneButton_clicked();

    void on_myRecord_customRecordButton_clicked();

    void on_editRecord_ArtistEdit_returnPressed();

    void on_editRecord_TitleEdit_returnPressed();

    void on_editRecord_CoverEdit_clicked();

    void on_actionDelete_All_User_Data_triggered();

    void on_actionDelete_All_Records_triggered();

    void on_actionDelete_All_Tags_triggered();

    void on_myRecord_FilterRatingMaxSpinBox_valueChanged(int arg1);

    void on_myRecord_FilterRatingMinSpinBox_valueChanged(int arg1);

    void on_actionExport_MRC_Collection_triggered();

    void on_actionImport_MRC_Collection_triggered();

    void on_editRecord_ReleaseEdit_valueChanged(int arg1);

    void on_myRecord_FilterReleaseMinSpinBox_valueChanged(int arg1);

    void on_myRecord_FilterReleaseMaxSpinBox_valueChanged(int arg1);

    void on_actionCover_triggered();

    void on_actionRating_triggered();

    void on_actionRelease_triggered();

    void on_actionAdded_Date_triggered();

    void on_importDiscogsAddAddedOpt_triggered();

    void on_editRecord_SuggestedTagButton_clicked();

private:
    Ui::MainWindow *ui;
    QPixmap getPixmapFromUrl(const QUrl& imageUrl);
    class ImageDelegate;
    void updateMyRecordsTable();
    void updateMyRecordsInfo();
    void updateTagList();
    void updateRecordsListOrder();
    bool deleteCover(const QString& coverName);
    void sortTagsAlpha(std::vector<ListTag> *list, bool delDups = false);
    void updateTagCount();
    Json json;
    Prefs prefs;
    std::vector<Record> results; // Search records results
    std::vector<Record> allMyRecords; // All records in collection
    std::vector<Record*> recordsList; // Records shown on my collection table
    std::vector<ListTag> tags; // All tags
    std::vector<ListTag> suggestedTags; // The list of suggested tags on the search records page
    QMenu *myRecordContextMenu;
    QAction *contextActionRemove;
    QAction *contextActionEdit;
    QAction *contextActionSearchSpotify;
    void removeTableRecord();
    void toggleEditRecordFrame();
    void checkClickEditRecordFrame();
    void setFocus(QWidget* parent, enum Qt::FocusPolicy focus);
    void handleImportFinished(Record *importedRec, bool skipped);
    int finishedImportsCount;
    int totalImports;
    int totalSkipped;
    void onProgressBarValueChanged(int value);
    void deleteUserData(bool delRecords, bool delTags);
    bool copyDirectory(const QString &sourceDir, const QString &destinationDir);
    void setInfoTimed(QLabel *label, int ms, QString text);
    Record* selectedRec = nullptr; // Currently selected record
    void updateEditPopup();
    int releaseMin; // The min release year/value of the collection
    int releaseMax; // The max release year/value of the collection
    void resizeRecordTable();
    std::vector<ThreadedCover*> searchThreadedCovers;
    void handleCoverThreadFinished(QPixmap pixmap, int pos);
    QMovie *loadingSymbol;
    void searchInSpotify();
    void setupContextMenuActions();
    bool isSortingByRelease();
};
#endif // MAINWINDOW_H
