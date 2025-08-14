#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>

// For general utility features that we add to QGraphicsView
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QWidget *parent = nullptr) : QGraphicsView(parent) {}

    void centerOn(const QGraphicsView *other) {
        if (other && other->viewport()) {
            QPoint center = other->viewport()->rect().center();
            QGraphicsView::centerOn(other->mapToScene(center));
        }
    }
};

class NoScrollGraphicsView : public GraphicsView
{
    Q_OBJECT
public:
    NoScrollGraphicsView(QWidget *parent = nullptr) : GraphicsView(parent) {}

protected:
    void wheelEvent(QWheelEvent *event) {
        event->ignore();
    }
};

class ClickableGraphicsView : public NoScrollGraphicsView
{
    Q_OBJECT
public:
    ClickableGraphicsView(QWidget *parent = nullptr) : NoScrollGraphicsView(parent) {}

public:
    void mouseReleaseEvent(QMouseEvent *event) override {
        QGraphicsView::mouseReleaseEvent(event);
        emit this->clicked(event);
    }

signals:
    void clicked(QMouseEvent *event);
};

class ConnectionsView : public GraphicsView
{
    Q_OBJECT
public:
    ConnectionsView(QWidget *parent = nullptr) : GraphicsView(parent) {}

signals:
    void pressedDelete();

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
};

#endif // GRAPHICSVIEW_H
