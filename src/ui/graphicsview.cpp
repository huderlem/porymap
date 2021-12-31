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
    foreach (Overlay * overlay, this->overlayMap)
        overlay->renderItems(painter);

    if (!editor) return;

    QStyleOptionGraphicsItem option;
    for (QGraphicsLineItem* line : editor->gridLines) {
        if (line && line->isVisible())
            line->paint(painter, &option, this);
    }
    if (editor->playerViewRect && editor->playerViewRect->isVisible())
        editor->playerViewRect->paint(painter, &option, this);
    if (editor->cursorMapTileRect && editor->cursorMapTileRect->isVisible())
        editor->cursorMapTileRect->paint(painter, &option, this);
}

void MapView::clearOverlays() {
    foreach (Overlay * overlay, this->overlayMap) {
        overlay->clearItems();
        delete overlay;
    }
    this->overlayMap.clear();
}

void MapView::setOverlaysHidden(bool hidden) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setHidden(hidden);
}

void MapView::setOverlaysX(int x) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setX(x);
}

void MapView::setOverlaysY(int y) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setY(y);
}

void MapView::setOverlaysPosition(int x, int y) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setPosition(x, y);
}

void MapView::setOverlaysOpacity(int opacity) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setOpacity(opacity);
}

void MapView::moveOverlays(int deltaX, int deltaY) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->move(deltaX, deltaY);
}

Overlay * MapView::getOverlay(int layer) {
    Overlay * overlay = this->overlayMap.value(layer, nullptr);
    if (!overlay) {
        overlay = new Overlay();
        this->overlayMap.insert(layer, overlay);
    }
    return overlay;
}
