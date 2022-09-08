#include "prefabcreationdialog.h"
#include "ui_prefabcreationdialog.h"
#include "currentselectedmetatilespixmapitem.h"
#include "graphicsview.h"
#include "prefab.h"

#include <QObject>

PrefabCreationDialog::PrefabCreationDialog(QWidget *parent, MetatileSelector *metatileSelector, Map *map) :
    QDialog(parent),
    ui(new Ui::PrefabCreationDialog)
{
    ui->setupUi(this);

    this->map = map;
    this->selection = metatileSelector->getMetatileSelection();
    QGraphicsScene *scene = new QGraphicsScene;
    QGraphicsPixmapItem *pixmapItem = scene->addPixmap(drawMetatileSelection(this->selection, map));
    scene->setSceneRect(scene->itemsBoundingRect());
    this->ui->graphicsView_Prefab->setScene(scene);
    this->ui->graphicsView_Prefab->setFixedSize(scene->itemsBoundingRect().width() + 2,
                                                 scene->itemsBoundingRect().height() + 2);

    QObject::connect(this->ui->graphicsView_Prefab, &ClickableGraphicsView::clicked, [=](QMouseEvent *event){
        auto pos = event->pos();
        int selectionWidth = this->selection.dimensions.x() * 16;
        int selectionHeight = this->selection.dimensions.y() * 16;
        if (pos.x() < 0 || pos.x() >= selectionWidth || pos.y() < 0 || pos.y() >= selectionHeight)
            return;
        int metatileX = pos.x() / 16;
        int metatileY = pos.y() / 16;
        int index = metatileY * this->selection.dimensions.x() + metatileX;
        bool toggledState = !this->selection.metatileItems[index].enabled;
        this->selection.metatileItems[index].enabled = toggledState;
        if (this->selection.hasCollision) {
            this->selection.collisionItems[index].enabled = toggledState;
        }
        pixmapItem->setPixmap(drawMetatileSelection(this->selection, map));
    });
}

PrefabCreationDialog::~PrefabCreationDialog()
{
    delete ui;
}

void PrefabCreationDialog::savePrefab() {
    prefab.addPrefab(this->selection, this->map, this->ui->lineEdit_PrefabName->text());
}
