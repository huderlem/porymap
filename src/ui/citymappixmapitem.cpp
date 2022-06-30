#include "citymappixmapitem.h"
#include "imageproviders.h"
#include "config.h"
#include "log.h"

#include <QFile>
#include <QPainter>
#include <QDebug>

void CityMapPixmapItem::init() {
    width_ = 10;
    height_ = 10;

    QFile binFile(file);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    data = binFile.readAll();
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
        for (int i = 0; i < data.size(); i++)
            data[i] = data[i] ^ 0x80;
    }

    binFile.close();
}

void CityMapPixmapItem::draw() {
    QImage image(width_ * 8, height_ * 8, QImage::Format_RGBA8888);

    // TODO: construct temporary tile from this based on the id?
    // QPainter painter(&image);
    // for (int i = 0; i < data.size() / 2; i++) {
    //     QImage img = this->tile_selector->tileImg(data[i * 2]);// need to skip every other tile
    //     int x = i % width_;
    //     int y = i / width_;
    //     QPoint pos = QPoint(x * 8, y * 8);
    //     painter.drawImage(pos, img);
    // }
    // painter.end();

    this->setPixmap(QPixmap::fromImage(image));
}

void CityMapPixmapItem::save() {
    QFile binFile(file);
    if (!binFile.open(QIODevice::WriteOnly)) {
        logError(QString("Cannot save city map tilemap to %1.").arg(file));
        return;
    }
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby) {
        for (int i = 0; i < data.size(); i++)
            data[i] = data[i] ^ 0x80;
    }
    binFile.write(data);
    binFile.close();
}

void CityMapPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    int index = getIndexAt(x, y);
    data[index] = static_cast<uint8_t>(this->tile_selector->selectedTile);

    draw();
}

void CityMapPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

void CityMapPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    if (x < width_  && x >= 0
     && y < height_ && y >= 0) {
        emit this->hoveredRegionMapTileChanged(x, y);
        emit mouseEvent(event, this);
    }
}

void CityMapPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    emit mouseEvent(event, this);
}

QVector<uint8_t> CityMapPixmapItem::getTiles() {
    QVector<uint8_t> tiles;
    for (auto tile : data) {
        tiles.append(tile);
    }
    return tiles;
}

void CityMapPixmapItem::setTiles(QVector<uint8_t> tiles) {
    QByteArray newData;
    for (auto tile : tiles) {
        newData.append(tile);
    }
    this->data = newData;
}

int CityMapPixmapItem::getIndexAt(int x, int y) {
    return 2 * (x + y * this->width_);
}

int CityMapPixmapItem::width() {
    return this->width_;
}

int CityMapPixmapItem::height() {
    return this->height_;
}
