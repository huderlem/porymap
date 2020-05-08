#include "overlay.h"

void OverlayText::render(QPainter *painter) {
    QFont font = painter->font();
    font.setPixelSize(this->fontSize);
    painter->setFont(font);
    painter->setPen(this->color);
    painter->drawText(this->x, this->y, this->text);
}

void OverlayRect::render(QPainter *painter) {
    if (this->filled) {
        painter->fillRect(this->x, this->y, this->width, this->height, this->color);
    } else {
        painter->setPen(this->color);
        painter->drawRect(this->x, this->y, this->width, this->height);
    }
}

void OverlayImage::render(QPainter *painter) {
    painter->drawImage(this->x, this->y, this->image);
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

void Overlay::addText(QString text, int x, int y, QString color, int fontSize) {
    this->items.append(new OverlayText(text, x, y, QColor(color), fontSize));
}

void Overlay::addRect(int x, int y, int width, int height, QString color, bool filled) {
    this->items.append(new OverlayRect(x, y, width, height, QColor(color), filled));
}

void Overlay::addImage(int x, int y, QString filepath) {
    this->items.append(new OverlayImage(x, y, QImage(filepath)));
}
