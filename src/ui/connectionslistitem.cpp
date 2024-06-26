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
