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
    foreach (Overlay * overlay, this->overlayMap) {
        overlay->clearItems();
        delete overlay;
    }
    this->overlayMap.clear();
}

void GraphicsView::setOverlaysHidden(bool hidden) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setHidden(hidden);
}

void GraphicsView::setOverlaysX(int x) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setX(x);
}

void GraphicsView::setOverlaysY(int y) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setY(y);
}

void GraphicsView::setOverlaysPosition(int x, int y) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->setPosition(x, y);
}

void GraphicsView::moveOverlays(int deltaX, int deltaY) {
    foreach (Overlay * overlay, this->overlayMap)
        overlay->move(deltaX, deltaY);
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
