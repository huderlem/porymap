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
    explicit ObjectPropertiesFrame(QWidget *parent = nullptr);
    ~ObjectPropertiesFrame();

public:
    Ui::ObjectPropertiesFrame *ui;
};

#endif // OBJECTPROPERTIESFRAME_H
