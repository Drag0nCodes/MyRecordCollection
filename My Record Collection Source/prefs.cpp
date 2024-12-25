#include "prefs.h"
#include <QDir>
#include <QCoreApplication>

Prefs::Prefs(int sortBy, bool asc, bool darkTheme, bool showCover, bool showRating, bool showRelease, bool showAdded, QSize size) {
    this->darkTheme = darkTheme;
    this->sortBy = sortBy;
    this->ascending = asc;
    this->showCover = showCover;
    this->showRating = showRating;
    this->showRelease = showRelease;
    this->showAdded = showAdded;
    this->size = size;
    updateThemeString();
}

void Prefs::setDark(bool dark)
{
    darkTheme = dark;
    updateThemeString();
}

void Prefs::setSort(int sort)
{
    sortBy = sort;
}

void Prefs::setAsc(bool asc)
{
    ascending = asc;
}

void Prefs::toggleTheme()
{
    darkTheme = !darkTheme;
    updateThemeString();
}

bool Prefs::getDark()
{
    return darkTheme;
}

int Prefs::getSort()
{
    return sortBy;
}

bool Prefs::getAsc()
{
    return ascending;
}

QString Prefs::getStyle()
{
    return styleText;
}

QString Prefs::getMessageStyle()
{
    return styleMessageText;
}

bool Prefs::getCover()
{
    return showCover;
}

bool Prefs::getRating()
{
    return showRating;
}

bool Prefs::getRelease()
{
    return showRating;
}

bool Prefs::getAdded()
{
    return showAdded;
}

void Prefs::setCover(bool showCover)
{
    this->showCover = showCover;
}

void Prefs::setRating(bool showRating)
{
    this->showRating = showRating;
}

void Prefs::setRelease(bool showRelease)
{
    this->showRelease = showRelease;
}

void Prefs::setAdded(bool showAdded)
{
    this->showAdded = showAdded;
}

QSize Prefs::getSize()
{
    return size;
}

void Prefs::setSize(QSize size)
{
    this->size = size;
}

void Prefs::updateThemeString()
{
    QFile *styleFile;
    QFile *styleMessageFile;
    if (darkTheme) {
        styleFile = new QFile(QCoreApplication::applicationDirPath() + "/resources/themes/dark_theme.qss");
        styleMessageFile = new QFile(QCoreApplication::applicationDirPath() + "/resources/themes/dark_theme_message.qss");
    }
    else {
        styleFile = new QFile(QCoreApplication::applicationDirPath() + "/resources/themes/light_theme.qss");
        styleMessageFile = new QFile(QCoreApplication::applicationDirPath() + "/resources/themes/light_theme_message.qss");
    }
    styleFile->open(QFile::ReadOnly);
    styleMessageFile->open(QFile::ReadOnly);
    styleText = styleFile->readAll();
    styleMessageText = styleMessageFile->readAll();
    styleFile->close();
    styleMessageFile->close();
}


