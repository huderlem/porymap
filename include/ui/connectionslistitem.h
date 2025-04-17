#ifndef CONNECTIONSLISTITEM_H
#define CONNECTIONSLISTITEM_H

#include "mapconnection.h"
#include "map.h"

#include <QFrame>
#include <QMouseEvent>
#include <QPointer>

namespace Ui {
class ConnectionsListItem;
}

// We show the data for each map connection in the panel on the right side of the Connections tab.
// An instance of this class is used for each item in that list.
// It communicates with the ConnectionPixmapItem on the map through a shared MapConnection pointer.
class ConnectionsListItem : public QFrame
{
    Q_OBJECT

public:
    explicit ConnectionsListItem(QWidget *parent, MapConnection *connection, const QStringList &mapNames);
    ~ConnectionsListItem();

    void setSelected(bool selected);

private:
    Ui::ConnectionsListItem *ui;
    QPointer<MapConnection> connection;
    Map *map;
    bool isSelected = false;
    unsigned actionId = 0;

    void updateUI();

protected:
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual bool eventFilter(QObject*, QEvent *event) override;

signals:
    void selected();
    void openMapClicked(MapConnection*);

private:
    void commitDirection();
    void commitMap(const QString &mapName);
    void commitMove(int offset);
    void commitRemove();
};

#endif // CONNECTIONSLISTITEM_H
