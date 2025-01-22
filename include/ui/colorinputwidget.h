#ifndef COLORINPUTWIDGET_H
#define COLORINPUTWIDGET_H

#include <QGroupBox>

namespace Ui {
class ColorInputWidget;
}


class ColorInputWidget : public QGroupBox {
    Q_OBJECT
public:
    explicit ColorInputWidget(QWidget *parent = nullptr);
    explicit ColorInputWidget(const QString &title, QWidget *parent = nullptr);
    ~ColorInputWidget();

    void setColor(QRgb color);
    QRgb color() const { return m_color; }

    bool setBitDepth(int bits);
    int bitDepth() const { return m_bitDepth; }

signals:
    void colorChanged(QRgb color);
    void bitDepthChanged(int bits);
    void editingFinished();

private:
    Ui::ColorInputWidget *ui;

    QRgb m_color = 0;
    int m_bitDepth = 0;

    void init();
    void updateColorUi();
    void pickColor();
    void blockEditSignals(bool block);

    void setRgbFromSliders();
    void setRgbFromSpinners();
    void setRgbFromHexString(const QString &);
};

#endif // COLORINPUTWIDGET_H
