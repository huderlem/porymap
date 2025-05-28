#ifndef RESIZELAYOUTPOPUP_H
#define RESIZELAYOUTPOPUP_H

#include "maplayout.h"
#include "project.h"

#include <QDialog>
#include <QPointer>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QDialogButtonBox>

class ResizableRect;
namespace Ui {
    class ResizeLayoutPopup;
}



/// Custom scene that paints its background a gray checkered pattern.
/// Additionally there is a definable "valid" area which will paint the checkerboard green inside.
class CheckeredBgScene : public QGraphicsScene {
    Q_OBJECT

public:
    CheckeredBgScene(QObject *parent = nullptr);
    void setValidRect(int x, int y, int width, int height) {
        this->validRect = QRect(x * this->gridSize, y * this->gridSize, width * this->gridSize, height * this->gridSize);
    }
    void setValidRect(QRect rect) {
        this->validRect = rect;
    }
    QRect getValidRect() { return this->validRect; }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    int gridSize = 16; // virtual pixels
    QRect validRect = QRect();
};



/// PixmapItem subclass which allows for creating a boundary which determine whether
/// the pixmap paints normally or with a black tint.
/// This item is movable and snaps on a 16x16 grid.
class BoundedPixmapItem : public QGraphicsPixmapItem {
public:
    BoundedPixmapItem(const QPixmap &pixmap, QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    void setBoundary(ResizableRect *rect) { this->boundary = rect; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    ResizableRect *boundary = nullptr;
    QPointF clickedPos = QPointF();
};



/// The main (modal) dialog window for resizing layout and border dimensions.
/// The dialog itself is minimal, and is connected to the parent widget's geometry.
class ResizeLayoutPopup : public QDialog
{
    Q_OBJECT

public:
    ResizeLayoutPopup(QWidget *parent, Layout *layout, Project *project);
    ~ResizeLayoutPopup();

    void setupLayoutView();

    void resetPosition();

    QMargins getResult();
    QSize getBorderResult();

protected:
    void moveEvent(QMoveEvent *) override {
        // Prevent the dialog from being moved
        this->resetPosition();
    }

    void resizeEvent(QResizeEvent *) override {
        // Prevent the dialog from being resized
        this->resetPosition();
    }

    void keyPressEvent(QKeyEvent *) override;

private slots:
    void on_spinBox_width_valueChanged(int value);
    void on_spinBox_height_valueChanged(int value);
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    QWidget *parent = nullptr;
    QPointer<Layout> layout = nullptr;
    QPointer<Project> project = nullptr;

    Ui::ResizeLayoutPopup *ui;

    ResizableRect *outline = nullptr;
    BoundedPixmapItem *layoutPixmap = nullptr;

    QPointer<CheckeredBgScene> scene = nullptr;

    void zoom(qreal delta);
    qreal scale = 1.0;
};

#endif // RESIZELAYOUTPOPUP_H
