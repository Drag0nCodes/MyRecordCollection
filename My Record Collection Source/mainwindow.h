#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "listtag.h"
#include "record.h"
#include "json.h"
#include <QDir>

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

    void on_searchRecord_Table_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_myRecord_SearchBar_textChanged();

    void on_myRecord_SortBox_activated(int index);

    void on_editRecord_EditTagsList_itemClicked(QListWidgetItem *item);

    void on_myRecord_FilterTagsList_itemClicked(QListWidgetItem *item);

    void on_editRecord_ManageTagButton_clicked();

    void on_searchRecord_Table_cellClicked(int row, int column);

    void on_searchRecord_SuggestedTagsList_itemClicked(QListWidgetItem *item);

    void on_myRecord_PickForMe_clicked();

    void on_settings_actionExit_triggered();

    void on_settings_actionToggleTheme_triggered();

    void on_actionSelect_File_and_Import_triggered();

    void on_actionDelete_All_User_Data_triggered();

    void on_myRecord_ResetTagsFilterButton_clicked();

    void on_help_actionAbout_triggered();

    void on_help_actionContact_triggered();

    void on_importDiscogsAddTagsOpt_triggered();

    void showContextMenu(const QPoint &pos);

    void on_editRecord_DoneButton_clicked();

    void on_myRecord_customRecordButton_clicked();

    void on_editRecord_ArtistEdit_returnPressed();

    void on_editRecord_TitleEdit_returnPressed();

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
    QDir dir;
    Prefs prefs;
    std::vector<Record> results; // Search records results
    std::vector<Record> allMyRecords; // All records in collection
    std::vector<Record*> recordsList; // Records shown on my collection table
    std::vector<ListTag> tags; // All tags
    std::vector<ListTag> suggestedTags; // The list of suggested tags on the search records page
    bool selectedMyRecord = false; // If a record is selected on the my record page
    QMenu *myRecordContextMenu;
    QAction *contextActionRemove;
    QAction *contextActionEdit;
    void removeTableRecord();
    void toggleEditRecordFrame();
    void checkClickEditRecordFrame();
    void setFocus(QWidget* parent, enum Qt::FocusPolicy focus);
};
#endif // MAINWINDOW_H
