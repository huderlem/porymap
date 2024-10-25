#include "graphicsview.h"
#include "mapview.h"
#include "editor.h"

void GraphicsView::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);
    if (editor) {
        editor->objectsView_onMousePress(event);
    }
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::moveEvent(QMoveEvent *event) {
    QGraphicsView::moveEvent(event);
    QLabel *label_MapRulerStatus = findChild<QLabel *>("label_MapRulerStatus", Qt::FindDirectChildrenOnly);
    if (label_MapRulerStatus && label_MapRulerStatus->isVisible())
        label_MapRulerStatus->move(mapToGlobal(QPoint(6, 6)));
}

void MapView::drawForeground(QPainter *painter, const QRectF&) {
    for (auto i = this->overlayMap.constBegin(); i != this->overlayMap.constEnd(); i++) {
        i.value()->renderItems(painter);
    }

    if (!editor) return;

    QStyleOptionGraphicsItem option;

    // Draw elements of the map view that should always render on top of anything added by the user with the scripting API.

    // Draw map grid
    if (editor->mapGrid && editor->mapGrid->isVisible()) {
        painter->save();
        if (editor->map) {
            // We're clipping here to hide parts of the grid that are outside the map.
            const QRectF mapRect(-0.5, -0.5, editor->map->getWidth() * 16 + 1.5, editor->map->getHeight() * 16 + 1.5);
            painter->setClipping(true);
            painter->setClipRect(mapRect);
        }
        for (auto item : editor->mapGrid->childItems())
            item->paint(painter, &option, this);
        painter->restore();
    }

    // Draw cursor rectangles
    if (editor->playerViewRect && editor->playerViewRect->isVisible())
        editor->playerViewRect->paint(painter, &option, this);
    if (editor->cursorMapTileRect && editor->cursorMapTileRect->isVisible())
        editor->cursorMapTileRect->paint(painter, &option, this);
}

void MapView::clearOverlayMap() {
    for (auto i = this->overlayMap.constBegin(); i != this->overlayMap.constEnd(); i++) {
        delete i.value();
    }
    this->overlayMap.clear();
}

Overlay * MapView::getOverlay(int layer) {
    Overlay * overlay = this->overlayMap.value(layer, nullptr);
    if (!overlay) {
        overlay = new Overlay();
        this->overlayMap.insert(layer, overlay);
    }
    return overlay;
}
