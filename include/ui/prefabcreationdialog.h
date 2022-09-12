#ifndef PREFABCREATIONDIALOG_H
#define PREFABCREATIONDIALOG_H

#include "metatileselector.h"
#include "map.h"

#include <QDialog>

namespace Ui {
class PrefabCreationDialog;
}

class PrefabCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrefabCreationDialog(QWidget *parent, MetatileSelector *metatileSelector, Map *map);
    ~PrefabCreationDialog();
    void savePrefab();

private:
    Map *map;
    Ui::PrefabCreationDialog *ui;
    MetatileSelection selection;
};

#endif // PREFABCREATIONDIALOG_H
