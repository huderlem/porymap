#include "noscrollcombobox.h"
#include "utility.h"

#include <QCompleter>
#include <QLineEdit>
#include <QWheelEvent>

NoScrollComboBox::NoScrollComboBox(QWidget *parent)
    : QComboBox(parent)
{
    // Don't let scrolling hijack focus.
    setFocusPolicy(Qt::StrongFocus);

    // Make speed a priority when loading comboboxes.
    setMinimumContentsLength(24);// an arbitrary limit
    setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    // Allow items to be searched by any part of the word, displaying all matches.
    setEditable(true);// can set to false manually when using
    this->completer()->setCompletionMode(QCompleter::PopupCompletion);
    this->completer()->setFilterMode(Qt::MatchContains);

    static const QRegularExpression re("[^\\s]*");
    QValidator *validator = new QRegularExpressionValidator(re, this);
    this->setValidator(validator);

    // QComboBox (as of writing) has no 'editing finished' signal to capture
    // changes made either through the text edit or the drop-down.
    connect(this, QOverload<int>::of(&QComboBox::activated), this, &NoScrollComboBox::editingFinished);
    connect(this->lineEdit(), &QLineEdit::editingFinished, this, &NoScrollComboBox::editingFinished);
}

// On macOS QComboBox::setEditable and QComboBox::setLineEdit will override our changes to the focus policy, so we enforce it here.
void NoScrollComboBox::setEditable(bool editable) {
    auto policy = focusPolicy();
    QComboBox::setEditable(editable);
    setFocusPolicy(policy);
}
void NoScrollComboBox::setLineEdit(QLineEdit *edit) {
    auto policy = focusPolicy();
    QComboBox::setLineEdit(edit);
    setFocusPolicy(policy);
}

void NoScrollComboBox::wheelEvent(QWheelEvent *event)
{
    // By default NoScrollComboBoxes will allow scrolling to modify its contents only when it explicitly has focus.
    // If focusedScrollingEnabled is false it won't allow scrolling even with focus.
    if (this->focusedScrollingEnabled && hasFocus()) {
        QComboBox::wheelEvent(event);
    } else {
        event->ignore();
    }
}

void NoScrollComboBox::setFocusedScrollingEnabled(bool enabled)
{
    this->focusedScrollingEnabled = enabled;
}

void NoScrollComboBox::setItem(int index, const QString &text)
{
    if (index >= 0) {
        // Valid item
        this->setCurrentIndex(index);
    } else if (this->isEditable()) {
        // Invalid item in editable box, just display the text
        this->setCurrentText(text);
    } else {
        // Invalid item in uneditable box, display text as placeholder
        // On Qt < 5.15 this will display an empty box
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
        this->setPlaceholderText(text);
#endif
        this->setCurrentIndex(index);
    }
}

void NoScrollComboBox::setTextItem(const QString &text)
{
    this->setItem(this->findText(text), text);
}

void NoScrollComboBox::setNumberItem(int value)
{
    this->setItem(this->findData(value), QString::number(value));
}

void NoScrollComboBox::setHexItem(uint32_t value)
{
    this->setItem(this->findData(value), Util::toHexString(value));
}

void NoScrollComboBox::setClearButtonEnabled(bool enabled) {
    if (this->lineEdit())
        this->lineEdit()->setClearButtonEnabled(enabled);
}
