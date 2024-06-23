#include "listtag.h"

ListTag::ListTag(QString tagName, bool checked) {
    name = tagName;
    this->checked = checked;
    count = 0;
}

QString ListTag::getName() {
    return name;
}


void ListTag::setName(QString tagName) {
    name = tagName;
}


bool ListTag::getChecked() {
    return checked;
}


void ListTag::setChecked(bool checked) {
    this->checked = checked;
}

bool ListTag::toggleCheck()
{
    checked = !checked;
    return checked;
}

void ListTag::setCount(int count)
{
    this->count = count;
}

int ListTag::incCount()
{
    return count++;
}

int ListTag::decCount()
{
    return count--;
}

int ListTag::getCount()
{
    return count;
}
