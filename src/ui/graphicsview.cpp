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
    for (auto item : this->overlay.getItems()) {
        item->render(painter);
    }
}

void GraphicsView::moveEvent(QMoveEvent *event) {
    QGraphicsView::moveEvent(event);
    QLabel *label_MapRulerStatus = findChild<QLabel *>("label_MapRulerStatus", Qt::FindDirectChildrenOnly);
    if (label_MapRulerStatus && label_MapRulerStatus->isVisible())
        label_MapRulerStatus->move(mapToGlobal(QPoint(6, 6)));
}
