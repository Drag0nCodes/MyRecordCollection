#ifndef LISTTAG_H
#define LISTTAG_H
#include <QString>

class ListTag
{
public:
    ListTag(QString tagName, bool checked = false);
    QString getName();
    void setName(QString tagName);
    bool getChecked();
    void setChecked(bool checked);

private:
    QString name;
    bool checked;
};


#endif // LISTTAG_H
