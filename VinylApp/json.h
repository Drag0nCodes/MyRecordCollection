#ifndef JSON_H
#define JSON_H
#include "record.h"

class json
{
public:
    json();
    std::vector<Record> getRecords();
    std::vector<Record> searchRecords(QString search);
    void writeRecords(std::vector<Record>* myRecords);
    std::vector<QString> getTags();
    void writeTags(std::vector<QString>* tags);
};

#endif // JSON_H
