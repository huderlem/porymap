#include "connectionslistitem.h"
#include "ui_connectionslistitem.h"
#include "editcommands.h"
#include "map.h"

#include <QLineEdit>

ConnectionsListItem::ConnectionsListItem(QWidget *parent, MapConnection * connection, const QStringList &mapNames) :
    QFrame(parent),
    ui(new Ui::ConnectionsListItem)
{
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);

    const QSignalBlocker blocker1(ui->comboBox_Direction);
    const QSignalBlocker blocker2(ui->comboBox_Map);
    const QSignalBlocker blocker3(ui->spinBox_Offset);

    ui->comboBox_Direction->setEditable(false);
    ui->comboBox_Direction->setMinimumContentsLength(0);
    ui->comboBox_Direction->addItems(MapConnection::cardinalDirections);

    ui->comboBox_Map->setMinimumContentsLength(6);
    ui->comboBox_Map->addItems(mapNames);
    ui->comboBox_Map->setFocusedScrollingEnabled(false); // Scrolling could cause rapid changes to many different maps
    ui->comboBox_Map->setInsertPolicy(QComboBox::NoInsert);

    ui->spinBox_Offset->setMinimum(INT_MIN);
    ui->spinBox_Offset->setMaximum(INT_MAX);

    // Invalid map names are not considered a change. If editing finishes with an invalid name, restore the previous name.
    connect(ui->comboBox_Map->lineEdit(), &QLineEdit::editingFinished, [this] {
        const QSignalBlocker blocker(ui->comboBox_Map);
        if (ui->comboBox_Map->findText(ui->comboBox_Map->currentText()) < 0)
            ui->comboBox_Map->setTextItem(this->connection->targetMapName());
    });

    // Distinguish between move actions for the edit history
    connect(ui->spinBox_Offset, &QSpinBox::editingFinished, [this] { this->actionId++; });

    this->connection = connection;
    this->map = connection->parentMap();
    this->updateUI();
}

ConnectionsListItem::~ConnectionsListItem()
{
    delete ui;
}

void ConnectionsListItem::updateUI() {
    if (!this->connection)
        return;

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

void ConnectionsListItem::on_comboBox_Direction_currentTextChanged(QString direction) {
    this->setSelected(true);
    if (this->map)
        this->map->commit(new MapConnectionChangeDirection(this->connection, direction));
}

void ConnectionsListItem::on_comboBox_Map_currentTextChanged(QString mapName) {
    this->setSelected(true);
    if (this->map && ui->comboBox_Map->findText(mapName) >= 0)
        this->map->commit(new MapConnectionChangeMap(this->connection, mapName));
}

void ConnectionsListItem::on_spinBox_Offset_valueChanged(int offset) {
    this->setSelected(true);
    if (this->map)
        this->map->commit(new MapConnectionMove(this->connection, offset, this->actionId));
}

void ConnectionsListItem::on_button_Delete_clicked() {
    if (this->map)
        this->map->commit(new MapConnectionRemove(this->map, this->connection));
}

void ConnectionsListItem::on_button_OpenMap_clicked() {
    emit openMapClicked(this->connection);
}

void ConnectionsListItem::focusInEvent(QFocusEvent* event) {
    this->setSelected(true);
    QFrame::focusInEvent(event);
}

void ConnectionsListItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        on_button_Delete_clicked();
    } else {
        QFrame::keyPressEvent(event);
    }
}
