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
    void newConnectionedAdded(const QString &mapName, const QString &direction);
    void connectionReplaced(const QString &mapName, const QString &direction);

private:
    Ui::NewMapConnectionDialog *ui;
    Map *m_map;

    bool mapNameIsValid();
    void setWarningVisible(bool visible);
    bool askReplaceConnection(MapConnection *connection, const QString &newMapName);
};

#endif // NEWMAPCONNECTIONDIALOG_H
