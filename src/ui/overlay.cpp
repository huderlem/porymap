#include "overlay.h"
#include "scripting.h"
#include "log.h"

void OverlayText::render(QPainter *painter, int x, int y) {
    QFont font = painter->font();
    font.setPixelSize(this->fontSize);
    painter->setFont(font);
    painter->setPen(this->color);
    painter->drawText(this->x + x, this->y + y, this->text);
}

void OverlayRect::render(QPainter *painter, int x, int y) {
    if (this->filled) {
        painter->fillRect(this->x + x, this->y + y, this->width, this->height, this->color);
    } else {
        painter->setPen(this->color);
        painter->drawRect(this->x + x, this->y + y, this->width, this->height);
    }
}

void OverlayImage::render(QPainter *painter, int x, int y) {
    painter->drawImage(this->x + x, this->y + y, this->image);
}

void Overlay::renderItems(QPainter *painter) {
    if (this->hidden) return;

    if (this->clippingRect) {
        painter->setClipping(true);
        painter->setClipRect(*this->clippingRect);
    }

    qreal oldOpacity = painter->opacity();
    painter->setOpacity(this->opacity);
    for (auto item : this->items)
        item->render(painter, this->x, this->y);
    painter->setOpacity(oldOpacity);

    if (this->clippingRect) {
        painter->setClipping(false);
    }
}

void Overlay::clearItems() {
    for (auto item : this->items) {
        delete item;
    }
    this->items.clear();
}

QList<OverlayItem*> Overlay::getItems() {
    return this->items;
}

bool Overlay::getHidden() {
    return this->hidden;
}

void Overlay::setHidden(bool hidden) {
    this->hidden = hidden;
}

int Overlay::getOpacity() {
    return this->opacity * 100;
}

void Overlay::setOpacity(int opacity) {
    if (opacity < 0 || opacity > 100) {
        logError(QString("Invalid overlay opacity '%1'").arg(opacity));
        return;
    }
    this->opacity = static_cast<qreal>(opacity) / 100;
}

int Overlay::getX() {
    return this->x;
}

int Overlay::getY() {
    return this->y;
}

void Overlay::setX(int x) {
    this->x = x;
}

void Overlay::setY(int y) {
    this->y = y;
}

void Overlay::setClippingRect(QRectF rect) {
    if (this->clippingRect) {
        delete this->clippingRect;
    }
    this->clippingRect = new QRectF(rect);
}

void Overlay::clearClippingRect() {
    if (this->clippingRect) {
        delete this->clippingRect;
    }
    this->clippingRect = nullptr;
}

void Overlay::setPosition(int x, int y) {
    this->x = x;
    this->y = y;
}

void Overlay::move(int deltaX, int deltaY) {
    this->x += deltaX;
    this->y += deltaY;
}

void Overlay::addText(QString text, int x, int y, QString color, int fontSize) {
    this->items.append(new OverlayText(text, x, y, QColor(color), fontSize));
}

void Overlay::addRect(int x, int y, int width, int height, QString color, bool filled) {
    this->items.append(new OverlayRect(x, y, width, height, QColor(color), filled));
}

bool Overlay::addImage(int x, int y, QString filepath, bool useCache, int width, int height, int xOffset, int yOffset, qreal hScale, qreal vScale, QList<QRgb> palette, bool setTransparency) {
    QImage image = useCache ? Scripting::getImage(filepath) : QImage(filepath);
    if (image.isNull()) {
        logError(QString("Failed to load image '%1'").arg(filepath));
        return false;
    }

    int fullWidth = image.width();
    int fullHeight = image.height();

    // Negative values used as an indicator for "use full dimension"
    if (width <= 0)
        width = fullWidth;
    if (height <= 0)
        height = fullHeight;

    if (xOffset < 0) xOffset = 0;
    if (yOffset < 0) yOffset = 0;

    if (width + xOffset > fullWidth || height + yOffset > fullHeight) {
        logError(QString("%1x%2 image starting at (%3,%4) exceeds the image size for '%5'")
                 .arg(width)
                 .arg(height)
                 .arg(xOffset)
                 .arg(yOffset)
                 .arg(filepath));
        return false;
    }

    // Get specified subset of image
    if (width != fullWidth || height != fullHeight)
        image = image.copy(xOffset, yOffset, width, height);

    if (hScale != 1 || vScale != 1)
        image = image.transformed(QTransform().scale(hScale, vScale));

    for (int i = 0; i < palette.size(); i++)
        image.setColor(i, palette.at(i));

    if (setTransparency)
        image.setColor(0, qRgba(0, 0, 0, 0));

    this->items.append(new OverlayImage(x, y, image));
    return true;
}

bool Overlay::addImage(int x, int y, QImage image) {
    if (image.isNull()) {
        logError(QString("Failed to load custom image"));
        return false;
    }
    this->items.append(new OverlayImage(x, y, image));
    return true;
}
