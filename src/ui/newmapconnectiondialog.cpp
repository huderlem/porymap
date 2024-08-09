#include "newmapconnectiondialog.h"
#include "ui_newmapconnectiondialog.h"

NewMapConnectionDialog::NewMapConnectionDialog(QWidget *parent, Map* map, const QStringList &mapNames) :
    QDialog(parent),
    ui(new Ui::NewMapConnectionDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    this->map = map;
    this->result = nullptr;

    ui->comboBox_Direction->setEditable(false);
    ui->comboBox_Direction->setMinimumContentsLength(0);
    ui->comboBox_Direction->addItems(MapConnection::cardinalDirections);

    ui->comboBox_Map->setMinimumContentsLength(6);
    ui->comboBox_Map->addItems(mapNames);

    // Choose default direction
    QMap<QString, int> directionCounts;
    for (auto connection : map->connections) {
        directionCounts[connection->direction()]++;
    }
    QString defaultDirection;
    int minCount = INT_MAX;
    for (auto direction : MapConnection::cardinalDirections) {
        if (directionCounts[direction] < minCount) {
            defaultDirection = direction;
            minCount = directionCounts[direction];
        }
    }
    ui->comboBox_Direction->setTextItem(defaultDirection);

    // Choose default map
    QString defaultMapName;
    if (mapNames.isEmpty()) {
        defaultMapName = QString();
    } else if (mapNames.first() == map->name && mapNames.length() > 1) {
        // Prefer not to connect the map to itself
        defaultMapName = mapNames.at(1);
    } else {
        defaultMapName = mapNames.first();
    }
    ui->comboBox_Map->setTextItem(defaultMapName);
}

NewMapConnectionDialog::~NewMapConnectionDialog()
{
    delete ui;
}

void NewMapConnectionDialog::accept() {
    QString direction = ui->comboBox_Direction->currentText();
    QString hostMapName = this->map ? this->map->name : QString();
    QString targetMapName = ui->comboBox_Map->currentText();

    this->result = new MapConnection(direction, this->map->name, targetMapName);

    QDialog::accept();
}
