#ifndef JSON_H
#define JSON_H
#include "record.h"
#include "listtag.h"

class json
{
public:
    json();
    std::vector<Record> getRecords();
    std::vector<Record> searchRecords(QString search);
    void writeRecords(std::vector<Record>* myRecords);
    std::vector<ListTag> getTags();
    void writeTags(std::vector<ListTag>* tags);
    std::vector<ListTag> wikiTags(Record record);
};

#endif // JSON_H
