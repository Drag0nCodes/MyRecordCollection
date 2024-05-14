#ifndef JSON_H
#define JSON_H
#include "record.h"
#include "listtag.h"
#include <QUrl>

class json
{
public:
    json(QObject *parent = nullptr) {}
    std::vector<Record> getRecords();
    std::vector<Record> searchRecords(QString search, int limit);
    void writeRecords(std::vector<Record>* myRecords);
    std::vector<ListTag> getTags();
    void writeTags(std::vector<ListTag>* tags);
    std::vector<ListTag> wikiTags(Record record);
    QString downloadCover(QUrl imageUrl);
    void writePrefs(Record rec);
};

#endif // JSON_H
