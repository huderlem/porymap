#ifndef CHECKEREDBGSCENE_H
#define CHECKEREDBGSCENE_H

#include <QGraphicsScene>

// Custom scene that paints its background a gray checkered pattern.
// Additionally there is a definable "valid" area which will paint the checkerboard green inside.
class CheckeredBgScene : public QGraphicsScene {
    Q_OBJECT

public:
    CheckeredBgScene(const QSize &gridSize, QObject *parent = nullptr)
        : QGraphicsScene(parent),
          gridSize(gridSize)
        {};
    CheckeredBgScene(int width, int height, QObject *parent = nullptr)
        : CheckeredBgScene(QSize(width, height), parent)
        {};

    void setValidRect(int x, int y, int width, int height) {
        this->validRect = QRect(x * this->gridSize.width(),
                                y * this->gridSize.height(),
                                width * this->gridSize.width(),
                                height * this->gridSize.height());
    }
    void setValidRect(const QRect &rect) {
        this->validRect = rect;
    }
    QRect getValidRect() { return this->validRect; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    QSize gridSize;
    QRect validRect;
};

#endif // CHECKEREDBGSCENE_H
