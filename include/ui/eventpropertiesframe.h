#ifndef EVENTPROPERTIESFRAME_H
#define EVENTPROPERTIESFRAME_H

#include "event.h"

#include <QFrame>

namespace Ui {
class EventPropertiesFrame;
}

class EventPropertiesFrame : public QFrame
{
    Q_OBJECT

public:
    explicit EventPropertiesFrame(Event *event, QWidget *parent = nullptr);
    ~EventPropertiesFrame();
    void paintEvent(QPaintEvent*);

public:
    Ui::EventPropertiesFrame *ui;

private:
    Event *event;
    bool firstShow = true;
};

#endif // EVENTPROPERTIESFRAME_H
