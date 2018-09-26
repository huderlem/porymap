#include "noscrollspinbox.h"

NoScrollSpinBox::NoScrollSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    // Don't let scrolling hijack focus.
    setFocusPolicy(Qt::StrongFocus);
}

void NoScrollSpinBox::wheelEvent(QWheelEvent *event)
{
    // Only allow scrolling to modify contents when it explicitly has focus.
    if (hasFocus())
        QSpinBox::wheelEvent(event);
}

