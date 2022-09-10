
#include "prefab.h"
#include "prefabframe.h"
#include "ui_prefabframe.h"
#include "parseutil.h"
#include "currentselectedmetatilespixmapitem.h"
#include "log.h"
#include "project.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGraphicsPixmapItem>
#include <QWidget>
#include <QDir>
#include <QSpacerItem>

using OrderedJson = poryjson::Json;
using OrderedJsonDoc = poryjson::JsonDoc;


void Prefab::loadPrefabs() {
    this->items.clear();
    QString filepath = projectConfig.getPrefabFilepath();
    if (filepath.isEmpty()) return;

    ParseUtil parser;
    QJsonDocument prefabDoc;
    if (!QFile::exists(filepath) || !parser.tryParseJsonFile(&prefabDoc, filepath)) {
        QString relativePath = QDir::cleanPath(projectConfig.getProjectDir() + QDir::separator() + filepath);
        if (!parser.tryParseJsonFile(&prefabDoc, relativePath)) {
            logError(QString("Failed to prefab data from %1").arg(filepath));
            return;
        }
    }

    QJsonArray prefabs = prefabDoc.array();
    if (prefabs.size() == 0) {
        logWarn(QString("Prefabs array is empty or missing in %1.").arg(filepath));
        return;
    }

    for (int i = 0; i < prefabs.size(); i++) {
        QJsonObject prefabObj = prefabs[i].toObject();
        if (prefabObj.isEmpty())
            continue;

        int width = prefabObj["width"].toInt();
        int height = prefabObj["height"].toInt();
        if (width <= 0 || height <= 0)
            continue;

        QString name = prefabObj["name"].toString();
        QString primaryTileset = prefabObj["primary_tileset"].toString();
        QString secondaryTileset = prefabObj["secondary_tileset"].toString();

        MetatileSelection selection;
        selection.dimensions = QPoint(width, height);
        selection.hasCollision = true;
        for (int j = 0; j < width * height; j++) {
            selection.metatileItems.append(MetatileSelectionItem{false, 0});
            selection.collisionItems.append(CollisionSelectionItem{false, 0, 0});
        }
        QJsonArray metatiles = prefabObj["metatiles"].toArray();
        for (int j = 0; j < metatiles.size(); j++) {
            QJsonObject metatileObj = metatiles[j].toObject();
            int x = metatileObj["x"].toInt();
            int y = metatileObj["y"].toInt();
            if (x < 0 || x >= width || y < 0 || y >= height)
                continue;
            int index = y * width + x;
            selection.metatileItems[index].enabled = true;
            selection.metatileItems[index].metatileId = metatileObj["metatile_id"].toInt();
            selection.collisionItems[index].enabled = true;
            selection.collisionItems[index].collision = metatileObj["collision"].toInt();
            selection.collisionItems[index].elevation = metatileObj["elevation"].toInt();
        }

        this->items.append(PrefabItem{QUuid::createUuid(), name, primaryTileset, secondaryTileset, selection});
    }
}

void Prefab::savePrefabs() {
    QString filepath = projectConfig.getPrefabFilepath();
    if (filepath.isEmpty()) return;
    QFile prefabsFile(filepath);
    if (!prefabsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(filepath));
        return;
    }

    OrderedJson::array prefabsArr;
    for (auto item : this->items) {
        OrderedJson::object prefabObj;
        prefabObj["name"] = item.name;
        prefabObj["width"] = item.selection.dimensions.x();
        prefabObj["height"] = item.selection.dimensions.y();
        prefabObj["primary_tileset"] = item.primaryTileset;
        prefabObj["secondary_tileset"] = item.secondaryTileset;
        OrderedJson::array metatiles;
        for (int y = 0; y < item.selection.dimensions.y(); y++) {
            for (int x = 0; x < item.selection.dimensions.x(); x++) {
                int index = y * item.selection.dimensions.x() + x;
                auto metatileItem = item.selection.metatileItems.at(index);
                if (metatileItem.enabled) {
                    auto collisionItem = item.selection.collisionItems.at(index);
                    OrderedJson::object metatileObj;
                    metatileObj["x"] = x;
                    metatileObj["y"] = y;
                    metatileObj["metatile_id"] = metatileItem.metatileId;
                    metatileObj["collision"] = collisionItem.collision;
                    metatileObj["elevation"] = collisionItem.elevation;
                    metatiles.push_back(metatileObj);
                }
            }
        }
        prefabObj["metatiles"] = metatiles;
        prefabsArr.push_back(prefabObj);
    }

    OrderedJson prefabJson(prefabsArr);
    OrderedJsonDoc jsonDoc(&prefabJson);
    jsonDoc.dump(&prefabsFile);
    prefabsFile.close();
}

QList<PrefabItem> Prefab::getPrefabsForTilesets(QString primaryTileset, QString secondaryTileset) {
    QList<PrefabItem> filteredPrefabs;
    for (auto item : this->items) {
        if ((item.primaryTileset.isEmpty() || item.primaryTileset == primaryTileset) &&
            (item.secondaryTileset.isEmpty() || item.secondaryTileset == secondaryTileset)) {
            filteredPrefabs.append(item);
        }
    }
    return filteredPrefabs;
}

void Prefab::initPrefabUI(MetatileSelector *selector, QWidget *prefabWidget, QLabel *emptyPrefabLabel, Map *map) {
    this->selector = selector;
    this->prefabWidget = prefabWidget;
    this->emptyPrefabLabel = emptyPrefabLabel;
    this->loadPrefabs();
    this->updatePrefabUi(map);
}

void Prefab::updatePrefabUi(Map *map) {
    if (!this->selector)
        return;
    // Cleanup the PrefabFrame to have a clean slate.
    auto layout = this->prefabWidget->layout();
    while (layout && layout->count() > 1) {
        auto child = layout->takeAt(1);
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    QList<PrefabItem> prefabs = this->getPrefabsForTilesets(map->layout->tileset_primary_label, map->layout->tileset_secondary_label);
    if (prefabs.isEmpty()) {
        emptyPrefabLabel->setVisible(true);
        return;
    }

    emptyPrefabLabel->setVisible(false);
    for (auto item : prefabs) {
        PrefabFrame *frame = new PrefabFrame();
        frame->ui->label_Name->setText(item.name);

        auto scene = new QGraphicsScene;
        scene->addPixmap(drawMetatileSelection(item.selection, map));
        scene->setSceneRect(scene->itemsBoundingRect());
        frame->ui->graphicsView_Prefab->setScene(scene);
        frame->ui->graphicsView_Prefab->setFixedSize(scene->itemsBoundingRect().width() + 2,
                                                     scene->itemsBoundingRect().height() + 2);
        prefabWidget->layout()->addWidget(frame);

        // Clicking on the prefab graphics item selects it for painting.
        QObject::connect(frame->ui->graphicsView_Prefab, &ClickableGraphicsView::clicked, [this, item](){
            selector->setDirectSelection(item.selection);
        });

        // Clicking the delete button removes it from the list of known prefabs and updates the UI.
        QObject::connect(frame->ui->pushButton_DeleteItem, &QPushButton::clicked, [this, item, map](){
            for (int i = 0; i < this->items.size(); i++) {
                if (this->items[i].id == item.id) {
                    this->items.removeAt(i);
                    this->savePrefabs();
                    this->updatePrefabUi(map);
                    break;
                }
            }
        });
    }
    auto spacer = new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding);
    prefabWidget->layout()->addItem(spacer);
}

void Prefab::addPrefab(MetatileSelection selection, Map *map, QString name) {
    bool usesPrimaryTileset = false;
    bool usesSecondaryTileset = false;
    for (auto metatile : selection.metatileItems) {
        if (!metatile.enabled)
            continue;
        if (metatile.metatileId < Project::getNumMetatilesPrimary()) {
            usesPrimaryTileset = true;
        } else if (metatile.metatileId < Project::getNumMetatilesTotal()) {
            usesSecondaryTileset = true;
        }
    }

    this->items.append(PrefabItem{
                           QUuid::createUuid(),
                           name,
                           usesPrimaryTileset ? map->layout->tileset_primary_label : "",
                           usesSecondaryTileset ? map->layout->tileset_secondary_label: "",
                           selection
                       });
    this->savePrefabs();
    this->updatePrefabUi(map);
}


Prefab prefab;
