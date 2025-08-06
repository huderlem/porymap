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
    virtual QSize getSelectionDimensions() const { return QSize(abs(this->selectionOffsetX) + 1, abs(this->selectionOffsetY) + 1); }
    virtual void draw() = 0;

    virtual void setMaxSelectionSize(const QSize &size) { setMaxSelectionSize(size.width(), size.height()); }
    virtual void setMaxSelectionSize(int width, int height);
    QSize maxSelectionSize() { return QSize(this->maxSelectionWidth, this->maxSelectionHeight); }

    void setSelectionStyle(Qt::PenStyle style);

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
    void select(const QPoint &pos, const QSize &size = QSize(1,1));
    void select(int x, int y, int width = 1, int height = 1) { select(QPoint(x, y), QSize(width, height)); }
    void updateSelection(const QPoint &pos);
    QPoint getCellPos(const QPointF &itemPos);
    int getBoundedWidth(int width) const { return qBound(1, width, this->maxSelectionWidth); }
    int getBoundedHeight(int height) const { return qBound(1, height, this->maxSelectionHeight); }
    virtual void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
    virtual void drawSelectionRect(const QPoint &, const QSize &, Qt::PenStyle style = Qt::SolidLine);
    virtual void drawSelection();
    virtual int cellsWide() const { return this->cellWidth ? (pixmap().width() / this->cellWidth) : 0; }
    virtual int cellsTall() const { return this->cellHeight ? (pixmap().height() / this->cellHeight) : 0; }

signals:
    void selectionChanged(const QPoint&, const QSize&);

private:
    QPoint prevCellPos = QPoint(-1,-1);
    Qt::PenStyle selectionStyle = Qt::SolidLine;

    void setSelection(const QPoint &pos, const QSize &size);
};

#endif // SELECTABLEPIXMAPITEM_H
