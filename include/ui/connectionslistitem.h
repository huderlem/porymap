#ifndef CONNECTIONSLISTITEM_H
#define CONNECTIONSLISTITEM_H

#include "mapconnection.h"

#include <QFrame>

namespace Ui {
class ConnectionsListItem;
}

// We show the data for each map connection in the panel on the right side of the Connections tab.
// An instance of this class is used for each item in that list.
// It communicates with the ConnectionPixmapItem on the map through a shared MapConnection pointer,
// and the two classes should signal one another when they change this data.
class ConnectionsListItem : public QFrame
{
    Q_OBJECT

public:
    explicit ConnectionsListItem(QWidget *parent, MapConnection * connection, const QStringList &mapNames);
    ~ConnectionsListItem();

    void updateUI();

private:
    Ui::ConnectionsListItem *ui;
    MapConnection * const connection;

signals:
    void edited();
    void deleted();

private slots:
    void on_comboBox_Direction_currentTextChanged(const QString &direction);
    void on_comboBox_Map_currentTextChanged(const QString &mapName);
    void on_spinBox_Offset_valueChanged(int offset);
    void on_button_Delete_clicked();
};

#endif // CONNECTIONSLISTITEM_H
