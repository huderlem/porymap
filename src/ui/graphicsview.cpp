#include "graphicsview.h"
#include "mapview.h"
#include "editor.h"

void MapView::moveEvent(QMoveEvent *event) {
    QGraphicsView::moveEvent(event);
    QLabel *label_MapRulerStatus = findChild<QLabel *>("label_MapRulerStatus", Qt::FindDirectChildrenOnly);
    if (label_MapRulerStatus && label_MapRulerStatus->isVisible())
        label_MapRulerStatus->move(mapToGlobal(QPoint(6, 6)));
}

void MapView::keyPressEvent(QKeyEvent *event) {
    if (editor && (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)) {
        editor->deleteSelectedEvents();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
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
        if (editor->layout) {
            // We're clipping here to hide parts of the grid that are outside the map.
            const QRectF mapRect(-0.5, -0.5, editor->layout->pixelWidth() + 1.5, editor->layout->pixelHeight() + 1.5);
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
        overlay = new Overlay(this->sceneRect().toRect());
        this->overlayMap.insert(layer, overlay);
    }
    return overlay;
}

void ConnectionsView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        emit pressedDelete();
        event->accept();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}
