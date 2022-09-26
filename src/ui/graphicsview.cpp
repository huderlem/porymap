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

void MapView::clearOverlayMap() {
    foreach (Overlay * overlay, this->overlayMap) {
        overlay->clearItems();
        delete overlay;
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
