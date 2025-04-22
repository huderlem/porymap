#include "divingmappixmapitem.h"
#include "config.h"

DivingMapPixmapItem::DivingMapPixmapItem(MapConnection *connection, QComboBox *combo)
    : QGraphicsPixmapItem(getBasePixmap(connection))
{
    m_connection = connection;
    m_combo = combo;

    setComboText(connection->targetMapName());

    // Update display if the connected map is swapped.
    connect(m_connection, &MapConnection::targetMapNameChanged, this, &DivingMapPixmapItem::onTargetMapChanged);
}

DivingMapPixmapItem::~DivingMapPixmapItem() {
    // Clear map name from combo box
    setComboText("");
}

QPixmap DivingMapPixmapItem::getBasePixmap(MapConnection* connection) {
    if (!connection)
        return QPixmap();
    if (!porymapConfig.showDiveEmergeMaps)
        return QPixmap(); // Save some rendering time if it won't be displayed
    if (connection->targetMapName() == connection->parentMapName())
        return QPixmap(); // If the map is connected to itself then rendering is pointless.
    return connection->render();
}

void DivingMapPixmapItem::updatePixmap() {
    setPixmap(getBasePixmap(m_connection));
}

void DivingMapPixmapItem::onTargetMapChanged() {
    updatePixmap();
    setComboText(m_connection->targetMapName());
}

void DivingMapPixmapItem::setComboText(const QString &text) {
    if (m_combo) m_combo->setCurrentText(text);
}
