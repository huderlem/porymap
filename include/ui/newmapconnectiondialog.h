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

    virtual void accept() override;

signals:
    void accepted(MapConnection *result);

private:
    Ui::NewMapConnectionDialog *ui;

    bool mapNameIsValid();
    void setWarningVisible(bool visible);
};

#endif // NEWMAPCONNECTIONDIALOG_H
