#include "mapsceneeventfilter.h"
#include <QEvent>
#include <QGraphicsSceneWheelEvent>

MapSceneEventFilter::MapSceneEventFilter(QObject *parent) : QObject(parent)
{

}

bool MapSceneEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneWheel)
    {
        QGraphicsSceneWheelEvent *wheelEvent = static_cast<QGraphicsSceneWheelEvent *>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier)
        {
            emit zoom(wheelEvent->delta() > 0 ? 1 : -1);
            event->accept();
            return true;
        }
    }
    return false;
}
