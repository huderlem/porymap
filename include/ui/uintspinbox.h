#ifndef UINTSPINBOX_H
#define UINTSPINBOX_H

#include <QAbstractSpinBox>
#include <QLineEdit>

/*
    QSpinBox stores minimum/maximum/value as ints. This class is a version of QAbstractSpinBox for values above 0x7FFFFFFF.
    It minimally implements some QSpinBox-specific features like prefixes to simplify support for hex value input.
    It also implements the scroll focus requirements of NoScrollSpinBox.
*/

class UIntSpinBox : public QAbstractSpinBox
{
    Q_OBJECT

public:
    explicit UIntSpinBox(QWidget *parent = nullptr);
    ~UIntSpinBox() {};

    uint32_t value() const { return m_value; }
    uint32_t minimum() const { return m_minimum; }
    uint32_t maximum() const { return m_maximum; }
    QString prefix() const { return m_prefix; }
    int displayIntegerBase() const { return m_displayIntegerBase; }
    bool hasPadding() const { return m_hasPadding; }

    void setMinimum(uint32_t min);
    void setMaximum(uint32_t max);
    void setRange(uint32_t min, uint32_t max);
    void setPrefix(const QString &prefix);
    void setDisplayIntegerBase(int base);
    void setHasPadding(bool enabled);

private:
    uint32_t m_minimum;
    uint32_t m_maximum;
    uint32_t m_value;
    QString m_prefix;
    int m_displayIntegerBase;
    bool m_hasPadding;
    int m_numChars;

    void updateEdit();
    QString stripped(QString input) const;

    virtual void stepBy(int steps) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void focusOutEvent(QFocusEvent *event) override;

protected:
    virtual uint32_t valueFromText(const QString &text) const;
    virtual QString textFromValue(uint32_t val) const;
    virtual QValidator::State validate(QString &input, int &pos) const override;
    virtual QAbstractSpinBox::StepEnabled stepEnabled() const override;


public slots:
    void setValue(uint32_t val);
    void onEditFinished();

signals:
    void valueChanged(uint32_t val);
    void textChanged(const QString &text);
};

class UIntHexSpinBox : public UIntSpinBox
{
    Q_OBJECT
public:
    UIntHexSpinBox(QWidget *parent = nullptr) : UIntSpinBox(parent) {
        this->setPrefix("0x");
        this->setDisplayIntegerBase(16);
        this->setHasPadding(true);
    }
};

#endif // UINTSPINBOX_H
