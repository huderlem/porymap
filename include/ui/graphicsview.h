#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>


// For general utility features that we add to QGraphicsView
class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QWidget *parent = nullptr) : QGraphicsView(parent) {
        viewport()->installEventFilter(this);
    }

    void centerOn(const QGraphicsView *other) {
        if (other && other->viewport()) {
            QPoint center = other->viewport()->rect().center();
            QGraphicsView::centerOn(other->mapToScene(center));
        }
    }

    bool eventFilter(QObject *obj, QEvent *event) {
        auto createLeftButtonMouseEvent = [](const QMouseEvent *srcEvent) {
// Some of QMouseEvent's position functions / constructors changed between Qt5 and Qt6.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            return new QMouseEvent(srcEvent->type(), srcEvent->pos(),
                                    Qt::MouseButton::LeftButton,
                                    Qt::MouseButton::LeftButton,
                                    Qt::KeyboardModifier::NoModifier);
#else
            return new QMouseEvent(srcEvent->type(), srcEvent->position(), srcEvent->globalPosition(),
                                    Qt::MouseButton::LeftButton,
                                    Qt::MouseButton::LeftButton,
                                    Qt::KeyboardModifier::NoModifier);
#endif
        };
        // The goal here is to enable pressing the middle mouse button to pan around the graphics view.
        // In Qt, the normal way to do this is via setDragMode(). However, that dragging mechanism only
        // works via the LEFT mouse button. To support middle mouse button, we have to hijack the middle
        // mouse button press event and simulate a fake left button press event. We're not done there,
        // though. The children pixmap items will also be receiving that fake button press along with their
        // own copy of the original middle-mouse press event because of how QGraphicsScene event handling
        // works. So, we maintain a this->isMiddleButtonScrollInProgress boolean which the Editor can query
        // to determine if it should ignore mouse events on the pixmap items (e.g. painting, bucket fill).
        if (obj == viewport()) {
            if (enableMiddleMouseButtonScroll) {
                if (event->type() == QEvent::MouseButtonPress) {
                    auto mouseEvent = static_cast<QMouseEvent*>(event);
                    if (mouseEvent->button() == Qt::MiddleButton) {
                        this->setDragMode(QGraphicsView::DragMode::ScrollHandDrag);
                        this->isMiddleButtonScrollInProgress = true;
                        QMouseEvent* pressEvent = createLeftButtonMouseEvent(mouseEvent);
                        this->mousePressEvent(pressEvent);
                    }
                    return false;
                }
                else if (event->type() == QEvent::MouseButtonRelease) {
                    auto mouseEvent = static_cast<QMouseEvent*>(event);
                    QMouseEvent* releaseEvent = createLeftButtonMouseEvent(mouseEvent);
                    this->mouseReleaseEvent(releaseEvent);
                    this->setDragMode(desiredDragMode);
                    this->isMiddleButtonScrollInProgress = false;
                }
            }
        }

        return QGraphicsView::eventFilter(obj, event);
    }

    bool getIsMiddleButtonScrollInProgress() {
        return this->isMiddleButtonScrollInProgress;
    }

    void setDesiredDragMode(DragMode mode) {
        this->setDragMode(mode);
        this->desiredDragMode = mode;
    }

protected:
    bool enableMiddleMouseButtonScroll = false;

private:
    bool isMiddleButtonScrollInProgress = false;
    DragMode desiredDragMode = DragMode::NoDrag;
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
