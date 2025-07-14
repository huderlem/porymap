#ifndef SELECTABLEPIXMAPITEM_H
#define SELECTABLEPIXMAPITEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>

class SelectablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    SelectablePixmapItem(const QSize &size, const QSize &maxSelectionSize = QSize(INT_MAX, INT_MAX))
        : SelectablePixmapItem(size.width(), size.height(), maxSelectionSize.width(), maxSelectionSize.height()) {}
    SelectablePixmapItem(int cellWidth, int cellHeight, int maxSelectionWidth = INT_MAX, int maxSelectionHeight = INT_MAX)
        : cellWidth(cellWidth),
          cellHeight(cellHeight),
          maxSelectionWidth(maxSelectionWidth),
          maxSelectionHeight(maxSelectionHeight),
          selectionInitialX(0),
          selectionInitialY(0),
          selectionOffsetX(0),
          selectionOffsetY(0)
        {}
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
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    virtual void drawSelection();

signals:
    void selectionChanged(int, int, int, int);
};

#endif // SELECTABLEPIXMAPITEM_H
