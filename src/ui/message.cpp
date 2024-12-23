#include "message.h"
#include "log.h"

#include <QApplication>

Message::Message(QMessageBox::Icon icon, const QString &text, QMessageBox::StandardButtons buttons, QWidget *parent) :
    QMessageBox(icon, QApplication::applicationName(), text, buttons, parent)
{
    setWindowModality(Qt::WindowModal);
}

ErrorMessage::ErrorMessage(const QString &message, QWidget *parent) :
    Message(QMessageBox::Critical,
            message,
            QMessageBox::Ok,
            parent)
{
    setDefaultButton(QMessageBox::Ok);
}

WarningMessage::WarningMessage(const QString &message, QWidget *parent) :
    Message(QMessageBox::Warning,
            message,
            QMessageBox::Ok,
            parent)
{
    setDefaultButton(QMessageBox::Ok);
}

InfoMessage::InfoMessage(const QString &message, QWidget *parent) :
    Message(QMessageBox::Information,
            message,
            QMessageBox::Close,
            parent)
{
    setDefaultButton(QMessageBox::Close);
}

QuestionMessage::QuestionMessage(const QString &message, QWidget *parent) :
    Message(QMessageBox::Question,
            message,
            QMessageBox::No | QMessageBox::Yes,
            parent)
{
    setDefaultButton(QMessageBox::No);
}

RecentErrorMessage::RecentErrorMessage(const QString &message, QWidget *parent) :
    ErrorMessage(message, parent)
{
    setInformativeText(QString("Please see %1 for full error details.").arg(getLogPath()));
    setDetailedText(getMostRecentError());
}

int RecentErrorMessage::show(const QString &message, QWidget *parent) {
    RecentErrorMessage msgBox(message, parent);
    return msgBox.exec();
};

int ErrorMessage::show(const QString &message, QWidget *parent) {
    ErrorMessage msgBox(message, parent);
    return msgBox.exec();
};

int WarningMessage::show(const QString &message, QWidget *parent) {
    WarningMessage msgBox(message, parent);
    return msgBox.exec();
};

int QuestionMessage::show(const QString &message, QWidget *parent) {
    QuestionMessage msgBox(message, parent);
    return msgBox.exec();
};

int InfoMessage::show(const QString &message, QWidget *parent) {
    InfoMessage msgBox(message, parent);
    return msgBox.exec();
};
