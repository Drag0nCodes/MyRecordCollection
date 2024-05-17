#ifndef JSON_H
#define JSON_H
#include "prefs.h"
#include "record.h"
#include "listtag.h"
#include <QUrl>
#include <QObject>

class Json
{
public:
    Json(QObject *parent = nullptr) {}
    std::vector<Record> getRecords();
    std::vector<Record> searchRecords(QString search, int limit);
    void writeRecords(std::vector<Record>* myRecords);
    std::vector<ListTag> getTags();
    void writeTags(std::vector<ListTag>* tags);
    std::vector<ListTag> wikiTags(QString name, QString artist);
    QString downloadCover(QUrl imageUrl);
    Prefs getPrefs();
    void writePrefs(Prefs *prefs);
    void deleteUserData();
};

#endif // JSON_H
