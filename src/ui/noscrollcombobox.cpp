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

void NoScrollComboBox::setTextItem(const QString &text)
{
    int index = this->findText(text);
    if (index >= 0)
        this->setCurrentIndex(index);
    else
        this->setCurrentText(text);
}
