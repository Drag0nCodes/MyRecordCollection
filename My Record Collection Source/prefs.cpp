#include "prefs.h"
#include <QDir>

Prefs::Prefs(int sortBy, bool darkTheme) {
    this->darkTheme = darkTheme;
    this->sortBy = sortBy;
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

QString Prefs::getStyle()
{
    return styleText;
}

QString Prefs::getMessageStyle()
{
    return styleMessageText;
}

void Prefs::updateThemeString()
{
    QDir dir;
    QFile *styleFile;
    QFile *styleMessageFile;
    if (darkTheme) {
        styleFile = new QFile(dir.absolutePath() + "/resources/themes/dark_theme.qss");
        styleMessageFile = new QFile(dir.absolutePath() + "/resources/themes/dark_theme_message.qss");
    }
    else {
        styleFile = new QFile(dir.absolutePath() + "/resources/themes/light_theme.qss");
        styleMessageFile = new QFile(dir.absolutePath() + "/resources/themes/light_theme_message.qss");
    }
    styleFile->open(QFile::ReadOnly);
    styleMessageFile->open(QFile::ReadOnly);
    styleText = styleFile->readAll();
    styleMessageText = styleMessageFile->readAll();
    styleFile->close();
    styleMessageFile->close();
}


