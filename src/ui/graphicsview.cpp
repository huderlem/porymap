#include "graphicsview.h"
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

void GraphicsView::drawForeground(QPainter *painter, const QRectF&) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->renderItems(painter);
}

void GraphicsView::clearOverlays() {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->clearItems();
}

void GraphicsView::setOverlaysHidden(bool hidden) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setHidden(hidden);
}

Overlay * GraphicsView::getOverlay(int layer) {
    Overlay * overlay = this->overlayMap.value(layer, nullptr);
    if (!overlay) {
        overlay = new Overlay();
        this->overlayMap.insert(layer, overlay);
    }
    return overlay;
}

void GraphicsView::moveEvent(QMoveEvent *event) {
    QGraphicsView::moveEvent(event);
    QLabel *label_MapRulerStatus = findChild<QLabel *>("label_MapRulerStatus", Qt::FindDirectChildrenOnly);
    if (label_MapRulerStatus && label_MapRulerStatus->isVisible())
        label_MapRulerStatus->move(mapToGlobal(QPoint(6, 6)));
}
