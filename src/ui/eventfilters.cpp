#include "eventfilters.h"

#include <QGraphicsSceneWheelEvent>



bool WheelFilter::eventFilter(QObject *, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        return true;
    }
    return false;
}



bool MapSceneEventFilter::eventFilter(QObject*, QEvent *event) {
    if (event->type() == QEvent::GraphicsSceneWheel) {
        QGraphicsSceneWheelEvent *wheelEvent = static_cast<QGraphicsSceneWheelEvent *>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            emit wheelZoom(wheelEvent->delta() > 0 ? 1 : -1);
            event->accept();
            return true;
        }
    }
    return false;
}



bool ActiveWindowFilter::eventFilter(QObject*, QEvent *event) {
    if (event->type() == QEvent::WindowActivate) {
        emit activated();
    }
    return false;
}
