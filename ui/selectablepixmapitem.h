#ifndef SELECTABLEPIXMAPITEM_H
#define SELECTABLEPIXMAPITEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class SelectablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    SelectablePixmapItem(int cellWidth, int cellHeight, int maxSelectionWidth, int maxSelectionHeight) {
        this->cellWidth = cellWidth;
        this->cellHeight = cellHeight;
        this->maxSelectionWidth = maxSelectionWidth;
        this->maxSelectionHeight = maxSelectionHeight;
    }
    QPoint getSelectionDimensions();
    QPoint getSelectionStart();
    virtual void draw() = 0;

protected:
    int cellWidth;
    int cellHeight;
    int maxSelectionWidth;
    int maxSelectionHeight;
    int selectionInitialX;
    int selectionInitialY;
    int selectionOffsetX;
    int selectionOffsetY;

    void select(int, int, int, int);
    void updateSelection(int, int);
    QPoint getCellPos(QPointF);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void drawSelection();
};

#endif // SELECTABLEPIXMAPITEM_H
