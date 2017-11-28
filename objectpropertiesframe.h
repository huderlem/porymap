#ifndef OBJECTPROPERTIESFRAME_H
#define OBJECTPROPERTIESFRAME_H

#include <QFrame>

namespace Ui {
class ObjectPropertiesFrame;
}

class ObjectPropertiesFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ObjectPropertiesFrame(QWidget *parent = 0);
    ~ObjectPropertiesFrame();

public:
    Ui::ObjectPropertiesFrame *ui;
};

#endif // OBJECTPROPERTIESFRAME_H
