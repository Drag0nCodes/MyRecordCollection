#include "threadedcover.h"
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QDir>
#include <QNetworkReply>
#include <iostream>
#include <QCoreApplication>

ThreadedCover::ThreadedCover(QUrl imageUrl, int pos, QObject *parent) : QObject{parent}
{
    this->imageUrl = imageUrl;
    this->pos = pos;
    stopFlag = false;
}

void ThreadedCover::run()
{
    importSingle();
}

void ThreadedCover::setStopFlag(bool val)
{
    stopFlag = val;
}

void ThreadedCover::importSingle()
{
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
        pixmap = QPixmap(QCoreApplication::applicationDirPath() + "/resources/images/missingImg.jpg");
    }

    delete reply;
    if (!stopFlag) emit finished(pixmap, pos);
}


