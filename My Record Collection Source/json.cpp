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
    QDir dir;
    try{
        QString jsonStr;
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

std::vector<Record> json::searchRecords(QString search, int limit) {
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

std::vector<ListTag> json::getTags(){
    std::vector<ListTag> allTags;
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
                        allTags.push_back(ListTag(name));
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

void json::writeTags(std::vector<ListTag>* tags){
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
                    myFile.write("    {\"name\": \"" + tags->at(tagNum).getName().toUtf8() + "\"},\n");
                }
                else { // last tag
                    myFile.write("    {\"name\": \"" + tags->at(tagNum).getName().toUtf8() + "\"}\n");
                }
            }
            myFile.write("]");
            myFile.close();
        }
    }
}

std::vector<ListTag> json::wikiTags(Record record) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    std::vector<ListTag> tags;

    QUrl searchUrl = QUrl("https://en.wikipedia.org/w/api.php?action=query&list=search&utf8=&format=json&srsearch=" + record.getName() + "%20" + record.getArtist() + "%20album");

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

void json::importDiscogs(QString file, std::vector<Record> *allRecords, QProgressDialog *progress) {
    QFile myFile(file); // File of playlists JSON
    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadOnly)){ // Erase all text in playlists json
            QString line = myFile.readLine();
            if (!line.startsWith("Catalog#,Artist,Title")){ // Not a discogs collection, invalid file
                myFile.close();
                std::cerr << "Inavlid collection file" << std::endl;
            }
            line = myFile.readLine(); // Get first discogs record
            while (!line.isEmpty()) { // Loop until all discogs record read in
                QString newArtist = ""; // The artist of the discogs record
                QString newName = ""; // The name of the discogs record
                int newRating = 0; // The rating of the discogs record
                int startSec = 0; // The index of the start of the csv cell/section (usually points to a ',' except when at the start of the line)
                int endSec = line.indexOf(',', startSec+1); // The index of the end of the csv cell/section
                bool literal = false; // Indicates if the current value being read in is string literal as it contains a ','


                if (line.at(startSec) == '"'){ // If the catalog number is a string literal, get to the proper end of the cell
                    literal = true;
                    endSec = line.indexOf('"', startSec+1) +1;
                }

                for (int i = 0; i < 5; i++){ // Run through five times to get the artist, title, and rating
                    startSec = endSec; // Last end cell index becomes new start cell index
                    endSec = line.indexOf(',', startSec+1); // New end cell index is the next ','
                    literal = false;
                    if (line.at(startSec+1) == '"'){ // If the cell is a string literal, get to the proper end of the cell and indicate as such
                        literal = true;
                        endSec = line.indexOf('"', startSec+2) +1;
                    }

                    switch (i){
                    case 0: // Getting the artist name
                        newArtist = line.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1);
                        if (newArtist.endsWith(")")){ // If the artist name ends with a ')', remove the (...)
                            int endBracket = newArtist.lastIndexOf(')');
                            int startBracket = newArtist.lastIndexOf('(');
                            newArtist.chop(endBracket-startBracket+2);
                        }
                        break;
                    case 1: // Getting the record name
                        newName = line.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1);
                        break;
                    case 4: // Get the record rating
                        newRating = line.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1).toInt();
                        newRating *= 2;
                    }
                }
                line = myFile.readLine(); // Get the next line for the next record

                // Find the record on last.fm to get the album cover
                QString coverUrl = searchRecords(newName + " " + newArtist, 1).at(0).getCover(); // Search the record name and artist on last fm, the first result will (hopefully) be the correct album cover
                bool copy = false;
                for (Record record : *allRecords){ // Check all my records to see if cover matches requested add
                    QString searchPageRecordCover = coverUrl; // Copy the cover URL
                    for (int i = searchPageRecordCover.size()-1; i >= 0; i--) { // remove the first part of the cover URL so only the file name remains
                        if (searchPageRecordCover[i] == '/') {
                            searchPageRecordCover.remove(0,i+1);
                            break;
                        }
                    }
                    if (record.getCover().compare(searchPageRecordCover) == 0 || (record.getName().compare(newName) == 0 && record.getArtist().compare(newArtist) == 0)){
                        copy = true; // If the (new cover matches cover filenames) or (the album name and artist match) with a record already in the collection, do not add it again
                        std::cerr << "Skipping from adding: " + newArtist.toStdString() + " - " + newName.toStdString() << std::endl;
                        break;
                    }
                }
                if (!copy){ // Record is not in collection
                    std::cerr << "Adding to collection: " + newArtist.toStdString() + " - " + newName.toStdString() << std::endl;
                    allRecords->push_back(Record(newName, newArtist, downloadCover(coverUrl), newRating)); // Add to master record vector (allMyRecords)
                }
            }
            myFile.close();
            writeRecords(allRecords);
        }
    }
}


QString json::downloadCover(QUrl imageUrl) { // Download image from the internet to covers subfolder
    QString fileName = imageUrl.toString();
    QDir dir;
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

