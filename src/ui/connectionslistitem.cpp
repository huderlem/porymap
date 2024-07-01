#include "connectionslistitem.h"
#include "ui_connectionslistitem.h"

static const QStringList directions = {"up", "down", "left", "right"};

ConnectionsListItem::ConnectionsListItem(QWidget *parent, const QStringList &mapNames) :
    QFrame(parent),
    ui(new Ui::ConnectionsListItem)
{
    ui->setupUi(this);

    const QSignalBlocker blocker1(ui->comboBox_Direction);
    const QSignalBlocker blocker2(ui->comboBox_Map);
    const QSignalBlocker blocker3(ui->spinBox_Offset);

    ui->comboBox_Direction->setEditable(false);
    ui->comboBox_Direction->setMinimumContentsLength(0);
    ui->comboBox_Direction->addItems(directions);

    ui->comboBox_Map->setMinimumContentsLength(6);
    ui->comboBox_Map->addItems(mapNames);

    /* TODO: Spin box limits
    ui->spinBox_ConnectionOffset->setMaximum(selected_connection_item->getMaxOffset());
    ui->spinBox_ConnectionOffset->setMinimum(selected_connection_item->getMinOffset());
    */

    // TODO:
    //connect(ui->button_Delete, &QAbstractButton::clicked, [this](bool) { this->d;});
}

void ConnectionsListItem::populate(const MapConnection * connection) {
    const QSignalBlocker blocker1(ui->comboBox_Direction);
    const QSignalBlocker blocker2(ui->comboBox_Map);
    const QSignalBlocker blocker3(ui->spinBox_Offset);

    ui->comboBox_Direction->setTextItem(connection->direction);
    ui->comboBox_Map->setTextItem(connection->map_name);
    ui->spinBox_Offset->setValue(connection->offset);
}

ConnectionsListItem::~ConnectionsListItem()
{
    delete ui;
}

// TODO
void ConnectionsListItem::on_comboBox_Direction_currentTextChanged(const QString &direction)
{
    /*editor->updateCurrentConnectionDirection(direction);
    markMapEdited();*/
}

// TODO
void ConnectionsListItem::on_comboBox_Map_currentTextChanged(const QString &mapName)
{
    /*if (mapName.isEmpty() || editor->project->mapNames.contains(mapName)) {
        editor->setConnectionMap(mapName);
        markMapEdited();
    }*/
}

// TODO
void ConnectionsListItem::on_spinBox_Offset_valueChanged(int offset)
{
    /*editor->updateConnectionOffset(offset);
    markMapEdited();*/
}

// TODO
void ConnectionsListItem::on_button_Delete_clicked()
{
    /*
    editor->removeCurrentConnection();
    markMapEdited();
    */
}

