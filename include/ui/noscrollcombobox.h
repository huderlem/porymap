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
    void setClearButtonEnabled(bool enabled);
    void setEditable(bool editable);
    void setLineEdit(QLineEdit *edit);
    void setFocusedScrollingEnabled(bool enabled);

signals:
    void editingFinished();

private:
    void setItem(int index, const QString &text);

    bool focusedScrollingEnabled = true;
};

#endif // NOSCROLLCOMBOBOX_H
