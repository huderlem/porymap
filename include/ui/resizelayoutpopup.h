#ifndef RESIZELAYOUTPOPUP_H
#define RESIZELAYOUTPOPUP_H

#include "maplayout.h"
#include "project.h"
#include "checkeredbgscene.h"

#include <QDialog>
#include <QPointer>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QDialogButtonBox>

class ResizableRect;
namespace Ui {
    class ResizeLayoutPopup;
}

/// PixmapItem subclass which allows for creating a boundary which determine whether
/// the pixmap paints normally or with a black tint.
/// This item is movable and snaps on a 'cellSize' grid.
class BoundedPixmapItem : public QGraphicsPixmapItem {
public:
    BoundedPixmapItem(const QPixmap &pixmap, const QSize &cellSize, QGraphicsItem *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    void setBoundary(ResizableRect *rect) { this->boundary = rect; }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    ResizableRect *boundary = nullptr;
    QPointF clickedPos = QPointF();
    QSize cellSize;
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
