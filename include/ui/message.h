#pragma once
#ifndef MESSAGE_H
#define MESSAGE_H

/*
    These classes are thin wrappers around QMessageBox for convenience.
    The base Message class is a regular window-modal QMessageBox with "porymap" as the window title.

    QMessageBox's static functions enforce application modality (among other things), which changes the style of the message boxes on macOS.
    With these equivalent static functions we have more control over the appearance and behavior of the window,
    and we keep the convenience of not needing to provide all the arguments.

    If more control is needed (like adding custom buttons to the window) use the constructors as you would for a normal QMessageBox.
*/

#include <QMessageBox>

class Message : public QMessageBox {
public:
    Message(QMessageBox::Icon icon, const QString &text, QMessageBox::StandardButtons buttons, QWidget *parent);
};

// Basic error message with an 'Ok' button.
class ErrorMessage : public Message {
public:
    ErrorMessage(const QString &message, QWidget *parent);
    static int show(const QString &message, QWidget *parent);
};

// Basic warning message with an 'Ok' button.
class WarningMessage : public Message {
public:
    WarningMessage(const QString &message, QWidget *parent);
    static int show(const QString &message, QWidget *parent);
};

// Basic informational message with a 'Close' button.
class InfoMessage : public Message {
public:
    InfoMessage(const QString &message, QWidget *parent);
    static int show(const QString &message, QWidget *parent);
};

// Basic question message with a 'Yes' and 'No' button.
class QuestionMessage : public Message {
public:
    QuestionMessage(const QString &message, QWidget *parent);
    static int show(const QString &message, QWidget *parent);
};

// Error message directing users to their log file.
// Shows the most recent error as detailed text.
class RecentErrorMessage : public ErrorMessage {
public:
    RecentErrorMessage(const QString &message, QWidget *parent);
    static int show(const QString &message, QWidget *parent);
};


#endif // MESSAGE_H
