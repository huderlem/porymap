#include "connectionslistitem.h"
#include "ui_connectionslistitem.h"

static const QStringList directions = {"up", "down", "left", "right"};

ConnectionsListItem::ConnectionsListItem(QWidget *parent, MapConnection * connection, const QStringList &mapNames) :
    QFrame(parent),
    ui(new Ui::ConnectionsListItem),
    connection(connection)
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

    ui->spinBox_Offset->setMinimum(INT_MIN);
    ui->spinBox_Offset->setMaximum(INT_MAX);

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

    ui->comboBox_Direction->setTextItem(this->connection->direction);
    ui->comboBox_Map->setTextItem(this->connection->map_name);
    ui->spinBox_Offset->setValue(this->connection->offset);
}

// TODO: Frame shifts slightly when style changes
void ConnectionsListItem::setSelected(bool selected) {
    if (selected == this->isSelected)
        return;
    this->isSelected = selected;

    this->setStyleSheet(selected ? ".ConnectionsListItem { border: 1px solid rgb(255, 0, 255); }" : "");
    if (selected)
        emit this->selected();
}

void ConnectionsListItem::mousePressEvent(QMouseEvent *) {
    this->setSelected(true);
}

void ConnectionsListItem::on_comboBox_Direction_currentTextChanged(const QString &direction)
{
    this->connection->direction = direction;
    this->setSelected(true);
    emit this->edited();
}

void ConnectionsListItem::on_comboBox_Map_currentTextChanged(const QString &mapName)
{
    if (ui->comboBox_Map->findText(mapName) >= 0) {
        this->connection->map_name = mapName;
        this->setSelected(true);
        emit this->edited();
    }
}

void ConnectionsListItem::on_spinBox_Offset_valueChanged(int offset)
{
    this->connection->offset = offset;
    this->setSelected(true);
    emit this->edited();
}

void ConnectionsListItem::on_button_Delete_clicked()
{
    this->deleteLater();
    emit this->deleted();
}
