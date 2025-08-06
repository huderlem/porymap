#ifndef CURSORTILERECT_H
#define CURSORTILERECT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QRgb>

class CursorTileRect : public QGraphicsItem
{
public:
    CursorTileRect(const QSize &tileSize, const QRgb &color, QGraphicsItem *parent = nullptr);
    
    QSize size() const;

    QRectF boundingRect() const override {
        auto s = size();
        qreal penWidth = 4;
        return QRectF(-penWidth,
                      -penWidth,
                      s.width() + penWidth * 2,
                      s.height() + penWidth * 2);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
        if (!isVisible()) return;
        auto rect = QRectF(pos(), size());
        painter->setPen(m_color);
        painter->drawRect(rect + QMargins(1,1,1,1)); // Fill
        painter->setPen(QColor(0, 0, 0));
        painter->drawRect(rect + QMargins(2,2,2,2)); // Outer border
        painter->drawRect(rect); // Inner border
    }

    void initAnchor(int coordX, int coordY);
    void stopAnchor();
    void initRightClickSelectionAnchor(int coordX, int coordY);
    void stopRightClickSelectionAnchor();

    void setSmartPathMode(bool enable) { m_smartPathMode = enable; }
    bool getSmartPathMode() const { return m_smartPathMode; }

    void setSingleTileMode(bool enable) { m_singleTileMode = enable; }
    bool getSingleTileMode() const { return m_singleTileMode; }

    void updateLocation(int x, int y);
    void updateSelectionSize(const QSize &size);
    void updateSelectionSize(int width, int height) { updateSelectionSize(QSize(width, height)); }

private:
    const QSize m_tileSize;
    QSize m_selectionSize;
    QPoint m_anchorCoord;
    QRgb m_color;

    bool m_anchored = false;
    bool m_rightClickSelectionAnchored = false;
    bool m_smartPathMode = false;
    bool m_singleTileMode = false;

    bool smartPathInEffect() const;
};


#endif // CURSORTILERECT_H
