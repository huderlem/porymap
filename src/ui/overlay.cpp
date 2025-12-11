#ifdef QT_QML_LIB
#include "overlay.h"
#include "scripting.h"
#include "log.h"

void OverlayText::render(QPainter *painter) {
    QFont font = painter->font();
    font.setPixelSize(this->fontSize);
    painter->setFont(font);
    painter->setPen(this->color);
    painter->drawStaticText(this->x, this->y, this->text);
}

void OverlayPath::render(QPainter *painter) {
    painter->fillPath(this->path, this->fillColor);
    painter->setPen(this->borderColor);
    painter->drawPath(this->path);
}

void OverlayPixmap::render(QPainter *painter) {
    painter->drawPixmap(this->x, this->y, this->pixmap);
}

void Overlay::renderItems(QPainter *painter) {
    if (this->hidden) return;

    painter->save();

    // don't waste time if there are no updated to be made
    if (!valid) {
        QPainter overlayPainter(&this->image);
        overlayPainter.setClipping(false);
        QTransform t = overlayPainter.transform();
        t.translate(-this->image.offset().x(), -this->image.offset().y());
        overlayPainter.setTransform(t);
        for (auto item : this->items) {
            item->render(&overlayPainter);
        }
    }

    if (this->clippingRect) {
        painter->setClipping(true);
        painter->setClipRect(*this->clippingRect);
    }

    QTransform transform = painter->transform();
    transform.translate(this->x, this->y);
    transform.rotate(this->angle);
    transform.scale(this->hScale, this->vScale);
    painter->setTransform(transform);

    painter->setOpacity(this->opacity);
    painter->drawImage(this->image.offset(), this->image);

    painter->restore();

    valid = true;
}

void Overlay::clearItems() {
    for (auto item : this->items) {
        delete item;
    }
    this->items.clear();
    this->image.fill(QColor(0, 0, 0, 0));
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
        logError(QString("Invalid overlay opacity '%1', must be in range 0-100").arg(opacity));
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

qreal Overlay::getHScale() {
    return this->hScale;
}

qreal Overlay::getVScale() {
    return this->vScale;
}

void Overlay::setHScale(qreal scale) {
    this->hScale = scale;
}

void Overlay::setVScale(qreal scale) {
    this->vScale = scale;
}

void Overlay::setScale(qreal hScale, qreal vScale) {
    this->hScale = hScale;
    this->vScale = vScale;
}

int Overlay::getRotation() {
    return this->angle;
}

void Overlay::setRotation(int angle) {
    this->angle = angle;
    this->clampAngle();
}

void Overlay::rotate(int degrees) {
    this->angle += degrees;
    this->clampAngle();
}

void Overlay::clampAngle() {
    // transform.rotate would handle this already, but we only
    // want to report angles 0-359 for Overlay::getRotation
    this->angle %= 360;
    if (this->angle < 0)
        this->angle += 360;
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

QColor Overlay::getColor(QString colorStr) {
    if (colorStr.isEmpty())
        colorStr = "transparent";
    QColor color = QColor(colorStr);
    if (!color.isValid()) {
        logWarn(QString("Invalid overlay color \"%1\". Colors can be in the format \"#RRGGBB\" or \"#AARRGGBB\"").arg(colorStr));
        color = QColor("transparent");
    }
    return color;
}

void Overlay::addText(const QString text, int x, int y, QString colorStr, int fontSize) {
    this->items.append(new OverlayText(text, x, y, getColor(colorStr), fontSize));
}

bool Overlay::addRect(int x, int y, int width, int height, QString borderColorStr, QString fillColorStr, int rounding) {
    if (rounding < 0 || rounding > 100) {
        logError(QString("Invalid rectangle rounding '%1', must be in range 0-100").arg(rounding));
        return false;
    }

    QPainterPath path;
    path.addRoundedRect(QRectF(x, y, width, height), rounding, rounding, Qt::RelativeSize);
    this->items.append(new OverlayPath(path, getColor(borderColorStr), getColor(fillColorStr)));
    return true;
}

bool Overlay::addPath(QList<int> xCoords, QList<int> yCoords, QString borderColorStr, QString fillColorStr) {
    int numPoints = qMin(xCoords.length(), yCoords.length());
    if (numPoints < 2) {
        logError("Overlay path must have at least two points.");
        return false;
    }

    QPainterPath path;
    path.moveTo(xCoords.at(0), yCoords.at(0));

    for (int i = 1; i < numPoints; i++)
        path.lineTo(xCoords.at(i), yCoords.at(i));

    this->items.append(new OverlayPath(path, getColor(borderColorStr), getColor(fillColorStr)));
    return true;
}

bool Overlay::addImage(int x, int y, QString filepath, bool useCache, int width, int height, int xOffset, int yOffset, qreal hScale, qreal vScale, QList<QRgb> palette, bool setTransparency) {
    const QImage * baseImage = Scripting::getImage(filepath, useCache);
    if (!baseImage || baseImage->isNull()) {
        logError(QString("Failed to load image '%1'").arg(filepath));
        return false;
    }
    QImage image = *baseImage;

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

    this->items.append(new OverlayPixmap(x, y, QPixmap::fromImage(image)));
    return true;
}

bool Overlay::addImage(int x, int y, QImage image) {
    if (image.isNull()) {
        logError(QString("Failed to load custom image"));
        return false;
    }
    this->items.append(new OverlayPixmap(x, y, QPixmap::fromImage(image)));
    return true;
}


#endif // QT_QML_LIB
