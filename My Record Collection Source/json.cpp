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

std::vector<Record> Json::getRecords(){
    std::vector<Record> allRecords;
    QDir dir;
    try{
        QString jsonStr;
        dir.mkpath(dir.absolutePath() + "/resources/user data");
        QFile myFile(dir.absolutePath() + "/resources/user data/records.json"); // File of records JSON


        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();

                QJsonDocument myDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject root = myDoc.object();
                int jsonVersion = root.value("_json_version").toInt();
                QJsonArray recs = root.value("records").toArray();

                if (recs.empty()){
                    std::cerr << "JSON error, myArr empty" << std::endl;
                } else {
                    for (int i = 0; i < recs.size(); i++){
                        QJsonObject val = recs.at(i).toObject();
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
        }else{ // Create file
            if (myFile.open(QIODevice::WriteOnly | QIODevice::Text)){
                myFile.close();
            }
        }
    }catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - json get records method: " << e.what() << std::endl;
    }
    return allRecords; // Return vector with all Song objects
}

std::vector<Record> Json::searchRecords(QString search, int limit) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    std::vector<Record> records;
    QString apiKey = "2155e8a6b605edf56e1a6bd2fbdd8115";

    QUrl searchUrl = QUrl("https://ws.audioscrobbler.com/2.0/?method=album.search&api_key=" + apiKey + "&format=json&limit=" + QString::number(limit) + "&album=" + search);

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
            records.push_back(Record(name, artist, coverUrl, 0));
        }
    }

    delete reply;
    return records;
}

void Json::writeRecords(std::vector<Record>* myRecords){
    QDir dir;
    QFile myFile(dir.absolutePath() + "/resources/user data/records.json"); // File of playlists JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in playlists json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all playlists to JSON
            QJsonDocument doc;
            QJsonObject root; // Array of all Record object information
            QJsonArray recs; // Array of all records
            root.insert("_json_version", 2);
            for (Record rec : *myRecords){ // Add information of each record to a JsonObject and add to the JsonArray
                QJsonObject recObj; // The record object json
                recObj.insert("name", rec.getName());
                recObj.insert("artist", rec.getArtist());
                recObj.insert("cover", rec.getCover());
                recObj.insert("rating", rec.getRating());
                QJsonArray tags;
                for (QString tag : rec.getTags()){
                    tags.insert(tags.size(), tag);
                }
                recObj.insert("tags", tags);
                recs.insert(recs.size(), recObj);
            }
            root.insert("records", recs);
            doc.setObject(root);
            myFile.write(doc.toJson());
            myFile.close();
        }
    }
}

std::vector<ListTag> Json::getTags(){
    std::vector<ListTag> allTags;
    try{
        QString jsonStr;
        QDir dir;
        dir.mkpath(dir.absolutePath() + "/resources/user data");
        QFile myFile(dir.absolutePath() + "/resources/user data/tags.json"); // File of tags JSON


        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();

                QJsonDocument myDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject root = myDoc.object();
                int jsonVersion = root.value("_json_version").toInt();

                if (jsonVersion == 2) {
                    QJsonArray tags = root.value("tags").toArray();
                    if (tags.empty()){
                        std::cerr << "JSON error, myArr empty" << std::endl;
                    } else {
                        for (int i = 0; i < tags.size(); i++){
                            allTags.push_back(ListTag(tags.at(i).toString()));
                        }
                    }
                }
            }
        }else{// Create file
            if (myFile.open(QIODevice::WriteOnly | QIODevice::Text)){
                myFile.close();
            }
        }
    }catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - json get tags method: " << e.what() << std::endl;
    }
    return allTags; // Return vector with all Song objects
}

void Json::writeTags(std::vector<ListTag>* tags){
    QDir dir;
    QFile myFile(dir.absolutePath() + "/resources/user data/tags.json"); // File of tags JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in tags json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all tags to JSON
            QJsonDocument doc;
            QJsonObject root; // Array of all tags
            QJsonArray tagsArr;
            root.insert("_json_version", 2);
            for (ListTag tag : *tags) tagsArr.append(tag.getName());
            // Add names of each tag to JsonArray
            root.insert("tags", tagsArr);
            doc.setObject(root);
            myFile.write(doc.toJson()); // Write names to json file
            myFile.close();
        }
    }
}

std::vector<ListTag> Json::wikiTags(QString name, QString artist) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    std::vector<ListTag> tags;

    QUrl searchUrl = QUrl("https://en.wikipedia.org/w/api.php?action=query&list=search&utf8=&format=json&srsearch=" + name + "%20" + artist + "%20album");

    QNetworkRequest request(searchUrl);
    QNetworkReply *reply = manager.get(request);
    loop.exec(); // Wait for the download to finish

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonObject jsonSearch = QJsonDocument::fromJson(data).object();
        QJsonObject query = jsonSearch.value("query").toObject();

        if (query.value("searchinfo").toObject().value("totalhits").toInt() > 0){ // Got a result from wikipedia search
            int pageid = query.value("search").toArray().at(0).toObject().value("pageid").toInt(); // Get the page id of the first search result
            searchUrl = QUrl("https://en.wikipedia.org/w/api.php?action=query&prop=revisions&rvprop=content&format=json&rvsection=0&pageids=" + QString::number(pageid));
            QNetworkRequest request(searchUrl);
            QNetworkReply *reply = manager.get(request);
            loop.exec(); // Wait for the download to finish

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QJsonObject jsonWikiPage = QJsonDocument::fromJson(data).object();
                QString content = jsonWikiPage.value("query").toObject().value("pages").toObject().value(QString::number(pageid)).toObject().value("revisions").toArray().at(0).toObject().value("*").toString();

                int position = content.indexOf("| genre", 0, Qt::CaseInsensitive); // The start of the genre section
                position = content.indexOf("[[", position) +2; // Move to position of first single genre
                int genreSectionEnd = content.indexOf("\n|", position); // The end of the entire genre section
                int genre = 0;
                int source = 0;
                if (position > 0) {
                    while (position < genreSectionEnd && position != -1) {
                        int genreEndPos = content.indexOf("]]", position); // End of single genre
                        int midLine = content.indexOf("|", position) +1; // Find next '|'

                        if (source < position && source > 0) { // See if "genre" is really a link to a reference site
                            source = content.indexOf("=[[", position);
                            position = content.indexOf("[[", position) +2;
                            continue;
                        }

                        QString genre;
                        if (midLine < genreEndPos){ // Genre is of the form "Pop music|Pop"
                            genre = content.mid(midLine, genreEndPos-midLine).toLower();
                        }
                        else { // Genre is of the from "Pop" (without middle line '|')
                            genre = content.mid(position, genreEndPos-position).toLower();
                        }
                        source = content.indexOf("=[[", position);
                        position = content.indexOf("[[", position) +2;

                        if (genre.contains("{") || genre.contains("}") ||  genre.contains("|") || genre.contains("[") || genre.contains("]")) {
                            continue;
                        }

                        tags.push_back(ListTag(genre));
                    }
                }
            }
        }
    }

    return tags;
}

QString Json::downloadCover(QUrl imageUrl) { // Download image from the internet to covers subfolder
    QString fileName = imageUrl.toString();
    QDir dir;
    for (int i = fileName.size()-1; i >= 0; i--) {
        if (fileName[i] == '/') {
            fileName.remove(0,i+1);
            break;
        }
    }
    QString savePath = dir.absolutePath() + "/resources/user data/covers/" + fileName;

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

Prefs Json::getPrefs(){
    QDir dir;
    try{
        QString jsonStr;
        dir.mkpath(dir.absolutePath() + "/resources/user data");
        QFile myFile(dir.absolutePath() + "/resources/user data/prefs.json"); // File of tags JSON

        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject root = doc.object();
                int jsonVersion = root.value("_json_version").toInt();

                if (jsonVersion == 2) {
                    return Prefs(root.value("sortBy").toInt(), root.value("darkTheme").toBool()); // Return a Prefs object with the read information
                }
            }
        }else{ // Create a prefs file with default vals
            if (myFile.open(QIODevice::WriteOnly | QIODevice::Text)){
                myFile.close();
                Prefs returnPref(0, true);
                writePrefs(&returnPref);
                return returnPref;
            }
        }
    }catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - json get tags method: " << e.what() << std::endl;
    }
    return Prefs(0, true);
}

void Json::writePrefs(Prefs *prefs){
    QDir dir;
    QFile myFile(dir.absolutePath() + "/resources/user data/prefs.json"); // File of preferences JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in prefs json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all prefs to JSON
            QJsonDocument doc;
            QJsonObject root;
            root.insert("_json_version", 2);
            root.insert("darkTheme", prefs->getDark()); // Add preferences to json object
            root.insert("sortBy", prefs->getSort());
            doc.setObject(root);
            myFile.write(doc.toJson()); // Write to json file
            myFile.close();
        }
    }
}

void Json::deleteUserData()
{
    QDir dir;
    QFile tagsFile(dir.absolutePath() + "/resources/user data/tags.json");
    QFile recordsFile(dir.absolutePath() + "/resources/user data/records.json");

    if (tagsFile.exists()){
        if (tagsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in prefs json
            tagsFile.close();
        }
    }
    if (recordsFile.exists()){
        if (recordsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in prefs json
            recordsFile.close();
        }
    }
}


