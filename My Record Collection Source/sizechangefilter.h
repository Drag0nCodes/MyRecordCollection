#ifndef SIZECHANGEFILTER_H
#define SIZECHANGEFILTER_H
#include <QResizeEvent>

class SizeChangeFilter : public QObject
{
    Q_OBJECT

protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Resize) {
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
            emit newSize(resizeEvent->size());
        }
        return QObject::eventFilter(obj, event);
    }

signals:
    void newSize(QSize size);
};
#endif // SIZECHANGEFILTER_H
