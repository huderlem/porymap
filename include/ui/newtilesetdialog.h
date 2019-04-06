#ifndef NEWTILESETDIALOG_H
#define NEWTILESETDIALOG_H

#include <QDialog>
#include "project.h"

namespace Ui {
class NewTilesetDialog;
}

class NewTilesetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewTilesetDialog(Project *project, QWidget *parent = nullptr);
    ~NewTilesetDialog();
    QString path;
    QString fullSymbolName;
    QString friendlyName;
    bool isSecondary;

private slots:
    void NameOrSecondaryChanged();
    void SecondaryChanged();

private:
    Ui::NewTilesetDialog *ui;
    Project *project = nullptr;
};

#endif // NEWTILESETDIALOG_H
