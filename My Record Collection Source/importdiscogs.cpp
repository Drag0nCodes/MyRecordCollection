#include "importdiscogs.h"
#include "json.h"
#include <iostream>

ImportDiscogs::ImportDiscogs(QString line, std::vector<Record> *allRecordPointer, QObject *parent) : QObject{parent}
{
    recordLine = line;
    allRecords = allRecordPointer;
    processedRec = NULL;
}

ImportDiscogs::ImportDiscogs(QObject *parent) : QObject{parent}
{
}

void ImportDiscogs::importAll(QString file, std::vector<Record> *allRecords) {
    QFile myFile(file); // File of playlists JSON
    json Json;
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
                QString coverUrl = Json.searchRecords(newName + " " + newArtist, 1).at(0).getCover(); // Search the record name and artist on last fm, the first result will (hopefully) be the correct album cover
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
                    allRecords->push_back(Record(newName, newArtist, Json.downloadCover(coverUrl), newRating)); // Add to master record vector (allMyRecords)
                }
            }
            myFile.close();
            //Json.writeRecords(allRecords);
        }
    }
}


void ImportDiscogs::importSingle() {
    QString newArtist = ""; // The artist of the discogs record
    QString newName = ""; // The name of the discogs record
    int newRating = 0; // The rating of the discogs record
    int startSec = 0; // The index of the start of the csv cell/section (usually points to a ',' except when at the start of the line)
    int endSec = recordLine.indexOf(',', startSec+1); // The index of the end of the csv cell/section
    bool literal = false; // Indicates if the current value being read in is string literal as it contains a ','
    Record *returnRec; // The record that will be returned
    //QScopedPointer<json> Json(new json);
    json Json;

    if (recordLine.at(startSec) == '"'){ // If the catalog number is a string literal, get to the proper end of the cell
        literal = true;
        endSec = recordLine.indexOf('"', startSec+1) +1;
    }

    for (int i = 0; i < 5; i++){ // Run through five times to get the artist, title, and rating
        startSec = endSec; // Last end cell index becomes new start cell index
        endSec = recordLine.indexOf(',', startSec+1); // New end cell index is the next ','
        literal = false;
        if (recordLine.at(startSec+1) == '"'){ // If the cell is a string literal, get to the proper end of the cell and indicate as such
            literal = true;
            endSec = recordLine.indexOf('"', startSec+2) +1;
        }

        switch (i){
        case 0: // Getting the artist name
            newArtist = recordLine.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1);
            if (newArtist.endsWith(")")){ // If the artist name ends with a ')', remove the (...)
                int endBracket = newArtist.lastIndexOf(')');
                int startBracket = newArtist.lastIndexOf('(');
                newArtist.chop(endBracket-startBracket+2);
            }
            break;
        case 1: // Getting the record name
            newName = recordLine.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1);
            break;
        case 4: // Get the record rating
            newRating = recordLine.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1).toInt();
            newRating *= 2;
        }
    }

    // Find the record on last.fm to get the album cover
    QString coverUrl = Json.searchRecords(newName + " " + newArtist, 1).at(0).getCover(); // Search the record name and artist on last fm, the first result will (hopefully) be the correct album cover
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
            processedRec = new Record("", "", "", -1);
            break;
        }
    }
    if (!copy){ // Record is not in collection
        //std::cerr << "Ready to return: " + newArtist.toStdString() + " - " + newName.toStdString() << std::endl;
        processedRec = new Record(newName, newArtist, Json.downloadCover(coverUrl), newRating); // Set return val to record pointer
    }
    emit finished();
}


void ImportDiscogs::run() {
    //std::cerr << "STARTING " << recordLine.toStdString() << std::endl;
    //QScopedPointer<QEventLoop> loop(new QEventLoop);
    //connect(this, &ImportDiscogs::finished, )
    importSingle();
    //std::cerr << "FINISHED " << recordLine.toStdString() << std::endl;
}

Record* ImportDiscogs::getProcessedRec(){
    //std::cerr << "Returning: " + processedRec->getArtist().toStdString() + " - " + processedRec->getName().toStdString() << std::endl;
    return processedRec;
}
