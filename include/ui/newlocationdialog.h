#ifndef NEWLOCATIONDIALOG_H
#define NEWLOCATIONDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QPointer>

class Project;

namespace Ui {
class NewLocationDialog;
}

class NewLocationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewLocationDialog(Project *project = nullptr, QWidget *parent = nullptr);
    ~NewLocationDialog();

    virtual void accept() override;

private:
    Ui::NewLocationDialog *ui;
    QPointer<Project> project = nullptr;
    const QString namePrefix;

    bool validateIdName(bool allowEmpty = false);
    void onIdNameChanged(const QString &name);
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // NEWLOCATIONDIALOG_H
