#include "citymappixmapitem.h"
#include "imageproviders.h"

#include <QFile>
#include <QPainter>
#include <QDebug>



// read to byte array from filename
void CityMapPixmapItem::init() {
    // TODO: are they always 10x10 squares?
    width = 10;
    height = 10;

    QFile binFile(file);
    if (!binFile.open(QIODevice::ReadOnly)) return;

    data = binFile.readAll();
    binFile.close();
}

void CityMapPixmapItem::draw() {
    QImage image(width * 8, height * 8, QImage::Format_RGBA8888);

    QPainter painter(&image);
    for (int i = 0; i < data.size() / 2; i++) {
        QImage img = this->tile_selector->tileImg(data[i * 2]);// need to skip every other tile
        int x = i % width;
        int y = i / width;
        QPoint pos = QPoint(x * 8, y * 8);
        painter.drawImage(pos, img);
    }
    painter.end();

    this->setPixmap(QPixmap::fromImage(image));
}

void CityMapPixmapItem::save() {
    //
    QFile binFile(file);
    if (!binFile.open(QIODevice::WriteOnly)) return;
    binFile.write(data);
    binFile.close();
}

void CityMapPixmapItem::paint(QGraphicsSceneMouseEvent *event) {
    //
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    int index = 2 * (x + y * width);
    data[index] = static_cast<uint8_t>(this->tile_selector->selectedTile);

    draw();
}

void CityMapPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    emit mouseEvent(event, this);
}
