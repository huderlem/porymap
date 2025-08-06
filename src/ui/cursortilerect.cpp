#include "cursortilerect.h"
#include "layoutpixmapitem.h"
#include "log.h"

CursorTileRect::CursorTileRect(const QSize &tileSize, const QRgb &color, QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_tileSize(tileSize),
      m_selectionSize(QSize(1,1)),
      m_anchorCoord(QPoint(0,0)),
      m_color(color)
{ }

// Size of the cursor may be explicitly enforced depending on settings.
QSize CursorTileRect::size() const {
    if (m_singleTileMode)
        return m_tileSize;

    if (smartPathInEffect())
        return m_tileSize * 2;

    return QSize(m_tileSize.width() * m_selectionSize.width(),
                 m_tileSize.height() * m_selectionSize.height());
}

void CursorTileRect::initAnchor(int coordX, int coordY) {
    m_anchorCoord = QPoint(coordX, coordY);
    m_anchored = true;
}

void CursorTileRect::stopAnchor() {
    m_anchored = false;
}

void CursorTileRect::initRightClickSelectionAnchor(int coordX, int coordY) {
    m_anchorCoord = QPoint(coordX, coordY);
    m_rightClickSelectionAnchored = true;
}

void CursorTileRect::stopRightClickSelectionAnchor() {
    m_rightClickSelectionAnchored = false;
}

void CursorTileRect::updateSelectionSize(const QSize &size) {
    m_selectionSize = size.expandedTo(QSize(1,1)); // Enforce minimum of 1x1 cell
    prepareGeometryChange();
    update();
}

bool CursorTileRect::smartPathInEffect() const {
    return !m_rightClickSelectionAnchored && m_smartPathMode && LayoutPixmapItem::isSmartPathSize(m_selectionSize);
}

void CursorTileRect::updateLocation(int coordX, int coordY) {
    if (!m_singleTileMode) {
        if (m_rightClickSelectionAnchored) {
            coordX = qMin(coordX, m_anchorCoord.x());
            coordY = qMin(coordY, m_anchorCoord.y());
        } else if (m_anchored && !smartPathInEffect()) {
            int xDiff = coordX - m_anchorCoord.x();
            int yDiff = coordY - m_anchorCoord.y();
            if (xDiff < 0 && xDiff % m_selectionSize.width() != 0) xDiff -= m_selectionSize.width();
            if (yDiff < 0 && yDiff % m_selectionSize.height() != 0) yDiff -= m_selectionSize.height();

            coordX = m_anchorCoord.x() + (xDiff / m_selectionSize.width()) * m_selectionSize.width();
            coordY = m_anchorCoord.y() + (yDiff / m_selectionSize.height()) * m_selectionSize.height();
        }
    }

    this->setX(coordX * m_tileSize.width());
    this->setY(coordY * m_tileSize.height());
}
