#ifndef IMPORTDISCOGS_H
#define IMPORTDISCOGS_H

#include <QObject>
#include <QFile>
#include <QThread>
#include <QRunnable>
#include <QEventLoop>
#include <QScopedPointer>
#include <QTimer>
#include "record.h"

class ImportDiscogs : public QObject, public QRunnable
{
    Q_OBJECT
public:
    ImportDiscogs(QString line, std::vector<Record> *recordPointer, bool addTags, QObject *parent = nullptr);
    ImportDiscogs(bool addTags, QObject *parent = nullptr);
    void run() Q_DECL_OVERRIDE;

signals:
    void finished(Record *record, bool skipped);

public slots:
    Record* getProcessedRec();

private:
    QString recordLine;
    std::vector<Record> *allRecords;
    Record *processedRec;
    bool addTags;
    void importSingle();
};

#endif // IMPORTDISCOGS_H
