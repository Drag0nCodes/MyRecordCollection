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

const int CURRVERSION = 5; // The current version of all JSON versions

std::vector<Record> Json::getRecords(int recordCount, QString path, bool import){
    std::vector<Record> allRecords;
    try{
        QString jsonStr;
        dir.mkpath(QDir::currentPath() + "/resources/user data");
        QFile myFile(path); // File of records JSON

        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();

                QJsonDocument myDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject root = myDoc.object();
                int jsonVersion = root.value("_json_version").toInt();
                if (jsonVersion > CURRVERSION) { // .json file version newer than app supports
                    allRecords.push_back(Record("\"resources/user data\" files not supported on this app version. User data is version " + QString::number(jsonVersion) + ". This app supports up to version " + QString::number(CURRVERSION) + ".", "Please update to a more recent version.", "", NULL, 0, 0, QDate()));
                    return allRecords;
                }

                QJsonArray recs = root.value("records").toArray();

                QJsonObject val;
                QString name;
                QString artist;
                QString cover;
                qint64 rating;
                std::vector<QString> tags;
                qint64 id;
                qint64 release;
                QDate added;

                if (recs.empty()){
                    std::cerr << "JSON error, myArr empty" << std::endl;
                } else {
                    for (int i = 0; i < recs.size(); i++){
                        val = recs.at(i).toObject();
                        if (jsonVersion <= 2 || import) { // If json is version 2 or older or an import, create an id for each record
                            id = recordCount + i;
                        }
                        else {
                            id = val.value("id").toInt();
                        }
                        if (jsonVersion <= 3) { // If json is version 3 or older, set year to 1900
                            release = 1900;
                        }
                        else {
                            release = val.value("release").toInt();
                        }
                        if (jsonVersion <= 4){ // No date added if json is version 4 or older, create
                            added = QDate::currentDate();
                        }
                        else{
                            added = QDate::fromString(val.value("added").toString(), "yyyy-MM-dd");
                        }
                        name = val.value("name").toString();
                        artist = val.value("artist").toString();
                        cover = val.value("cover").toString();
                        rating = val.value("rating").toInt();

                        QJsonArray tagArr = val.value("tags").toArray();
                        tags.clear();
                        for (int j = 0; j < tagArr.size(); j++){
                            tags.push_back(tagArr.at(j).toString());
                        }
                        allRecords.push_back(Record(name, artist, cover, tags, rating, id, release, added)); // Turn all JSON info into a Song object and add to vector
                    }
                }
                if (jsonVersion < CURRVERSION){ // If json is old version, rewrite it with new data
                    writeRecords(&allRecords);
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
            records.push_back(Record(name, artist, coverUrl, 0, 0, 0, QDate::currentDate()));
        }
    }
    else {
        records.push_back(Record("", "", "", -1, -1, 0, QDate(0, 0, 0)));
    }

    delete reply;
    return records;
}

void Json::writeRecords(std::vector<Record>* myRecords){
    QFile myFile(QDir::currentPath() + "/resources/user data/records.json"); // File of records JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in records json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all records to JSON
            QJsonDocument doc;
            QJsonObject root; // Array of all Record object information
            QJsonArray recs; // Array of all records
            root.insert("_json_version", CURRVERSION);
            for (Record rec : *myRecords){ // Add information of each record to a JsonObject and add to the JsonArray
                QJsonObject recObj; // The record object json
                recObj.insert("name", rec.getName());
                recObj.insert("artist", rec.getArtist());
                recObj.insert("cover", rec.getCover());
                recObj.insert("rating", rec.getRating());
                recObj.insert("id", rec.getId());
                recObj.insert("release", rec.getRelease());
                recObj.insert("added", rec.getAdded().toString("yyyy-MM-dd"));
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

std::vector<ListTag> Json::getTags(QString path){
    std::vector<ListTag> allTags;
    try{
        QString jsonStr;
        dir.mkpath(QDir::currentPath() + "/resources/user data");
        QFile myFile(path); // File of tags JSON

        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();

                QJsonDocument myDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject root = myDoc.object();
                int jsonVersion = root.value("_json_version").toInt();
                if (jsonVersion > CURRVERSION) { // .json file version newer than app supports
                    return allTags;
                }

                QJsonArray tags = root.value("tags").toArray();
                if (tags.empty()){
                    std::cerr << "JSON error, myArr empty" << std::endl;
                } else {
                    for (int i = 0; i < tags.size(); i++){
                        allTags.push_back(ListTag(tags.at(i).toString()));
                    }
                }
                if (jsonVersion < CURRVERSION){ // If json is old version, rewrite it with new data
                    writeTags(&allTags);
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
    QFile myFile(QDir::currentPath() + "/resources/user data/tags.json"); // File of tags JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in tags json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all tags to JSON
            QJsonDocument doc;
            QJsonObject root; // Array of all tags
            QJsonArray tagsArr;
            root.insert("_json_version", CURRVERSION);
            for (ListTag tag : *tags) tagsArr.append(tag.getName());
            // Add names of each tag to JsonArray
            root.insert("tags", tagsArr);
            doc.setObject(root);
            myFile.write(doc.toJson()); // Write names to json file
            myFile.close();
        }
    }
}


std::vector<ListTag> Json::wikiTags(QString name, QString artist, bool release) {
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    std::vector<ListTag> tags;

    QUrl searchUrl = QUrl("https://en.wikipedia.org/w/api.php?action=query&list=search&utf8=&format=json&srsearch=" + artist + "%20" + name + "%20album");

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

                QString title = jsonWikiPage.value("query").toObject().value("pages").toObject().value(QString::number(pageid)).toObject().value("title").toString();
                // Maybe try to get a new page if the title is wrong / just the artist? (charlixcx I think)
                if (!title.toLower().contains(name.toLower())) {
                    std::cerr << " COULD NOT FIND " + name.toStdString() + " got " + title.toStdString() + " wiki id: " + QString::number(pageid).toStdString();
                    if (title.compare(artist) == 0){
                        std::cerr << " GOT THE ARTIST PAGE";
                    }
                    std::cerr << std::endl;
                }

                QString content = jsonWikiPage.value("query").toObject().value("pages").toObject().value(QString::number(pageid)).toObject().value("revisions").toArray().at(0).toObject().value("*").toString();

                if (release) tags.push_back(ListTag(QString::number(wikiRelease(content)))); // First tag will really be the release year

                int position = content.indexOf("| genre", 0, Qt::CaseInsensitive); // The start of the genre section
                if (position < 0) return tags;
                int label = content.indexOf("| label", 0, Qt::CaseInsensitive); // The start of the label section
                int arrowStart = content.indexOf("<!--", 0, Qt::CaseInsensitive); // Start of the <!-- ... --> that can sometimes appear after the "| genre" string (e.g. Folklore deluxe)
                int arrowEnd = content.indexOf("-->", 0, Qt::CaseInsensitive); // End of the <!-- ... --> that can sometimes appear after the "| genre" string
                if (arrowStart > 0 && arrowStart < position+20) position = arrowEnd; // Move to position of first genre after --> if start arrow is close to "| genre"

                position = content.indexOf("[[", position) +2; // Move to position of first single genre

                if (label < position) return tags; // No genres on wiki page, return nothing

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

                        if (genre.contains("{") || genre.contains("}") ||  genre.contains("|") || genre.contains("[") || genre.contains("]")) { // If genre has {, }, |, [, or ], do not add to vector
                            continue;
                        }

                        tags.push_back(ListTag(genre.replace("&nbsp;", " "))); // Add the genre tag to the vector that will be returned. If it has "&nbsp;" remove that,
                        }
                }
            }
        }
    }

    return tags;
}

int Json::wikiRelease(QString name, QString artist) { // Get the wikipedia page of a album and return the release year
    QString content = "";
    QNetworkAccessManager manager;
    QEventLoop loop;
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    bool lowConfidence = false;

    QUrl searchUrl = QUrl("https://en.wikipedia.org/w/api.php?action=query&list=search&utf8=&format=json&srsearch=" + artist + "%20" + name + "%20album");

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

                QString title = jsonWikiPage.value("query").toObject().value("pages").toObject().value(QString::number(pageid)).toObject().value("title").toString();
                // Maybe try to get a new page if the title is wrong / just the artist? (charlixcx I think)
                if (!title.toLower().contains(name.toLower())) {
                    std::cerr << " COULD NOT FIND " + name.toStdString() + " got " + title.toStdString() + " wiki id: " + QString::number(pageid).toStdString();
                    if (title.compare(artist) == 0){
                        std::cerr << " GOT THE ARTIST PAGE";
                    }
                    std::cerr << std::endl;
                    lowConfidence = true;
                }

                content = jsonWikiPage.value("query").toObject().value("pages").toObject().value(QString::number(pageid)).toObject().value("revisions").toArray().at(0).toObject().value("*").toString();
            }
        }
    }
    int release = wikiRelease(content);
    if (lowConfidence) release *= -1; // Return a neg number if low confidence
    return release;
}

int Json::wikiRelease(QString content) { // Return the release year of an album given the wikipedia page already
    int release = 0;

    int position = content.indexOf("| released", 0, Qt::CaseInsensitive); // The start of the Released section
    if (position < 0) return 0;
    int yearEnd;

    // Try to get year for "|" separated date
    position = content.indexOf("|", position+1, Qt::CaseInsensitive);
    if (content[position+1]  > 'a' && content[position+1] < 'z') position = content.indexOf("|", position+1, Qt::CaseInsensitive); // | released     = {{Start date|df=yes|2018|4|6}}\n (df=yes before the year)
    yearEnd = content.indexOf("|", position+1, Qt::CaseInsensitive);
    release = content.mid(position+1, yearEnd-position-1).toInt(); // | released     = {{Start date|2023|11|10|df=y}}\n|

    // Did not have one of the above formats, try again for comma separated format
    if (release == 0){
        int position = content.indexOf("| released", 0, Qt::CaseInsensitive); // The start of the Released section
        position = content.indexOf(", ", position+1, Qt::CaseInsensitive) + 1;
        yearEnd = content.indexOf("\n", position+1, Qt::CaseInsensitive);
        if (yearEnd > 0 && content[yearEnd-1] == '}') {
            release = content.mid(position+1, yearEnd-position-3).toInt(); // | released     = {{start date|October 22, 2021}}
        }
        else {
            release = content.mid(position+1, yearEnd-position-1).toInt(); // | released     = October 22, 2021
        }
    }

    // Did not have one of the above formats, try again for space separated format
    if (release == 0) {
        int position = content.indexOf("| released", 0, Qt::CaseInsensitive); // The start of the Released section
        position = content.indexOf("= ", position+1, Qt::CaseInsensitive) + 1;
        position = content.indexOf(" ", position+1, Qt::CaseInsensitive);
        position = content.indexOf(" ", position+1, Qt::CaseInsensitive);
        yearEnd = content.indexOf("\n", position+1, Qt::CaseInsensitive);
        release = content.mid(position+1, yearEnd-position-1).toInt(); // | released     = 1 October 2018\n
    }
    return release;
}


QString Json::downloadCover(QUrl imageUrl) { // Download image from the internet to covers subfolder
    QString fileName = imageUrl.toString();
    for (int i = fileName.size()-1; i >= 0; i--) {
        if (fileName[i] == '/') {
            fileName.remove(0,i+1);
            break;
        }
    }
    QString savePath = QDir::currentPath() + "/resources/user data/covers/" + fileName;
    QFileInfo fileInfo(savePath);

    QNetworkAccessManager manager;

    QNetworkRequest request(imageUrl);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();

        // Ensure savePath directory exists
        QDir().mkpath(fileInfo.path());

        int fileNum = 0;
        QString fileName = fileInfo.baseName();
        while (fileInfo.isFile()) { // While file already exists, add (1), (2), ... until unqiue file name is found
            savePath = fileInfo.path() + "/" + fileName + " (" + QString::number(++fileNum) + ")." + fileInfo.suffix();
            fileInfo = QFileInfo(savePath);
        }

        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(imageData);
            file.close();
            //qDebug() << "Image saved to" << savePath;
        } else {
            qDebug() << "Error: Could not open file for writing";
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
    }

    reply->deleteLater();
    return fileInfo.fileName(); // Return name of file saved
}

bool Json::deleteCover(const QString& coverName) { // Delete album cover from covers subfolder
    QFile file(QDir::currentPath() + "/resources/user data/covers/" + coverName);

    if (file.exists()) {
        if (file.remove()) {
            qDebug() << "File" << coverName << "deleted successfully";
            return true;
        } else {
            qDebug() << "Error: Could not delete file" << coverName;
            return false;
        }
    } else {
        qDebug() << "Error: File" << coverName << "does not exist";
        return false;
    }
}

Prefs Json::getPrefs(){
    try{
        QString jsonStr;
        dir.mkpath(QDir::currentPath() + "/resources/user data");
        QFile myFile(QDir::currentPath() + "/resources/user data/prefs.json"); // File of tags JSON

        if (myFile.exists()){
            if (myFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                jsonStr = myFile.readAll(); // Read all file into jsonStr
                myFile.close();
                QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
                QJsonObject root = doc.object();
                int jsonVersion = root.value("_json_version").toInt();
                if (jsonVersion > CURRVERSION) { // .json file version newer than app supports
                    return Prefs(0, true, true, true, true, true, QSize(1000, 800));
                }
                bool showCover;
                bool showRating;
                bool showRelease;
                bool showAdded;
                QSize size;
                if (jsonVersion <= 4){ // No date added if json is version 4 or older, create
                    showCover = true;
                    showRating = true;
                    showRelease = true;
                    showAdded = true;
                    size = QSize(1000, 800);
                }
                else{ // Get window prefs
                    showCover = root.value("showCover").toBool();
                    showRating = root.value("showRating").toBool();
                    showRelease = root.value("showRelease").toBool();
                    showAdded = root.value("showAdded").toBool();
                    size = QSize(root.value("width").toInt(), root.value("height").toInt());
                }
                int sortBy = root.value("sortBy").toInt();
                bool theme = root.value("darkTheme").toBool();

                Prefs prefs = Prefs(sortBy, theme, showCover, showRating, showRelease, showAdded, size);

                if (jsonVersion < CURRVERSION){ // If json is old version, rewrite it with new data
                    writePrefs(&prefs);
                }
                return prefs;
            }
        }else{ // Create a prefs file with default vals
            if (myFile.open(QIODevice::WriteOnly | QIODevice::Text)){
                myFile.close();
                Prefs returnPref(0, true, true, true, true, true, QSize(1000, 800));
                writePrefs(&returnPref);
                return returnPref;
            }
        }
    }catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - json get tags method: " << e.what() << std::endl;
    }
    return Prefs(0, true, true, true, true, true, QSize(1000, 800));
}

void Json::writePrefs(Prefs *prefs){
    QFile myFile(QDir::currentPath() + "/resources/user data/prefs.json"); // File of preferences JSON

    if (myFile.exists()){
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in prefs json
            myFile.close();
        }
        if (myFile.open(QIODevice::ReadWrite | QIODevice::Text)){ // Write all prefs to JSON
            QJsonDocument doc;
            QJsonObject root;
            root.insert("_json_version", CURRVERSION);
            root.insert("darkTheme", prefs->getDark()); // Add preferences to json object
            root.insert("sortBy", prefs->getSort());
            root.insert("showCover", prefs->getCover());
            root.insert("showRating", prefs->getRating());
            root.insert("showRelease", prefs->getRelease());
            root.insert("showAdded", prefs->getAdded());
            root.insert("width", prefs->getSize().width());
            root.insert("height", prefs->getSize().height());
            doc.setObject(root);
            myFile.write(doc.toJson()); // Write to json file
            myFile.close();
        }
    }
}

void Json::deleteUserData(bool delRecords, bool delTags)
{
    QFile tagsFile(QDir::currentPath() + "/resources/user data/tags.json");
    QFile recordsFile(QDir::currentPath() + "/resources/user data/records.json");

    if (delTags){
        if (tagsFile.exists()){
            if (tagsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in prefs json
                QJsonDocument doc;
                QJsonObject root; // Array of all tags
                QJsonArray tagsArr;
                root.insert("_json_version", CURRVERSION);
                root.insert("tags", tagsArr);
                doc.setObject(root);
                tagsFile.write(doc.toJson()); // Write names to json file
                tagsFile.close();
            }
        }
    }
    if (delRecords){
        if (recordsFile.exists()){
            if (recordsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){ // Erase all text in records json
                QJsonDocument doc;
                QJsonObject root; // Array of all Record object information
                QJsonArray recs; // Array of all records
                root.insert("_json_version", CURRVERSION);
                root.insert("records", recs);
                doc.setObject(root);
                recordsFile.write(doc.toJson());
                recordsFile.close();
            }
        }
    }
}


