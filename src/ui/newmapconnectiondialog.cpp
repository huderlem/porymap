#include "newmapconnectiondialog.h"
#include "ui_newmapconnectiondialog.h"

NewMapConnectionDialog::NewMapConnectionDialog(QWidget *parent, Map* map, const QStringList &mapNames) :
    QDialog(parent),
    ui(new Ui::NewMapConnectionDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->comboBox_Direction->addItems(MapConnection::cardinalDirections);

    ui->comboBox_Map->addItems(mapNames);
    ui->comboBox_Map->setInsertPolicy(QComboBox::NoInsert);

    // Choose default direction
    QMap<QString, int> directionCounts;
    for (auto connection : map->getConnections()) {
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
    } else if (mapNames.first() == map->name() && mapNames.length() > 1) {
        // Prefer not to connect the map to itself
        defaultMapName = mapNames.at(1);
    } else {
        defaultMapName = mapNames.first();
    }
    ui->comboBox_Map->setTextItem(defaultMapName);

    connect(ui->comboBox_Map, &QComboBox::currentTextChanged, [this] {
        if (ui->label_Warning->isVisible() && mapNameIsValid())
            setWarningVisible(false);
    });
    setWarningVisible(false);
}

NewMapConnectionDialog::~NewMapConnectionDialog()
{
    delete ui;
}

bool NewMapConnectionDialog::mapNameIsValid() {
    return ui->comboBox_Map->findText(ui->comboBox_Map->currentText()) >= 0;
}

void NewMapConnectionDialog::setWarningVisible(bool visible) {
    ui->label_Warning->setVisible(visible);
    adjustSize();
}

void NewMapConnectionDialog::accept() {
    if (!mapNameIsValid()) {
        setWarningVisible(true);
        return;
    }
    emit accepted(new MapConnection(ui->comboBox_Map->currentText(), ui->comboBox_Direction->currentText()));
    QDialog::accept();
}
