#ifndef NOSCROLLCOMBOBOX_H
#define NOSCROLLCOMBOBOX_H

#include <QComboBox>

class NoScrollComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit NoScrollComboBox(QWidget *parent = nullptr);
    void wheelEvent(QWheelEvent *event);
    void setTextItem(const QString &text);
    void setNumberItem(int value);
    void setHexItem(uint32_t value);

private:
    void setItem(int index, const QString &text);
};

#endif // NOSCROLLCOMBOBOX_H
