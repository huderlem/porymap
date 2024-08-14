#include "connectionslistitem.h"
#include "ui_connectionslistitem.h"

ConnectionsListItem::ConnectionsListItem(QWidget *parent, MapConnection * connection, const QStringList &mapNames) :
    QFrame(parent),
    ui(new Ui::ConnectionsListItem)
{
    ui->setupUi(this);

    const QSignalBlocker blocker1(ui->comboBox_Direction);
    const QSignalBlocker blocker2(ui->comboBox_Map);
    const QSignalBlocker blocker3(ui->spinBox_Offset);

    ui->comboBox_Direction->setEditable(false);
    ui->comboBox_Direction->setMinimumContentsLength(0);
    ui->comboBox_Direction->addItems(MapConnection::cardinalDirections);

    ui->comboBox_Map->setMinimumContentsLength(6);
    ui->comboBox_Map->addItems(mapNames);
    ui->comboBox_Map->setFocusedScrollingEnabled(false); // Scrolling could cause rapid changes to many different maps

    ui->spinBox_Offset->setMinimum(INT_MIN);
    ui->spinBox_Offset->setMaximum(INT_MAX);

    this->connection = connection;
    this->updateUI();
}

ConnectionsListItem::~ConnectionsListItem()
{
    delete ui;
}

void ConnectionsListItem::updateUI() {
    const QSignalBlocker blocker1(ui->comboBox_Direction);
    const QSignalBlocker blocker2(ui->comboBox_Map);
    const QSignalBlocker blocker3(ui->spinBox_Offset);

    ui->comboBox_Direction->setTextItem(this->connection->direction());
    ui->comboBox_Map->setTextItem(this->connection->targetMapName());
    ui->spinBox_Offset->setValue(this->connection->offset());
}

void ConnectionsListItem::setSelected(bool selected) {
    if (selected == this->isSelected)
        return;
    this->isSelected = selected;

    this->setStyleSheet(selected ? ".ConnectionsListItem { border: 1px solid rgb(255, 0, 255); }"
                                 : ".ConnectionsListItem { border-width: 1px; }");
    if (selected)
        emit this->selected();
}

void ConnectionsListItem::mousePressEvent(QMouseEvent *) {
    this->setSelected(true);
}

void ConnectionsListItem::mouseDoubleClickEvent(QMouseEvent *) {
    emit doubleClicked(this->connection);
}

void ConnectionsListItem::on_comboBox_Direction_currentTextChanged(const QString &direction) {
    this->setSelected(true);
    this->connection->setDirection(direction);
}

void ConnectionsListItem::on_comboBox_Map_currentTextChanged(const QString &mapName) {
    this->setSelected(true);
    if (ui->comboBox_Map->findText(mapName) >= 0)
        this->connection->setTargetMapName(mapName);
}

void ConnectionsListItem::on_spinBox_Offset_valueChanged(int offset) {
    this->setSelected(true);
    this->connection->setOffset(offset);
}

void ConnectionsListItem::on_button_Delete_clicked() {
    emit this->removed(this->connection);
}
