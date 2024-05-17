#ifndef PREFS_H
#define PREFS_H
#include <QObject>

class Prefs
{
public:
    Prefs(QObject *parent = nullptr) {}
    Prefs(int sortBy, bool darkTheme);
    void setDark(bool dark);
    void setSort(int sort);
    void toggleTheme();
    bool getDark();
    int getSort();
    QString getStyle();
    QString getMessageStyle();

private:
    int sortBy;
    bool darkTheme;
    QString styleText;
    QString styleMessageText;
    void updateThemeString();
};

#endif // PREFS_H
