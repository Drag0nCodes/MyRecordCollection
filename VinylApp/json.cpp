#include "json.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <iostream>

json::json() {}

std::vector<Record> json::getRecords(){
    std::vector<Record> allRecords;
    try{
        QString jsonStr;
        QDir dir;
        QFile myFile(dir.absolutePath() + "/resources/records.json"); // File of records JSON


        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();

                QJsonDocument myDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonArray myArr = myDoc.array();

                if (myArr.empty()){
                    std::cerr << "JSON error, myArr empty" << std::endl;
                } else {
                    for (int i = 0; i < myArr.size(); i++){
                        QJsonObject val = myArr.at(i).toObject();
                        QString name = val.value("name").toString();
                        QString artist = val.value("artist").toString();
                        QString cover = val.value("cover").toString();
                        qint64 rating = val.value("rating").toInteger();
                        QJsonArray tagArr = val.value("tags").toArray();
                        std::vector<QString> tags;
                        for (int j = 0; j < tagArr.size(); j++){
                            tags.push_back(tagArr.at(j).toString());
                        }
                        allRecords.push_back(Record(name, artist, cover, tags, rating)); // Turn all JSON info into a Song object and add to vector
                    }
                }
            }
        }else{

        }
    }catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - json get records method: " << e.what() << std::endl;
    }
    return allRecords; // Return vector with all Song objects
}

std::vector<Record> json::searchRecords(QString search) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    std::vector<Record> records;
    QString apiKey = "2155e8a6b605edf56e1a6bd2fbdd8115";

    QUrl searchUrl = QUrl("https://ws.audioscrobbler.com/2.0/?method=album.search&api_key=" + apiKey + "&format=json&limit=10&album=" + search);

    QNetworkRequest request(searchUrl);
    QNetworkReply *reply = manager.get(request);

    loop.exec(); // Wait for the download to finish

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();


        QJsonObject jsonWeb = QJsonDocument::fromJson(data).object();
        QJsonObject results = jsonWeb.value("results").toObject();
        QJsonObject albumMatches = results.value("albummatches").toObject();
        QJsonArray albumArr = albumMatches.value("album").toArray();

        for (int i = 0; i < albumArr.size(); i++){
            QJsonObject val = albumArr.at(i).toObject();
            QString name = val.value("name").toString();
            QString artist = val.value("artist").toString();
            QJsonArray coversArr = val.value("image").toArray();
            QString coverUrl = coversArr.at(2).toObject().value("#text").toString();
            records.push_back(Record(name, artist, coverUrl));
        }
    }

    delete reply;
    return records;
}

void json::writeRecords(std::vector<Record>* myRecords){
    QDir dir;
    QFile myFile(dir.absolutePath() + "/resources/records.json"); // File of playlists JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in playlists json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all playlists to JSON
            myFile.write("[\n");
            for (int recNum = 0; recNum < myRecords->size(); recNum++){
                myFile.write("    {\n        \"name\": \"" + myRecords->at(recNum).getName().toUtf8() + "\",\n"
                "        \"artist\": \"" + myRecords->at(recNum).getArtist().toUtf8() + "\",\n"
                "        \"cover\": \"" + myRecords->at(recNum).getCover().toUtf8() + "\",\n"
                "        \"rating\": " + QString::number(myRecords->at(recNum).getRating()).toUtf8() + ",\n        \"tags\": [");
                for (int tagNum = 0; tagNum < myRecords->at(recNum).getTags().size(); tagNum++){
                    if (tagNum < myRecords->at(recNum).getTags().size()-1){ // not last
                        myFile.write("\"" + myRecords->at(recNum).getTags().at(tagNum).toUtf8() + "\", ");
                    } else { // last tag
                        myFile.write("\"" + myRecords->at(recNum).getTags().at(tagNum).toUtf8() + "\"");
                    }
                }
                if (recNum < myRecords->size()-1) myFile.write("]\n    },\n"); // another record
                else myFile.write("]\n    }\n"); // no records left
            }
            myFile.write("]");
            myFile.close();
        }
    }
}

std::vector<QString> json::getTags(){
    std::vector<QString> allTags;
    try{
        QString jsonStr;
        QDir dir;
        QFile myFile(dir.absolutePath() + "/resources/tags.json"); // File of tags JSON


        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();

                QJsonDocument myDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonArray myArr = myDoc.array();

                if (myArr.empty()){
                    std::cerr << "JSON error, myArr empty" << std::endl;
                } else {
                    for (int i = 0; i < myArr.size(); i++){
                        QJsonObject val = myArr.at(i).toObject();
                        QString name = val.value("name").toString();
                        allTags.push_back(name);
                    }
                }
            }
        }else{

        }
    }catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - json get tags method: " << e.what() << std::endl;
    }
    return allTags; // Return vector with all Song objects
}

void json::writeTags(std::vector<QString>* tags){
    QDir dir;
    QFile myFile(dir.absolutePath() + "/resources/tags.json"); // File of playlists JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in playlists json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all playlists to JSON
            myFile.write("[\n");
            for (int tagNum = 0; tagNum < tags->size(); tagNum++){
                if (tagNum != tags->size()-1) { // Not last tag
                    myFile.write("    {\"name\": \"" + tags->at(tagNum).toUtf8() + "\"},\n");
                }
                else { // last tag
                    myFile.write("    {\"name\": \"" + tags->at(tagNum).toUtf8() + "\"}\n");
                }
            }
            myFile.write("]");
            myFile.close();
        }
    }
}
