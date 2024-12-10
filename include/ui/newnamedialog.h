#ifndef NEWNAMEDIALOG_H
#define NEWNAMEDIALOG_H

/*
    This is a generic dialog for requesting a new unique name from the user.
*/

#include <QDialog>
#include <QAbstractButton>

class Project;

namespace Ui {
class NewNameDialog;
}

class NewNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewNameDialog(const QString &label, Project *project, QWidget *parent = nullptr);
    ~NewNameDialog();

    void setNamePrefix(const QString &prefix);

    virtual void accept() override;

signals:
    void applied(const QString &newName);

private:
    Ui::NewNameDialog *ui;
    Project *project = nullptr;
    const QString symbolPrefix;

    bool validateName(bool allowEmpty = false);
    void onNameChanged(const QString &name);
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // NEWNAMEDIALOG_H
