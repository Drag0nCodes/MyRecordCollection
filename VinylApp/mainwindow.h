#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>

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
    void on_toMyRecordsButton_clicked();

    void on_toSearchRecordsButton_clicked();

    void on_searchRecordSearchBar_returnPressed();

    void on_addToMyRecordButton_clicked();

    void on_removeFromMyRecordButton_clicked();

    void on_myRecordTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_searchRecordTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

    void on_myRecordRatingSlider_valueChanged(int value);

    void on_myRecordSearchBar_returnPressed();

    void on_myRecordSortBox_activated(int index);

    void on_myRecordSearchBar_textChanged(const QString &arg1);

    void on_myRecordEditTagsList_itemClicked(QListWidgetItem *item);

    void on_myRecordSortTagsList_itemClicked(QListWidgetItem *item);

    void on_myRecordManageTagButton_clicked();

private:
    Ui::MainWindow *ui;
    QPixmap getPixmapFromUrl(const QUrl& imageUrl);
    QString downloadCover(QUrl imageUrl);
    class ImageDelegate;
    void updateMyRecords();
    void updateMyRecordsInfo();
    void updateTagsList();
    void updateRecordsListOrder();
    bool deleteCover(const QString& coverName);

};
#endif // MAINWINDOW_H
