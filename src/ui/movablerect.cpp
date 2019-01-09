#include "movablerect.h"

MovableRect::MovableRect(bool *enabled, int width, int height, QRgb color)
{
    this->enabled = enabled;
    this->width = width;
    this->height = height;
    this->color = color;
    this->setVisible(*enabled);
}

void MovableRect::updateLocation(int x, int y)
{
    this->setX((x * 16) - this->width / 2 + 8);
    this->setY((y * 16) - this->height / 2 + 8);
    this->setVisible(*this->enabled);
}
