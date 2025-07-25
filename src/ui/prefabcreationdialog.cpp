#include "prefabcreationdialog.h"
#include "ui_prefabcreationdialog.h"
#include "currentselectedmetatilespixmapitem.h"
#include "graphicsview.h"
#include "prefab.h"

#include <QObject>

PrefabCreationDialog::PrefabCreationDialog(QWidget *parent, MetatileSelector *metatileSelector, Layout *layout) :
    QDialog(parent),
    ui(new Ui::PrefabCreationDialog)
{
    ui->setupUi(this);

    this->layout = layout;
    this->selection = metatileSelector->getMetatileSelection();
    QGraphicsScene *scene = new QGraphicsScene;
    QGraphicsPixmapItem *pixmapItem = scene->addPixmap(drawMetatileSelection(this->selection, layout));
    scene->setSceneRect(scene->itemsBoundingRect());
    this->ui->graphicsView_Prefab->setScene(scene);
    this->ui->graphicsView_Prefab->setFixedSize(scene->itemsBoundingRect().width() + 2,
                                                 scene->itemsBoundingRect().height() + 2);

    QObject::connect(this->ui->graphicsView_Prefab, &ClickableGraphicsView::clicked, [=](QMouseEvent *event){
        auto pos = event->pos();
        int selectionWidth = this->selection.dimensions.x() * Metatile::pixelWidth();
        int selectionHeight = this->selection.dimensions.y() * Metatile::pixelHeight();
        if (pos.x() < 0 || pos.x() >= selectionWidth || pos.y() < 0 || pos.y() >= selectionHeight)
            return;
        int metatileX = pos.x() / Metatile::pixelWidth();
        int metatileY = pos.y() / Metatile::pixelHeight();
        int index = metatileY * this->selection.dimensions.x() + metatileX;
        bool toggledState = !this->selection.metatileItems[index].enabled;
        this->selection.metatileItems[index].enabled = toggledState;
        if (this->selection.hasCollision) {
            this->selection.collisionItems[index].enabled = toggledState;
        }
        pixmapItem->setPixmap(drawMetatileSelection(this->selection, layout));
    });

    connect(this, &PrefabCreationDialog::accepted, this, &PrefabCreationDialog::savePrefab);
}

PrefabCreationDialog::~PrefabCreationDialog()
{
    delete ui;
}

void PrefabCreationDialog::savePrefab() {
    prefab.addPrefab(this->selection, this->layout, this->ui->lineEdit_PrefabName->text());
}
