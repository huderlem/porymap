#ifndef EVENTPROPERTIESFRAME_H
#define EVENTPROPERTIESFRAME_H

#include <QFrame>

namespace Ui {
class EventPropertiesFrame;
}

class EventPropertiesFrame : public QFrame
{
    Q_OBJECT

public:
    explicit EventPropertiesFrame(QWidget *parent = nullptr);
    ~EventPropertiesFrame();

public:
    Ui::EventPropertiesFrame *ui;
};

#endif // EVENTPROPERTIESFRAME_H
