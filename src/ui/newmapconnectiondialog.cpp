#include "newmapconnectiondialog.h"
#include "ui_newmapconnectiondialog.h"
#include "message.h"

NewMapConnectionDialog::NewMapConnectionDialog(QWidget *parent, Map* map, const QStringList &mapNames) :
    QDialog(parent),
    ui(new Ui::NewMapConnectionDialog),
    m_map(map)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->comboBox_Direction->addItems(MapConnection::cardinalDirections);

    ui->comboBox_Map->addItems(mapNames);
    ui->comboBox_Map->setInsertPolicy(QComboBox::NoInsert);

    // Choose default direction
    QMap<QString, int> directionCounts;
    for (auto connection : m_map->getConnections()) {
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
    } else if (mapNames.first() == m_map->name() && mapNames.length() > 1) {
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

bool NewMapConnectionDialog::askReplaceConnection(MapConnection *connection, const QString &newMapName) {
    QString message = QString("%1 already has a %2 connection to '%3'. Replace it with a %2 connection to '%4'?")
                                .arg(m_map->name())
                                .arg(connection->direction())
                                .arg(connection->targetMapName())
                                .arg(newMapName);
    return QuestionMessage::show(message, this) == QMessageBox::Yes;
}

void NewMapConnectionDialog::accept() {
    if (!mapNameIsValid()) {
        setWarningVisible(true);
        return;
    }

    const QString direction = ui->comboBox_Direction->currentText();
    const QString targetMapName = ui->comboBox_Map->currentText();

    // This is a very niche use case. Normally the user should add Dive/Emerge map connections using the line edits at the top of
    // the Connections tab, but because we allow custom direction names in this dialog's Direction drop-down, a user could type
    // in "dive" or "emerge" and we have to decide what to do. If there's no existing Dive/Emerge map we can just add it normally
    // as if they had typed in the regular line edits. If there's already an existing connection we need to replace it.
    if (MapConnection::isDiving(direction)) {
        MapConnection *connection = m_map->getConnection(direction);
        if (connection) {
            if (connection->targetMapName() != targetMapName) {
                if (!askReplaceConnection(connection, targetMapName))
                    return; // Canceled
                emit connectionReplaced(targetMapName, direction);
            }
            // Replaced the diving connection (or no-op, if adding a diving connection with the same map name)
            QDialog::accept();
            return;
        }
        // Adding a new diving connection that doesn't exist yet, proceed normally.
    }

    emit newConnectionedAdded(targetMapName, direction);
    QDialog::accept();
}
