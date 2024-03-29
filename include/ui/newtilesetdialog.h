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
    bool checkerboardFill;

private slots:
    void NameOrSecondaryChanged();
    void SecondaryChanged();
    void FillChanged();

private:
    Ui::NewTilesetDialog *ui;
    Project *project = nullptr;
};

#endif // NEWTILESETDIALOG_H
