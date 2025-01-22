#ifndef NEWMAPGROUPDIALOG_H
#define NEWMAPGROUPDIALOG_H

/*
    This is a generic dialog for requesting a new unique name from the user.
*/

#include <QDialog>
#include <QAbstractButton>

class Project;

namespace Ui {
class NewMapGroupDialog;
}

class NewMapGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewMapGroupDialog(Project *project = nullptr, QWidget *parent = nullptr);
    ~NewMapGroupDialog();

    virtual void accept() override;

private:
    Ui::NewMapGroupDialog *ui;
    Project *project = nullptr;

    bool validateName(bool allowEmpty = false);
    void onNameChanged(const QString &name);
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // NEWMAPGROUPDIALOG_H
