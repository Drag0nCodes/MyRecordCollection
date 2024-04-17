#include "listtag.h"

ListTag::ListTag(QString tagName, bool checked) {
    name = tagName;
    this->checked = checked;
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
