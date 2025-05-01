#include "connectionslistitem.h"
#include "ui_connectionslistitem.h"
#include "editcommands.h"
#include "map.h"

#include <QLineEdit>

ConnectionsListItem::ConnectionsListItem(QWidget *parent, MapConnection * connection, const QStringList &mapNames) :
    QFrame(parent),
    ui(new Ui::ConnectionsListItem),
    connection(connection),
    map(connection->parentMap())
{
    ui->setupUi(this);
    setFocusPolicy(Qt::StrongFocus);

    // Direction
    const QSignalBlocker b_Direction(ui->comboBox_Direction);
    ui->comboBox_Direction->setMinimumContentsLength(0);
    ui->comboBox_Direction->addItems(MapConnection::cardinalDirections);
    ui->comboBox_Direction->installEventFilter(this);

    connect(ui->comboBox_Direction, &NoScrollComboBox::editingFinished, this, &ConnectionsListItem::commitDirection);

    // Map
    const QSignalBlocker b_Map(ui->comboBox_Map);
    ui->comboBox_Map->setMinimumContentsLength(6);
    ui->comboBox_Map->addItems(mapNames);
    ui->comboBox_Map->setFocusedScrollingEnabled(false); // Scrolling could cause rapid changes to many different maps
    ui->comboBox_Map->setInsertPolicy(QComboBox::NoInsert);
    ui->comboBox_Map->installEventFilter(this);

    connect(ui->comboBox_Map, &QComboBox::currentTextChanged, this, &ConnectionsListItem::commitMap);

    // Invalid map names are not considered a change. If editing finishes with an invalid name, restore the previous name.
    connect(ui->comboBox_Map->lineEdit(), &QLineEdit::editingFinished, [this] {
        const QSignalBlocker b(ui->comboBox_Map);
        if (this->connection && ui->comboBox_Map->findText(ui->comboBox_Map->currentText()) < 0)
            ui->comboBox_Map->setTextItem(this->connection->targetMapName());
    });

    // Offset
    const QSignalBlocker b_Offset(ui->spinBox_Offset);
    ui->spinBox_Offset->setMinimum(INT_MIN);
    ui->spinBox_Offset->setMaximum(INT_MAX);
    ui->spinBox_Offset->installEventFilter(this);

    connect(ui->spinBox_Offset, &QSpinBox::editingFinished, [this] { this->actionId++; }); // Distinguish between move actions for the edit history
    connect(ui->spinBox_Offset, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConnectionsListItem::commitMove);

    // If the connection changes externally we want to update to reflect the change.
    connect(connection, &MapConnection::offsetChanged, this, &ConnectionsListItem::updateUI);
    connect(connection, &MapConnection::directionChanged, this, &ConnectionsListItem::updateUI);
    connect(connection, &MapConnection::targetMapNameChanged, this, &ConnectionsListItem::updateUI);

    connect(ui->button_Delete, &QToolButton::clicked, this, &ConnectionsListItem::commitRemove);
    connect(ui->button_OpenMap, &QToolButton::clicked, [this] { emit openMapClicked(this->connection); });

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

bool ConnectionsListItem::eventFilter(QObject*, QEvent *event) {
    if (event->type() == QEvent::FocusIn)
        this->setSelected(true);
    return false;
}

void ConnectionsListItem::setSelected(bool selected) {
    if (selected == this->isSelected)
        return;
    this->isSelected = selected;

    this->setStyleSheet(selected ? QStringLiteral(".ConnectionsListItem { border: 1px solid rgb(255, 0, 255); }")
                                 : QStringLiteral(".ConnectionsListItem { border-width: 1px; }"));
    if (selected)
        emit this->selected();
}

void ConnectionsListItem::mousePressEvent(QMouseEvent *) {
    this->setSelected(true);
}

void ConnectionsListItem::commitDirection() {
    const QString direction = ui->comboBox_Direction->currentText();
    if (!this->connection || this->connection->direction() == direction)
        return;

    if (MapConnection::isDiving(direction)) {
        // Diving maps are displayed separately, no support right now for replacing a list item with a diving map.
        // For now just restore the original direction.
        ui->comboBox_Direction->setTextItem(this->connection->direction());
        return;
    }

    if (this->map) {
        this->map->commit(new MapConnectionChangeDirection(this->connection, direction));
    }
}

void ConnectionsListItem::commitMap(const QString &mapName) {
    if (this->map && ui->comboBox_Map->findText(mapName) >= 0)
        this->map->commit(new MapConnectionChangeMap(this->connection, mapName));
}

void ConnectionsListItem::commitMove(int offset) {
    if (this->map)
        this->map->commit(new MapConnectionMove(this->connection, offset, this->actionId));
}

void ConnectionsListItem::commitRemove() {
    if (this->map)
        this->map->commit(new MapConnectionRemove(this->map, this->connection));
}
