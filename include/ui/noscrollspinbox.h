#ifndef NOSCROLLSPINBOX_H
#define NOSCROLLSPINBOX_H

#include <QSpinBox>

class NoScrollSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    explicit NoScrollSpinBox(QWidget *parent = nullptr);
    void wheelEvent(QWheelEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    unsigned getActionId();

private:
};

#endif // NOSCROLLSPINBOX_H
