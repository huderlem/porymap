#ifndef SELECTABLEPIXMAPITEM_H
#define SELECTABLEPIXMAPITEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class SelectablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    SelectablePixmapItem(int cellWidth, int cellHeight): SelectablePixmapItem(cellWidth, cellHeight, INT_MAX, INT_MAX) {}
    SelectablePixmapItem(int cellWidth, int cellHeight, int maxSelectionWidth, int maxSelectionHeight) {
        this->cellWidth = cellWidth;
        this->cellHeight = cellHeight;
        this->maxSelectionWidth = maxSelectionWidth;
        this->maxSelectionHeight = maxSelectionHeight;
    }
    virtual QPoint getSelectionDimensions();
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

    QPoint getSelectionStart();
    void select(int, int, int, int);
    void updateSelection(int, int);
    QPoint getCellPos(QPointF);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    virtual void drawSelection();

signals:
    void selectionChanged(int, int, int, int);
};

#endif // SELECTABLEPIXMAPITEM_H
