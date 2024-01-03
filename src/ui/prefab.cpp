
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
#include <QMessageBox>

using OrderedJson = poryjson::Json;
using OrderedJsonDoc = poryjson::JsonDoc;

const QString defaultFilepath = "prefabs.json";

void Prefab::loadPrefabs() {
    this->items.clear();
    QString filepath = projectConfig.getPrefabFilepath();
    if (filepath.isEmpty()) return;

    ParseUtil parser;
    QJsonDocument prefabDoc;

    QString validPath = Project::getExistingFilepath(filepath);
    if (validPath.isEmpty() || !parser.tryParseJsonFile(&prefabDoc, validPath)) {
        logError(QString("Failed to read prefab data from %1").arg(filepath));
        return;
    }
    filepath = validPath;

    QJsonArray prefabs = prefabDoc.array();
    if (prefabs.size() == 0) {
        logWarn(QString("Prefabs array is empty or missing in %1.").arg(filepath));
        return;
    }

    for (int i = 0; i < prefabs.size(); i++) {
        QJsonObject prefabObj = prefabs[i].toObject();
        if (prefabObj.isEmpty())
            continue;

        int width = ParseUtil::jsonToInt(prefabObj["width"]);
        int height = ParseUtil::jsonToInt(prefabObj["height"]);
        if (width <= 0 || height <= 0)
            continue;

        QString name = ParseUtil::jsonToQString(prefabObj["name"]);
        QString primaryTileset = ParseUtil::jsonToQString(prefabObj["primary_tileset"]);
        QString secondaryTileset = ParseUtil::jsonToQString(prefabObj["secondary_tileset"]);

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
            int x = ParseUtil::jsonToInt(metatileObj["x"]);
            int y = ParseUtil::jsonToInt(metatileObj["y"]);
            if (x < 0 || x >= width || y < 0 || y >= height)
                continue;
            int index = y * width + x;
            int metatileId = ParseUtil::jsonToInt(metatileObj["metatile_id"]);
            if (metatileId < 0 || metatileId >= Project::getNumMetatilesTotal())
                continue;
            selection.metatileItems[index].metatileId = ParseUtil::jsonToInt(metatileObj["metatile_id"]);
            selection.collisionItems[index].collision = ParseUtil::jsonToInt(metatileObj["collision"]);
            selection.collisionItems[index].elevation = ParseUtil::jsonToInt(metatileObj["elevation"]);
            selection.metatileItems[index].enabled = true;
            selection.collisionItems[index].enabled = true;
        }

        this->items.append(PrefabItem{QUuid::createUuid(), name, primaryTileset, secondaryTileset, selection});
    }
}

void Prefab::savePrefabs() {
    QString filepath = projectConfig.getPrefabFilepath();
    if (filepath.isEmpty()) {
        filepath = defaultFilepath;
        projectConfig.setPrefabFilepath(filepath);
    }

    QFileInfo info(filepath);
    if (info.isRelative()) {
        filepath = QDir::cleanPath(projectConfig.getProjectDir() + QDir::separator() + filepath);
    }
    QFile prefabsFile(filepath);
    if (!prefabsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(filepath));
        QMessageBox messageBox;
        messageBox.setText("Failed to save prefabs file!");
        messageBox.setInformativeText(QString("Could not open \"%1\" for writing").arg(filepath));
        messageBox.setDetailedText("Any created prefabs will not be available next time Porymap is opened. Please fix your prefabs_filepath in the Porymap project config file.");
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.exec();
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
                    OrderedJson::object metatileObj;
                    metatileObj["x"] = x;
                    metatileObj["y"] = y;
                    metatileObj["metatile_id"] = metatileItem.metatileId;
                    if (item.selection.hasCollision && index < item.selection.collisionItems.size()) {
                        auto collisionItem = item.selection.collisionItems.at(index);
                        metatileObj["collision"] = collisionItem.collision;
                        metatileObj["elevation"] = collisionItem.elevation;
                    }
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
    // Prefabs are only valid for the tileset(s) from which they were created.
    // If, say, no metatiles in the prefab are from the primary tileset, then
    // any primary tileset is valid for that prefab.
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

// This function recreates the UI state for the prefab tab.
// We completely delete all the prefab widgets, and recreate new widgets
// from the relevant list of prefab items.
// This is not very efficient, but it gets the job done.
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

    // Add all the prefab widgets to this wrapper parent frame because directly adding them
    // to the main prefab widget will cause the UI to immediately render each new widget, which is slow.
    // This avoids unpleasant UI lag when a large number of prefabs are present.
    QFrame *parentFrame = new QFrame();
    parentFrame->setLayout(new QVBoxLayout());

    for (auto item : prefabs) {
        PrefabFrame *frame = new PrefabFrame();
        frame->ui->label_Name->setText(item.name);

        auto scene = new QGraphicsScene;
        scene->addPixmap(drawMetatileSelection(item.selection, map));
        scene->setSceneRect(scene->itemsBoundingRect());
        frame->ui->graphicsView_Prefab->setScene(scene);
        frame->ui->graphicsView_Prefab->setFixedSize(scene->itemsBoundingRect().width() + 2,
                                                     scene->itemsBoundingRect().height() + 2);
        parentFrame->layout()->addWidget(frame);

        // Clicking on the prefab graphics item selects it for painting.
        QObject::connect(frame->ui->graphicsView_Prefab, &ClickableGraphicsView::clicked, [this, item, frame](QMouseEvent *event){
            selector->setPrefabSelection(item.selection);
            QToolTip::showText(frame->ui->graphicsView_Prefab->mapToGlobal(event->pos()), "Selected prefab!", nullptr, {}, 1000);
        });

        // Clicking the delete button removes it from the list of known prefabs and updates the UI.
        QObject::connect(frame->ui->pushButton_DeleteItem, &QPushButton::clicked, [this, item, map](){
            for (int i = 0; i < this->items.size(); i++) {
                if (this->items[i].id == item.id) {
                    QMessageBox msgBox;
                    msgBox.setText("Confirm Prefab Deletion");
                    if (this->items[i].name.isEmpty()) {
                        msgBox.setInformativeText("Are you sure you want to delete this prefab?");
                    } else {
                        msgBox.setInformativeText(QString("Are you sure you want to delete prefab \"%1\"?").arg(this->items[i].name));
                    }

                    QPushButton *deleteButton = msgBox.addButton("Delete", QMessageBox::DestructiveRole);
                    msgBox.addButton(QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Cancel);
                    msgBox.exec();
                    if (msgBox.clickedButton() == deleteButton) {
                        this->items.removeAt(i);
                        this->savePrefabs();
                        this->updatePrefabUi(map);
                    }
                    break;
                }
            }
        });
    }
    prefabWidget->layout()->addWidget(parentFrame);
    auto verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Expanding);
    prefabWidget->layout()->addItem(verticalSpacer);
}

void Prefab::addPrefab(MetatileSelection selection, Map *map, QString name) {
    // First, determine which tilesets are actually used in this new prefab,
    // based on the metatile ids.
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

bool Prefab::tryImportDefaultPrefabs(QWidget * parent, BaseGameVersion version, QString filepath) {
    // Ensure we have default prefabs for the project's game version.
    if (version != BaseGameVersion::pokeruby && version != BaseGameVersion::pokeemerald && version != BaseGameVersion::pokefirered)
        return false;

    if (filepath.isEmpty())
        filepath = defaultFilepath;

    // Get the absolute filepath for writing/warnings
    QString absFilepath;
    QFileInfo fileInfo(filepath);
    if (fileInfo.suffix().isEmpty())
        filepath += ".json";
    if (fileInfo.isRelative()) {
        absFilepath = QDir::cleanPath(projectConfig.getProjectDir() + QDir::separator() + filepath);
    } else {
        absFilepath = filepath;
    }

    // The warning message when importing defaults changes if there's a pre-existing file.
    QString fileWarning;
    if (!QFileInfo::exists(absFilepath)) {
        fileWarning = QString("This will create a file called '%1'").arg(absFilepath);
    } else {
        fileWarning = QString("This will overwrite any existing prefabs in '%1'").arg(absFilepath);
    }

    // Display a dialog box to the user, asking if the default prefabs should be imported
    // into their project.
     QMessageBox::StandardButton prompt =
             QMessageBox::question(parent,
                                   "Import Default Prefabs",
                                   QString("Would you like to import the default prefabs for %1? %2.")
                                               .arg(projectConfig.getBaseGameVersionString(version))
                                               .arg(fileWarning),
                                   QMessageBox::Yes | QMessageBox::No);

    bool acceptedImport = (prompt == QMessageBox::Yes);
    if (acceptedImport) {
        // Sets up the default prefabs.json filepath.
        projectConfig.setPrefabFilepath(filepath);
        QFile prefabsFile(absFilepath);
        if (!prefabsFile.open(QIODevice::WriteOnly)) {
            projectConfig.setPrefabFilepath(QString());

            logError(QString("Error: Could not open %1 for writing").arg(absFilepath));
            QMessageBox messageBox(parent);
            messageBox.setText("Failed to import default prefabs file!");
            messageBox.setInformativeText(QString("Could not open \"%1\" for writing").arg(absFilepath));
            messageBox.setIcon(QMessageBox::Warning);
            messageBox.exec();
            return false;
        }

        ParseUtil parser;
        QString content;
        switch (version) {
        case BaseGameVersion::pokeruby:
            content = parser.readTextFile(":/text/prefabs_default_ruby.json");
            break;
        case BaseGameVersion::pokefirered:
            content = parser.readTextFile(":/text/prefabs_default_firered.json");
            break;
        case BaseGameVersion::pokeemerald:
            content = parser.readTextFile(":/text/prefabs_default_emerald.json");
            break;
        }

        prefabsFile.write(content.toUtf8());
        prefabsFile.close();
        this->loadPrefabs();
    }

    projectConfig.setPrefabImportPrompted(true);
    return acceptedImport;
}

Prefab prefab;
