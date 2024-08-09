#ifndef NEWMAPCONNECTIONDIALOG_H
#define NEWMAPCONNECTIONDIALOG_H

#include <QDialog>
#include "map.h"
#include "mapconnection.h"

namespace Ui {
class NewMapConnectionDialog;
}

class NewMapConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewMapConnectionDialog(QWidget *parent, Map* map, const QStringList &mapNames);
    ~NewMapConnectionDialog();

    MapConnection *result;

    virtual void accept() override;

private:
    Ui::NewMapConnectionDialog *ui;
    Map *map;
};

#endif // NEWMAPCONNECTIONDIALOG_H
