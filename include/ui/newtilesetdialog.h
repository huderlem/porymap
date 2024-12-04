#ifndef NEWTILESETDIALOG_H
#define NEWTILESETDIALOG_H

#include <QDialog>
#include <QAbstractButton>

class Project;
class Tileset;

namespace Ui {
class NewTilesetDialog;
}

class NewTilesetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewTilesetDialog(Project *project, QWidget *parent = nullptr);
    ~NewTilesetDialog();

    virtual void accept() override;

private:
    Ui::NewTilesetDialog *ui;
    Project *project = nullptr;
    const QString symbolPrefix;

    bool validateName(bool allowEmpty = false);
    void onFriendlyNameChanged(const QString &friendlyName);
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // NEWTILESETDIALOG_H
