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
    bool toggleCheck();
    void setCount(int count);
    int incCount();
    int decCount();
    int getCount();

private:
    QString name;
    bool checked;
    int count;
};


#endif // LISTTAG_H
