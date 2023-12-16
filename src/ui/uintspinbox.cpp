#include "uintspinbox.h"

UIntSpinBox::UIntSpinBox(QWidget *parent)
   : QAbstractSpinBox(parent)
{
    // Don't let scrolling hijack focus.
    setFocusPolicy(Qt::StrongFocus);

    m_minimum = 0;
    m_maximum = 99;
    m_value = m_minimum;
    m_displayIntegerBase = 10;
    m_numChars = 2;
    m_hasPadding = false;

    this->updateEdit();
    connect(lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(onEditFinished()));
};

void UIntSpinBox::setValue(uint32_t val) {
    val = qMax(m_minimum, qMin(m_maximum, val));
    if (m_value != val) {
        m_value = val;
        emit valueChanged(m_value);
    }
    this->updateEdit();
}

uint32_t UIntSpinBox::valueFromText(const QString &text) const {
    return this->stripped(text).toUInt(nullptr, m_displayIntegerBase);
}

QString UIntSpinBox::textFromValue(uint32_t val) const {
    if (m_hasPadding)
        return m_prefix + QString("%1").arg(val, m_numChars, m_displayIntegerBase, QChar('0')).toUpper();

    return m_prefix + QString::number(val, m_displayIntegerBase).toUpper();
}

void UIntSpinBox::setMinimum(uint32_t min) {
    this->setRange(min, qMax(min, m_maximum));
}

void UIntSpinBox::setMaximum(uint32_t max) {
    this->setRange(qMin(m_minimum, max), max);
}

void UIntSpinBox::setRange(uint32_t min, uint32_t max) {
    max = qMax(min, max);

    if (m_maximum == max && m_minimum == min)
        return;

    if (m_maximum != max) {
        // Update number of characters for padding
        m_numChars = 0;
        for (uint32_t i = max; i != 0; i /= m_displayIntegerBase)
            m_numChars++;
    }

    m_minimum = min;
    m_maximum = max;

    if (m_value < min)
        m_value %= min;
    else if (m_value > max)
        m_value %= max;
    this->updateEdit();
}

void UIntSpinBox::setPrefix(const QString &prefix) { 
    if (m_prefix != prefix) {
        m_prefix = prefix;
        this->updateEdit();
    }
}

void UIntSpinBox::setDisplayIntegerBase(int base) {
    if (base < 2 || base > 36)
        base = 10;
    if (m_displayIntegerBase != base) {
        m_displayIntegerBase = base;
        this->updateEdit();
    }
}

void UIntSpinBox::setHasPadding(bool enabled) {
    if (m_hasPadding != enabled) {
        m_hasPadding = enabled;
        this->updateEdit();
    }
}

void UIntSpinBox::updateEdit() {
    const QString text = this->textFromValue(m_value);
    if (text != this->lineEdit()->text()) {
        this->lineEdit()->setText(text);
        emit textChanged(this->lineEdit()->text());
    }
}

void UIntSpinBox::onEditFinished() {
    int pos = this->lineEdit()->cursorPosition();
    QString input = this->lineEdit()->text();

    auto state = this->validate(input, pos);
    if (state == QValidator::Invalid)
        return;

    auto newValue = m_value;
    if (state == QValidator::Acceptable) {
        // Valid input
        newValue = this->valueFromText(input);
    } else if (state == QValidator::Intermediate) {
        // User has deleted all the number text.
        // If they did this by selecting all text and then hitting delete
        // make sure to put the cursor back in front of the prefix.
        newValue = m_minimum;
        this->lineEdit()->setCursorPosition(m_prefix.length());
    }
    if (newValue != m_value) {
        m_value = newValue;
        emit this->valueChanged(m_value);
    }
    emit this->textChanged(input);
}

void UIntSpinBox::stepBy(int steps) {
    auto newValue = m_value;
    if (steps < 0 && newValue + steps > newValue) {
        newValue = 0;
    } else if (steps > 0 && newValue + steps < newValue) {
        newValue = UINT_MAX;
    } else {
        newValue += steps;
    }
    this->setValue(newValue);
}

QString UIntSpinBox::stripped(QString input) const {
    if (m_prefix.length() && input.startsWith(m_prefix))
        input.remove(0, m_prefix.length());
    return input.trimmed();
}

QValidator::State UIntSpinBox::validate(QString &input, int &pos) const {
    QString copy(input);
    input = m_prefix;

    // Adjust for prefix
    copy = this->stripped(copy);
    if (copy.isEmpty())
        return QValidator::Intermediate;

    // Editing the prefix (if not deleting all text) is not allowed.
    // Nor is editing beyond the maximum value's character limit.
    if (pos < m_prefix.length() || pos > (m_numChars + m_prefix.length()))
        return QValidator::Invalid;

    bool ok;
    uint32_t num = copy.toUInt(&ok, m_displayIntegerBase);
    if (!ok || num < m_minimum || num > m_maximum)
        return QValidator::Invalid;

    input += copy.toUpper();
    return QValidator::Acceptable;
}

QAbstractSpinBox::StepEnabled UIntSpinBox::stepEnabled() const {
    QAbstractSpinBox::StepEnabled flags = QAbstractSpinBox::StepNone;
    if (m_value < m_maximum)
        flags |= QAbstractSpinBox::StepUpEnabled;
    if (m_value > m_minimum)
        flags |= QAbstractSpinBox::StepDownEnabled;

    return flags;
}

void UIntSpinBox::wheelEvent(QWheelEvent *event) {
    // Only allow scrolling to modify contents when it explicitly has focus.
    if (hasFocus())
        QAbstractSpinBox::wheelEvent(event);
}

void UIntSpinBox::focusOutEvent(QFocusEvent *event) {
    this->updateEdit();
    QAbstractSpinBox::focusOutEvent(event);
}
