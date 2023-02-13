#include "eventfilters.h"



bool WheelFilter::eventFilter(QObject *, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        return true;
    }
    return false;
}
