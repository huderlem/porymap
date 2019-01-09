#include "playerviewrect.h"

PlayerViewRect::PlayerViewRect(bool *enabled)
{
    this->enabled = enabled;
    this->setVisible(*enabled);
}

void PlayerViewRect::updateLocation(int x, int y)
{
    this->setX((x * 16) - (14 * 8));
    this->setY((y * 16) - (9 * 8));
    this->setVisible(*this->enabled);
}
