#include "record.h"
#include <sstream>
#include <iostream>

Record::Record(QString name, QString artist, QString cover, std::vector<QString> tags, qint64 rating, qint64 id, qint64 release, QDate added) {
    this->name = name;
    this->artist = artist;
    this->cover = cover;
    this->tags = tags; // Lower case
    this->rating = rating;
    this->id = id;
    this->release = release;
    this->added = added;
}

Record::Record(QString name, QString artist, QString cover, qint64 rating, qint64 id, qint64 release, QDate added) {
    this->name = name;
    this->artist = artist;
    this->cover = cover;
    this->rating = rating;
    this->id = id;
    this->release = release;
    this->added = added;
}

QString Record::getName(){
    return name;
}

QString Record::getArtist(){
    return artist;
}

QString Record::getCover(){
    return cover;
}

std::vector<QString> Record::getTags(){
    return tags;
}

qint64 Record::getRating() const{
    return rating;
}


void Record::setName(QString name){
    this->name = name;
}

void Record::setArtist(QString artist){
    this->artist = artist;
}

void Record::setCover(QString cover){
    this->cover = cover;
}

int Record::addTag(QString tag){
    for (QString testTag : tags){
        if (QString::compare(testTag, tag, Qt::CaseInsensitive) == 0) {
            return 1; // 1 = error, record already has tag, dont add again
        }
    }
    tags.push_back(tag.toLower());
    std::sort(tags.begin(),tags.end());
    return 0;
}

void Record::removeTag(QString tag){
    for (int i = 0; i < tags.size(); i++){
        if (QString::compare(tags.at(i), tag, Qt::CaseInsensitive) == 0) {
            tags.erase(tags.begin()+i);
            break;
        }
    }
}


void Record::removeAllTags()
{
    tags.clear();
}


void Record::setRating(qint64 rating){
    this->rating = rating;
}

bool Record::contains(QString search) {
    try{
        std::vector<std::string> searchWords; // Vector to hold each word separated by spaces from search input
        std::string searchStr = search.toStdString();
        std::stringstream ss(searchStr);
        std::string separatedWord;
        std::string nameLower = name.toLower().toStdString();
        std::string artistLower = artist.toLower().toStdString();

        while (ss >> separatedWord) { // Take each word separated by spaces from input search and put it into vector searchWords
            transform(separatedWord.begin(), separatedWord.end(), separatedWord.begin(), ::tolower);
            searchWords.push_back(separatedWord);
        }

        if (searchWords.empty()) {
            return false;
        }

        for (const std::string& word : searchWords) { // Check if a word from search sting is in either the song name, album, or artist
            bool foundInName = nameLower.find(word) != std::string::npos;
            bool foundInArtist = artistLower.find(word) != std::string::npos;

            if (!(foundInName || foundInArtist)) {
                return false;
            }
        }

        return true;
    } catch (const std::out_of_range& e) {
        std::cerr << "Exception caught - contains method: ";
    }
    return false;
}

void Record::setId(qint64 id)
{
    this->id = id;
}

qint64 Record::getId()
{
    return id;
}

void Record::setRelease(qint64 release)
{
    this->release = release;
}

qint64 Record::getRelease() const
{
    return release;
}

QDate Record::getAdded() const
{
    return added;
}

void Record::setAdded(QDate added)
{
    this->added = added;
}

bool Record::hasTag(QString tag){
    for (QString testTag : tags){
        if (testTag.compare(tag) == 0) return true;
    }
    return false;
}
