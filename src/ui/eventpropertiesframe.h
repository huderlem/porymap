#ifndef OBJECTPROPERTIESFRAME_H
#define OBJECTPROPERTIESFRAME_H

#include <QFrame>

namespace Ui {
class ObjectPropertiesFrame;
}

class EventPropertiesFrame : public QFrame
{
    Q_OBJECT

public:
    explicit EventPropertiesFrame(QWidget *parent = nullptr);
    ~EventPropertiesFrame();

public:
    Ui::ObjectPropertiesFrame *ui;
};

#endif // OBJECTPROPERTIESFRAME_H
