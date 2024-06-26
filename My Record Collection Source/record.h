#ifndef RECORD_H
#define RECORD_H
#include "QString"
#include "vector"

class Record
{
private:
    QString name;
    QString artist;
    QString cover;
    std::vector<QString> tags; // Lower case
    qint64 rating;

public:
    Record(QString name, QString artist, QString cover, std::vector<QString> tags, qint64 rating);
    Record(QString name, QString artist, QString cover, qint64 rating);
    QString getName();
    QString getArtist();
    QString getCover();
    std::vector<QString> getTags();
    qint64 getRating();
    void setName(QString name);
    void setArtist(QString artist);
    void setCover(QString cover);
    int addTag(QString tag);
    void removeTag(QString tag);
    bool hasTag(QString tag);
    void setRating(qint64 rating);
    bool contains(QString search);
};



#endif // RECORD_H
