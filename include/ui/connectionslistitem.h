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

    void updateUI();
    void setSelected(bool selected);

private:
    Ui::ConnectionsListItem *ui;
    QPointer<MapConnection> connection;
    Map *map;
    bool isSelected = false;
    unsigned actionId = 0;

protected:
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void focusInEvent(QFocusEvent*) override;
    virtual void keyPressEvent(QKeyEvent*) override;

signals:
    void selected();
    void openMapClicked(MapConnection*);

private slots:
    void on_comboBox_Direction_currentTextChanged(QString direction);
    void on_comboBox_Map_currentTextChanged(QString mapName);
    void on_spinBox_Offset_valueChanged(int offset);
    void on_button_Delete_clicked();
    void on_button_OpenMap_clicked();
};

#endif // CONNECTIONSLISTITEM_H
