#ifndef JSON_H
#define JSON_H
#include "record.h"
#include "listtag.h"
#include <QUrl>
#include <QProgressDialog>

class json
{
public:
    json();
    std::vector<Record> getRecords();
    std::vector<Record> searchRecords(QString search, int limit);
    void writeRecords(std::vector<Record>* myRecords);
    std::vector<ListTag> getTags();
    void writeTags(std::vector<ListTag>* tags);
    std::vector<ListTag> wikiTags(Record record);
    void importDiscogs(QString file, std::vector<Record> *allRecords, QProgressDialog *progress);
    QString downloadCover(QUrl imageUrl);
};

#endif // JSON_H
