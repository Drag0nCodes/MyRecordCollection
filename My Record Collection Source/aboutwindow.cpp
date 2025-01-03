#include "aboutwindow.h"
#include "ui_aboutwindow.h"
#include "json.h"

AboutWindow::AboutWindow(Prefs *prefs, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutWindow)
{
    ui->setupUi(this);

    // Set the style sheet for the program
    setStyleSheet(prefs->getStyle());

    setWindowTitle("About - My Record Collection");

    QDir dir;
    this->setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/resources/images/appico.ico"));

    // Fill text boxes
    try{
        QString jsonStr;

        // Tags about tab
        QFile aboutTagsFile(QCoreApplication::applicationDirPath() + "/resources/about text/about_tags.txt"); // File of about_tags.txt
        if (aboutTagsFile.exists()){
            if (aboutTagsFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                ui->aboutWinTagsText->setText(aboutTagsFile.readAll()); // Read all file text into JLabel
                aboutTagsFile.close();
            }
        }

        QFile aboutRecordsFile(QCoreApplication::applicationDirPath() + "/resources/about text/about_records.txt"); // File of about_tags.txt
        if (aboutRecordsFile.exists()){
            if (aboutRecordsFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                ui->aboutWinRecordsText->setText(aboutRecordsFile.readAll()); // Read all file text into JLabel
                aboutRecordsFile.close();
            }
        }

        QFile aboutDiscogsFile(QCoreApplication::applicationDirPath() + "/resources/about text/about_discogs.txt"); // File of about_discogs.txt
        if (aboutDiscogsFile.exists()){
            if (aboutDiscogsFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                ui->aboutWinDiscogsText->setText(aboutDiscogsFile.readAll()); // Read all file text into JLabel
                aboutDiscogsFile.close();
            }
        }
    }catch (const std::out_of_range& e) {
        //std::cerr << "Exception caught - json get records method: " << e.what() << std::endl;
    }
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::on_aboutWinClose_clicked()
{
    this->close();
}

