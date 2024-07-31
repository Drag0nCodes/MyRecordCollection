#ifndef JSON_H
#define JSON_H
#include "prefs.h"
#include "record.h"
#include "listtag.h"
#include <QUrl>
#include <QObject>
#include <QDir>

class Json
{
public:
    Json(QObject *parent = nullptr) {}
    std::vector<Record> getRecords(int recordCount, QString path, bool import);
    std::vector<Record> searchRecords(QString search, int limit);
    void writeRecords(std::vector<Record>* myRecords);
    std::vector<ListTag> getTags(QString path);
    void writeTags(std::vector<ListTag>* tags);
    std::vector<ListTag> wikiTags(QString name, QString artist, bool release);
    int wikiRelease(QString name, QString artist);
    int wikiRelease(QString data);
    QString downloadCover(QUrl imageUrl);
    bool deleteCover(const QString& coverName);
    Prefs getPrefs();
    void writePrefs(Prefs *prefs);
    void deleteUserData(bool delRecords, bool delTags);

private:
    QDir dir;
};


#endif // JSON_H
