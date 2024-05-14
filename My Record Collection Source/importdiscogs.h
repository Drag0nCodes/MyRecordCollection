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
    ImportDiscogs(QString line, std::vector<Record> *recordPointer, QObject *parent = nullptr);
    ImportDiscogs(QObject *parent = nullptr);
    void run() Q_DECL_OVERRIDE;

signals:
    void finished();

public slots:
    void importAll(QString file, std::vector<Record> *allRecordsPoint);
    void importSingle();
    Record* getProcessedRec();

private:
    QString recordLine;
    std::vector<Record> *allRecords;
    Record *processedRec;
};

#endif // IMPORTDISCOGS_H
