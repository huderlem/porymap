#ifndef NEWDEFINEDIALOG_H
#define NEWDEFINEDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class NewDefineDialog;
}

class NewDefineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewDefineDialog(QWidget *parent = nullptr);
    ~NewDefineDialog();

    virtual void accept() override;

signals:
    void createdDefine(const QString &name, const QString &expression);

private:
    Ui::NewDefineDialog *ui;

    bool validateName(bool allowEmpty = false);
    void onNameChanged(const QString &name);
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // NEWDEFINEDIALOG_H
