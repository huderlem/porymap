#ifndef CUSTOMATTRIBUTESFRAME_H
#define CUSTOMATTRIBUTESFRAME_H

/*
    The frame containing the Custom Attributes table and its Add/Delete buttons.
    Shared by the map's Header tab and Events.
*/

#include "customattributestable.h"

#include <QObject>
#include <QFrame>

namespace Ui {
class CustomAttributesFrame;
}

class CustomAttributesFrame : public QFrame
{
    Q_OBJECT

public:
    explicit CustomAttributesFrame(QWidget *parent = nullptr);
    ~CustomAttributesFrame() {};

    CustomAttributesTable *table;

private:
    Ui::CustomAttributesFrame *ui;
};

#endif // CUSTOMATTRIBUTESFRAME_H
