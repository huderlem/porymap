#ifndef PREFABCREATIONDIALOG_H
#define PREFABCREATIONDIALOG_H

#include "metatileselector.h"

#include <QDialog>

class Layout;

namespace Ui {
class PrefabCreationDialog;
}

class PrefabCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrefabCreationDialog(QWidget *parent, MetatileSelector *metatileSelector, Layout *layout);
    ~PrefabCreationDialog();
    void savePrefab();

private:
    Layout *layout = nullptr;
    Ui::PrefabCreationDialog *ui;
    MetatileSelection selection;
};

#endif // PREFABCREATIONDIALOG_H
