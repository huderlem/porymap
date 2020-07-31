#include "noscrollspinbox.h"

unsigned actionId = 0xffff;

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

void NoScrollSpinBox::focusOutEvent(QFocusEvent *event) {
    actionId++;
    QSpinBox::focusOutEvent(event);
}

unsigned NoScrollSpinBox::getActionId() {
    return actionId;
}
