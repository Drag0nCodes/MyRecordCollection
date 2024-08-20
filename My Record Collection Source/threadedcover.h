#ifndef THREADEDCOVER_H
#define THREADEDCOVER_H

#include <QObject>
#include <QThread>
#include <QRunnable>
#include <QPixmap>
#include <QUrl>

class ThreadedCover : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ThreadedCover(QUrl imageUrl, int pos, QObject *parent = nullptr);
    void run() Q_DECL_OVERRIDE;
    void setStopFlag(bool val);

signals:
    void finished(QPixmap pixmap, int pos);

private:
    QPixmap pixmap;
    void importSingle();
    QUrl imageUrl;
    int pos;
    bool stopFlag;
};

#endif // THREADEDCOVER_H
