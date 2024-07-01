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

private slots:
    void on_comboBox_Direction_currentTextChanged(const QString &direction);
    void on_comboBox_Map_currentTextChanged(const QString &mapName);
    void on_spinBox_Offset_valueChanged(int offset);
    void on_button_Delete_clicked();
};

#endif // CONNECTIONSLISTITEM_H
