#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QDialog>
#include <QGraphicsScene>

namespace Ui {
class ColorPicker;
}

class ColorPicker : public QDialog
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget *parent = nullptr);
    ~ColorPicker();

    QColor getColor() { return this->color; }

private:
    Ui::ColorPicker *ui;
    QGraphicsScene *scene = nullptr;
    QTimer *timer = nullptr;

    QPoint lastCursorPos = QPoint(0, 0);

    QColor color = Qt::white;

    void hover(int mouseX, int mouseY);
};

#endif // COLORPICKER_H
