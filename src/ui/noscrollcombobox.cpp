#include "noscrollcombobox.h"

#include <QCompleter>

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
    QValidator *validator = new QRegularExpressionValidator(re);
    this->setValidator(validator);
}

void NoScrollComboBox::wheelEvent(QWheelEvent *event)
{
    // Only allow scrolling to modify contents when it explicitly has focus.
    if (hasFocus())
        QComboBox::wheelEvent(event);
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
    this->setItem(this->findData(value), "0x" + QString::number(value, 16).toUpper());
}
