#include "importdiscogs.h"
#include "json.h"
#include <iostream>

ImportDiscogs::ImportDiscogs(QString line, std::vector<Record> *allRecordPointer, bool addTags, bool addAdded, QObject *parent) : QObject{parent}
{
    recordLine = line;
    allRecords = allRecordPointer;
    processedRec = NULL;
    this->addTags = addTags;
    this->addAdded = addAdded;
}

ImportDiscogs::ImportDiscogs(bool addTags, QObject *parent) : QObject{parent}
{
    this->addTags = addTags;
}

void ImportDiscogs::importSingle() {
    QString newArtist = ""; // The artist of the discogs record
    QString newName = ""; // The name of the discogs record
    int newRating = 0; // The rating of the discogs record
    int newRelease = 1900;
    QDate newAdded;
    int startSec = 0; // The index of the start of the csv cell/section (usually points to a ',' except when at the start of the line)
    int endSec = recordLine.indexOf(',', startSec+1); // The index of the end of the csv cell/section
    bool literal = false; // Indicates if the current value being read in is string literal as it contains a ','
    Record *returnRec; // The record that will be returned
    QScopedPointer<Json> json(new Json);

    if (recordLine.at(startSec) == '"'){ // If the catalog number is a string literal, get to the proper end of the cell
        literal = true;
        endSec = recordLine.indexOf('"', startSec+1) +1;
    }

    for (int i = 0; i < 9; i++){ // Run through five times to get the artist, title, and rating
        startSec = endSec; // Last end cell index becomes new start cell index
        endSec = recordLine.indexOf(',', startSec+1); // New end cell index is the next ','
        literal = false;
        if (recordLine.at(startSec+1) == '"'){ // If the cell is a string literal, get to the proper end of the cell and indicate as such
            literal = true;
            endSec = recordLine.indexOf('"', startSec+2) +1;
            while (recordLine[endSec] == '"') endSec = recordLine.indexOf('"', endSec+1) +1; // if string literal has a "" in it due to size of record (12"), account for and go to end of string literal
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
            break;
        case 5: // Get record release
            newRelease = recordLine.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1).toInt();
            if (newRelease == 0) newRelease = 1900;
            break;
        case 8: // Get record release
            if (addAdded){
                QString addedString = recordLine.mid(startSec + literal + 1, endSec - startSec - literal*2 - 1);
                newAdded = QDateTime::fromString(addedString, "yyyy-MM-dd HH:mm:ss").date();
            }
            else newAdded = QDate::currentDate();
            break;
        }
    }

    // Find the record on last.fm to get the album cover
    QString coverUrl = json->searchRecords(newName + " " + newArtist, 1).at(0).getCover(); // Search the record name and artist on last fm, the first result will (hopefully) be the correct album cover
    bool copy = false;
    for (Record record : *allRecords){ // Check all my records to see if cover matches requested add
        QString searchPageRecordCover = coverUrl; // Copy the cover URL
        for (int i = searchPageRecordCover.size()-1; i >= 0; i--) { // remove the first part of the cover URL so only the file name remains
            if (searchPageRecordCover[i] == '/') {
                searchPageRecordCover.remove(0,i+1);
                break;
            }
        }
        if (record.getCover().compare(searchPageRecordCover) == 0 || (record.getName().toLower().compare(newName.toLower()) == 0 && record.getArtist().toLower().compare(newArtist.toLower()) == 0)){
            copy = true; // If the (new cover matches cover filenames) or (the album name and artist match) with a record already in the collection, do not add it again
            std::cerr << "Skipping from adding: " + newArtist.toStdString() + " - " + newName.toStdString() << std::endl;
            processedRec = new Record("", "", "", -1, 0, 0, QDate(0, 0, 0));
            break;
        }
    }
    if (!copy){ // Record is not in collection
        //std::cerr << "Ready to return: " + newArtist.toStdString() + " - " + newName.toStdString() << std::endl;
        processedRec = new Record(newName, newArtist, json->downloadCover(coverUrl), newRating, 0, newRelease, newAdded); // Set return val to record pointer
        if (addTags){
            std::vector<ListTag> tags = json->wikiTags(newName, newArtist, false);
            for (ListTag tag : tags) {
                processedRec->addTag(tag.getName());
            }
        }
    }
    emit finished(processedRec, copy);
}


void ImportDiscogs::run() {
    importSingle();
}

Record* ImportDiscogs::getProcessedRec(){
    //std::cerr << "Returning: " + processedRec->getArtist().toStdString() + " - " + processedRec->getName().toStdString() << std::endl;
    return processedRec;
}
