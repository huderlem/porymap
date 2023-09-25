#ifndef CUSTOMSCRIPTSLISTITEM_H
#define CUSTOMSCRIPTSLISTITEM_H

#include <QFrame>

namespace Ui {
class CustomScriptsListItem;
}

class CustomScriptsListItem : public QFrame
{
    Q_OBJECT

public:
    explicit CustomScriptsListItem(QWidget *parent = nullptr);
    ~CustomScriptsListItem();

public:
    Ui::CustomScriptsListItem *ui;
};

#endif // CUSTOMSCRIPTSLISTITEM_H
