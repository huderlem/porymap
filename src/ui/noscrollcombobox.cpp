#include "noscrollcombobox.h"

NoScrollComboBox::NoScrollComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // Don't let scrolling hijack focus.
    setFocusPolicy(Qt::StrongFocus);
}

void NoScrollComboBox::wheelEvent(QWheelEvent *event)
{
    // Only allow scrolling to modify contents when it explicitly has focus.
    if (hasFocus())
        QComboBox::wheelEvent(event);
}
