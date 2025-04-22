#ifndef NOSCROLLTEXTEDIT_H
#define NOSCROLLTEXTEDIT_H

#include <QTextEdit>
#include <QWheelEvent>

class NoScrollTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit NoScrollTextEdit(const QString &text, QWidget *parent = nullptr) : QTextEdit(text, parent) {
        setFocusPolicy(Qt::StrongFocus);
    };
    explicit NoScrollTextEdit(QWidget *parent = nullptr) : NoScrollTextEdit(QString(), parent) {};

    virtual void wheelEvent(QWheelEvent *event) override {
        if (hasFocus()) {
            QTextEdit::wheelEvent(event);
        } else {
            event->ignore();
        }
    };
};

#endif // NOSCROLLTEXTEDIT_H
