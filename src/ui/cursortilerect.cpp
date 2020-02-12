#include "cursortilerect.h"
#include "log.h"

CursorTileRect::CursorTileRect(bool *enabled, QRgb color)
{
    this->enabled = enabled;
    this->visible = true;
    this->color = color;
    this->width = 16;
    this->height = 16;
    this->smartPathMode = false;
    this->singleTileMode = false;
    this->anchored = false;
    this->rightClickSelectionAnchored = false;
    this->anchorCoordX = 0;
    this->anchorCoordY = 0;
    this->selectionWidth = 1;
    this->selectionHeight = 1;
}

void CursorTileRect::setVisibility(bool visible)
{
    this->visible = visible;
}

void CursorTileRect::initAnchor(int coordX, int coordY)
{
    this->anchorCoordX = coordX;
    this->anchorCoordY = coordY;
    this->anchored = true;
}

void CursorTileRect::stopAnchor()
{
    this->anchored = false;
}

void CursorTileRect::initRightClickSelectionAnchor(int coordX, int coordY)
{
    this->anchorCoordX = coordX;
    this->anchorCoordY = coordY;
    this->rightClickSelectionAnchored = true;
}

void CursorTileRect::stopRightClickSelectionAnchor()
{
    this->rightClickSelectionAnchored = false;
}

void CursorTileRect::updateSelectionSize(int width, int height)
{
    this->selectionWidth = width;
    this->selectionHeight = height;
    this->width = width * 16;
    this->height = height * 16;
    this->prepareGeometryChange();
    this->update();
}

void CursorTileRect::setSmartPathMode()
{
    this->smartPathMode = true;
}

void CursorTileRect::setSingleTileMode()
{
    this->singleTileMode = true;
}

void CursorTileRect::stopSingleTileMode()
{
    this->singleTileMode = false;
}

void CursorTileRect::setNormalPathMode()
{
    this->smartPathMode = false;
}

bool CursorTileRect::smartPathInEffect()
{
    return !this->rightClickSelectionAnchored && this->smartPathMode && this->selectionHeight == 3 && this->selectionWidth == 3;
}

void CursorTileRect::updateLocation(int coordX, int coordY)
{
    if (!this->singleTileMode) {
        if (this->rightClickSelectionAnchored) {
            coordX = qMin(coordX, this->anchorCoordX);
            coordY = qMin(coordY, this->anchorCoordY);
        }
        else if (this->anchored && !this->smartPathInEffect()) {
            int xDiff = coordX - this->anchorCoordX;
            int yDiff = coordY - this->anchorCoordY;
            if (xDiff < 0 && xDiff % this->selectionWidth != 0) xDiff -= this->selectionWidth;
            if (yDiff < 0 && yDiff % this->selectionHeight != 0) yDiff -= this->selectionHeight;

            coordX = this->anchorCoordX + (xDiff / this->selectionWidth) * this->selectionWidth;
            coordY = this->anchorCoordY + (yDiff / this->selectionHeight) * this->selectionHeight;
        }
    }

    coordX = qMax(coordX, 0);
    coordY = qMax(coordY, 0);
    this->setX(coordX * 16);
    this->setY(coordY * 16);
    this->setVisible(*this->enabled && this->visible);
}
