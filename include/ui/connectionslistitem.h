#ifndef CONNECTIONSLISTITEM_H
#define CONNECTIONSLISTITEM_H

#include "mapconnection.h"

#include <QFrame>

namespace Ui {
class ConnectionsListItem;
}

class ConnectionsListItem : public QFrame
{
    Q_OBJECT

public:
    explicit ConnectionsListItem(QWidget *parent, const QStringList &mapNames);
    ~ConnectionsListItem();

    void populate(const MapConnection * connection);

public:
    Ui::ConnectionsListItem *ui;
};

#endif // CONNECTIONSLISTITEM_H
