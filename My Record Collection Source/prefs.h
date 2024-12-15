#ifndef PREFS_H
#define PREFS_H
#include <QObject>
#include <QSize>

class Prefs
{
public:
    Prefs(QObject *parent = nullptr) {}
    Prefs(int sortBy,  bool asc, bool darkTheme, bool showCover, bool showRating, bool showRelease, bool showAdded, QSize size);
    void setDark(bool dark);
    void setSort(int sort);
    void setAsc(bool asc);
    void toggleTheme();
    bool getDark();
    int getSort();
    bool getAsc();
    QString getStyle();
    QString getMessageStyle();
    bool getCover();
    bool getRating();
    bool getRelease();
    bool getAdded();
    void setCover(bool showCover);
    void setRating(bool showRating);
    void setRelease(bool showRelease);
    void setAdded(bool showAdded);
    QSize getSize();
    void setSize(QSize size);

private:
    int sortBy;
    bool ascending;
    bool darkTheme;
    QString styleText;
    QString styleMessageText;
    bool showCover;
    bool showRating;
    bool showRelease;
    bool showAdded;
    void updateThemeString();
    QSize size;
};

#endif // PREFS_H
