#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "project.h"
#include "editor.h"
#include "objectpropertiesframe.h"
#include "ui_objectpropertiesframe.h"

#include <QDebug>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QShortcut>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>
#include <QSpacerItem>
#include <QFont>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("pretmap");

    ui->setupUi(this);
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this, SLOT(redo()));

    editor = new Editor(ui);
    connect(editor, SIGNAL(objectsChanged()), this, SLOT(updateSelectedObjects()));
    connect(editor, SIGNAL(selectedObjectsChanged()), this, SLOT(updateSelectedObjects()));

    on_toolButton_Paint_clicked();

    QSettings settings;
    QString key = "recent_projects";
    if (settings.contains(key)) {
        QString default_dir = settings.value(key).toStringList().last();
        if (!default_dir.isNull()) {
            qDebug() << QString("default_dir: '%1'").arg(default_dir);
            openProject(default_dir);
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setStatusBarMessage(QString message, int timeout/* = 0*/) {
    statusBar()->showMessage(message, timeout);
}

void MainWindow::openProject(QString dir) {
    if (dir.isNull()) {
        return;
    }

    setStatusBarMessage(QString("Opening project %1").arg(dir));

    bool already_open = (
        (editor != NULL && editor != nullptr)
        && (editor->project != NULL && editor->project != nullptr)
        && (editor->project->root == dir)
    );
    if (!already_open) {
        editor->project = new Project;
        editor->project->root = dir;
        setWindowTitle(editor->project->getProjectTitle() + " - pretmap");
        loadDataStructures();
        populateMapList();
        setMap(getDefaultMap());
    } else {
        setWindowTitle(editor->project->getProjectTitle() + " - pretmap");
        loadDataStructures();
        populateMapList();
    }

    setStatusBarMessage(QString("Opened project %1").arg(dir));
}

QString MainWindow::getDefaultMap() {
    if (editor && editor->project) {
        QList<QStringList> names = editor->project->groupedMapNames;
        if (!names.isEmpty()) {
            QSettings settings;
            QString key = "project:" + editor->project->root;
            if (settings.contains(key)) {
                QMap<QString, QVariant> qmap = settings.value(key).toMap();
                if (qmap.contains("recent_map")) {
                    QString map_name = qmap.value("recent_map").toString();
                    for (int i = 0; i < names.length(); i++) {
                        if (names.value(i).contains(map_name)) {
                            return map_name;
                        }
                    }
                }
            }
            // Failing that, just get the first map in the list.
            for (int i = 0; i < names.length(); i++) {
                QStringList list = names.value(i);
                if (list.length()) {
                    return list.value(0);
                }
            }
        }
    }
    return QString();
}

QString MainWindow::getExistingDirectory(QString dir) {
    return QFileDialog::getExistingDirectory(this, "Open Directory", dir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

void MainWindow::on_action_Open_Project_triggered()
{
    QSettings settings;
    QString key = "recent_projects";
    QString recent = ".";
    if (settings.contains(key)) {
        recent = settings.value(key).toStringList().last();
    }
    QString dir = getExistingDirectory(recent);
    if (!dir.isEmpty()) {
        QStringList recents;
        if (settings.contains(key)) {
            recents = settings.value(key).toStringList();
        }
        recents.removeAll(dir);
        recents.append(dir);
        settings.setValue(key, recents);

        openProject(dir);
    }
}

void MainWindow::setMap(QString map_name) {
    qDebug() << QString("setMap(%1)").arg(map_name);
    if (map_name.isNull()) {
        return;
    }
    editor->setMap(map_name);

    on_tabWidget_currentChanged(ui->tabWidget->currentIndex());

    ui->graphicsView_Map->setScene(editor->scene);
    ui->graphicsView_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Map->setFixedSize(editor->scene->width() + 2, editor->scene->height() + 2);

    ui->graphicsView_Objects_Map->setScene(editor->scene);
    ui->graphicsView_Objects_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Objects_Map->setFixedSize(editor->scene->width() + 2, editor->scene->height() + 2);
    ui->graphicsView_Objects_Map->editor = editor;

    ui->graphicsView_Connections->setScene(editor->scene);
    ui->graphicsView_Connections->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Connections->setFixedSize(editor->scene->width() + 2, editor->scene->height() + 2);

    ui->graphicsView_Metatiles->setScene(editor->scene_metatiles);
    //ui->graphicsView_Metatiles->setSceneRect(editor->scene_metatiles->sceneRect());
    ui->graphicsView_Metatiles->setFixedSize(editor->metatiles_item->pixmap().width() + 2, editor->metatiles_item->pixmap().height() + 2);

    ui->graphicsView_Collision->setScene(editor->scene_collision_metatiles);
    //ui->graphicsView_Collision->setSceneRect(editor->scene_collision_metatiles->sceneRect());
    ui->graphicsView_Collision->setFixedSize(editor->collision_metatiles_item->pixmap().width() + 2, editor->collision_metatiles_item->pixmap().height() + 2);

    ui->graphicsView_Elevation->setScene(editor->scene_elevation_metatiles);
    //ui->graphicsView_Elevation->setSceneRect(editor->scene_elevation_metatiles->sceneRect());
    ui->graphicsView_Elevation->setFixedSize(editor->elevation_metatiles_item->pixmap().width() + 2, editor->elevation_metatiles_item->pixmap().height() + 2);

    displayMapProperties();

    setWindowTitle(map_name + " - " + editor->project->getProjectTitle() + " - pretmap");

    connect(editor->map, SIGNAL(mapChanged(Map*)), this, SLOT(onMapChanged(Map *)));
    connect(editor->map, SIGNAL(statusBarMessage(QString)), this, SLOT(setStatusBarMessage(QString)));

    setRecentMap(map_name);
    updateMapList();
}

void MainWindow::setRecentMap(QString map_name) {
    QSettings settings;
    QString key = "project:" + editor->project->root;
    QMap<QString, QVariant> qmap;
    if (settings.contains(key)) {
        qmap = settings.value(key).toMap();
    }
    qmap.insert("recent_map", map_name);
    settings.setValue(key, qmap);
}

void MainWindow::displayMapProperties() {
    ui->comboBox_Song->clear();
    ui->comboBox_Location->clear();
    ui->comboBox_Visibility->clear();
    ui->comboBox_Weather->clear();
    ui->comboBox_Type->clear();
    ui->comboBox_BattleScene->clear();
    ui->checkBox_ShowLocation->setChecked(false);
    if (!editor || !editor->map || !editor->project) {
        ui->frame_3->setEnabled(false);
        return;
    }
    ui->frame_3->setEnabled(true);
    Map *map = editor->map;
    Project *project = editor->project;

    QStringList songs = project->getSongNames();
    ui->comboBox_Song->addItems(songs);
    ui->comboBox_Song->setCurrentText(map->song);

    ui->comboBox_Location->addItems(project->getLocations());
    ui->comboBox_Location->setCurrentText(map->location);

    ui->comboBox_Visibility->addItems(project->getVisibilities());
    ui->comboBox_Visibility->setCurrentText(map->visibility);

    ui->comboBox_Weather->addItems(project->getWeathers());
    ui->comboBox_Weather->setCurrentText(map->weather);

    ui->comboBox_Type->addItems(project->getMapTypes());
    ui->comboBox_Type->setCurrentText(map->type);

    ui->comboBox_BattleScene->addItems(project->getBattleScenes());
    ui->comboBox_BattleScene->setCurrentText(map->battle_scene);

    ui->checkBox_ShowLocation->setChecked(map->show_location.toInt() > 0 || map->show_location == "TRUE");
}

void MainWindow::on_comboBox_Song_activated(const QString &song)
{
    if (editor && editor->map) {
        editor->map->song = song;
    }
}

void MainWindow::on_comboBox_Location_activated(const QString &location)
{
    if (editor && editor->map) {
        editor->map->location = location;
    }
}

void MainWindow::on_comboBox_Visibility_activated(const QString &visibility)
{
    if (editor && editor->map) {
        editor->map->visibility = visibility;
    }
}

void MainWindow::on_comboBox_Weather_activated(const QString &weather)
{
    if (editor && editor->map) {
        editor->map->weather = weather;
    }
}

void MainWindow::on_comboBox_Type_activated(const QString &type)
{
    if (editor && editor->map) {
        editor->map->type = type;
    }
}

void MainWindow::on_comboBox_BattleScene_activated(const QString &battle_scene)
{
    if (editor && editor->map) {
        editor->map->battle_scene = battle_scene;
    }
}

void MainWindow::on_checkBox_ShowLocation_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->show_location = "TRUE";
        } else {
            editor->map->show_location = "FALSE";
        }
    }
}

void MainWindow::loadDataStructures() {
    Project *project = editor->project;
    project->readMapAttributesTable();
    project->readAllMapAttributes();
    project->readItemNames();
    project->readFlagNames();
    project->readVarNames();
}

void MainWindow::populateMapList() {
    Project *project = editor->project;

    QIcon mapFolderIcon;
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);

    QIcon folderIcon;
    folderIcon.addFile(QStringLiteral(":/icons/folder_closed.ico"), QSize(), QIcon::Normal, QIcon::Off);

    mapIcon = new QIcon;
    mapIcon->addFile(QStringLiteral(":/icons/map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapIcon->addFile(QStringLiteral(":/icons/image.ico"), QSize(), QIcon::Normal, QIcon::On);

    mapListModel = new QStandardItemModel;
    mapGroupsModel = new QList<QStandardItem*>;

    QStandardItem *entry = new QStandardItem;
    entry->setText(project->getProjectTitle());
    entry->setIcon(folderIcon);
    entry->setEditable(false);
    mapListModel->appendRow(entry);

    QStandardItem *maps = new QStandardItem;
    maps->setText("maps");
    maps->setIcon(folderIcon);
    maps->setEditable(false);
    entry->appendRow(maps);

    project->readMapGroups();
    for (int i = 0; i < project->groupNames->length(); i++) {
        QString group_name = project->groupNames->value(i);
        QStandardItem *group = new QStandardItem;
        group->setText(group_name);
        group->setIcon(mapFolderIcon);
        group->setEditable(false);
        group->setData(group_name, Qt::UserRole);
        group->setData("map_group", MapListUserRoles::TypeRole);
        group->setData(i, MapListUserRoles::GroupRole);
        maps->appendRow(group);
        mapGroupsModel->append(group);
        QStringList names = project->groupedMapNames.value(i);
        for (int j = 0; j < names.length(); j++) {
            QString map_name = names.value(j);
            QStandardItem *map = createMapItem(map_name, i, j);
            group->appendRow(map);
        }
    }

    // Right-clicking on items in the map list tree view brings up a context menu.
    ui->mapList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapList, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(onOpenMapListContextMenu(const QPoint &)));

    ui->mapList->setModel(mapListModel);
    ui->mapList->setUpdatesEnabled(true);
    ui->mapList->expandToDepth(2);
    ui->mapList->repaint();
}

QStandardItem* MainWindow::createMapItem(QString mapName, int groupNum, int inGroupNum) {
    QStandardItem *map = new QStandardItem;
    map->setText(QString("[%1.%2] ").arg(groupNum).arg(inGroupNum, 2, 10, QLatin1Char('0')) + mapName);
    map->setIcon(*mapIcon);
    map->setEditable(false);
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListUserRoles::TypeRole);
    return map;
}

void MainWindow::onOpenMapListContextMenu(const QPoint &point)
{
    QModelIndex index = ui->mapList->indexAt(point);
    if (!index.isValid()) {
        return;
    }

    QStandardItem *selectedItem = mapListModel->itemFromIndex(index);
    QVariant itemType = selectedItem->data(MapListUserRoles::TypeRole);
    if (!itemType.isValid()) {
        return;
    }

    // Build custom context menu depending on which type of item was selected (map group, map name, etc.)
    if (itemType == "map_group") {
        QString groupName = selectedItem->data(Qt::UserRole).toString();
        int groupNum = selectedItem->data(MapListUserRoles::GroupRole).toInt();
        QMenu* menu = new QMenu();
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map to Group"))->setData(groupNum);
        connect(actions, SIGNAL(triggered(QAction*)), this, SLOT(onAddNewMapToGroupClick(QAction*)));
        menu->exec(QCursor::pos());
    }
}

void MainWindow::onAddNewMapToGroupClick(QAction* triggeredAction)
{
    int groupNum = triggeredAction->data().toInt();
    QStandardItem* groupItem = mapGroupsModel->at(groupNum);

    QString newMapName = editor->project->getNewMapName();
    Map* newMap = editor->project->addNewMapToGroup(newMapName, groupNum);
    editor->project->saveMap(newMap);
    editor->project->saveAllDataStructures();

    int numMapsInGroup = groupItem->rowCount();
    QStandardItem *newMapItem = createMapItem(newMapName, groupNum, numMapsInGroup);
    groupItem->appendRow(newMapItem);

    setMap(newMapName);
}

void MainWindow::on_mapList_activated(const QModelIndex &index)
{
    QVariant data = index.data(Qt::UserRole);
    if (!data.isNull()) {
        setMap(data.toString());
    }
    //updateMapList();
}

void MainWindow::markAllEdited(QAbstractItemModel *model) {
    QList<QModelIndex> list;
    list.append(QModelIndex());
    while (list.length()) {
        QModelIndex parent = list.takeFirst();
        for (int i = 0; i < model->rowCount(parent); i++) {
            QModelIndex index = model->index(i, 0, parent);
            if (model->hasChildren(index)) {
                list.append(index);
            }
            markEdited(index);
        }
    }
}

void MainWindow::markEdited(QModelIndex index) {
    QVariant data = index.data(Qt::UserRole);
    if (!data.isNull()) {
        QString map_name = data.toString();
        if (editor->project) {
            if (editor->project->map_cache->contains(map_name)) {
                // Just mark anything that's been opened for now.
                // TODO if (project->getMap()->saved)
                //ui->mapList->setExpanded(index, true);
                ui->mapList->setExpanded(index, editor->project->map_cache->value(map_name)->hasUnsavedChanges());
            }
        }
    }
}

void MainWindow::updateMapList() {
    QAbstractItemModel *model = ui->mapList->model();
    markAllEdited(model);
}

void MainWindow::on_action_Save_Project_triggered()
{
    editor->saveProject();
    updateMapList();
}

void MainWindow::undo() {
    editor->undo();
}

void MainWindow::redo() {
    editor->redo();
}

void MainWindow::on_action_Save_triggered() {
    editor->save();
    updateMapList();
}

void MainWindow::on_tabWidget_2_currentChanged(int index)
{
    if (index == 0) {
        editor->setEditingMap();
    } else if (index == 1) {
        editor->setEditingCollision();
    }
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 0) {
        on_tabWidget_2_currentChanged(ui->tabWidget_2->currentIndex());
    } else if (index == 1) {
        editor->setEditingObjects();
    } else if (index == 3) {
        editor->setEditingConnections();
    }
}

void MainWindow::on_actionUndo_triggered()
{
    undo();
}

void MainWindow::on_actionRedo_triggered()
{
    redo();
}

void MainWindow::on_toolButton_newObject_clicked()
{
    if (editor) {
        DraggablePixmapItem *object = editor->addNewEvent();
        if (object) {
            //if (editor->selected_events->length()) {
                editor->selectMapObject(object, true);
            //}
        }
        updateSelectedObjects();
    }
}

// Should probably just pass layout and let the editor work it out
void MainWindow::updateSelectedObjects() {

    QList<DraggablePixmapItem *> *all_events = editor->getObjects();
    QList<DraggablePixmapItem *> *events = all_events;

    if (editor->selected_events && editor->selected_events->length()) {
        events = editor->selected_events;
    }

    QMap<QString, int> map_obj_gfx_constants = editor->project->getMapObjGfxConstants();

    QList<ObjectPropertiesFrame *> frames;

    for (DraggablePixmapItem *item : *events) {
        ObjectPropertiesFrame *frame = new ObjectPropertiesFrame;
//        frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        QSpinBox *x = frame->ui->spinBox_x;
        QSpinBox *y = frame->ui->spinBox_y;
        QSpinBox *z = frame->ui->spinBox_z;

        x->setValue(item->event->x());
        connect(x, SIGNAL(valueChanged(QString)), item, SLOT(set_x(QString)));
        connect(item, SIGNAL(xChanged(int)), x, SLOT(setValue(int)));

        y->setValue(item->event->y());
        connect(y, SIGNAL(valueChanged(QString)), item, SLOT(set_y(QString)));
        connect(item, SIGNAL(yChanged(int)), y, SLOT(setValue(int)));

        z->setValue(item->event->elevation());
        connect(z, SIGNAL(valueChanged(QString)), item, SLOT(set_elevation(QString)));
        connect(item, SIGNAL(elevationChanged(int)), z, SLOT(setValue(int)));

        QFont font;
        font.setCapitalization(QFont::Capitalize);
        frame->ui->label_name->setFont(font);
        QString event_type = item->event->get("event_type");
        QString map_name = item->event->get("map_name");
        frame->ui->label_name->setText(
            QString("%1 %2 %3")
                .arg(map_name)
                .arg(event_type)
                .arg(editor->project->getMap(map_name)->events.value(event_type).indexOf(item->event) + 1)
        );

        frame->ui->label_spritePixmap->setPixmap(item->event->pixmap);
        connect(item, SIGNAL(spriteChanged(QPixmap)), frame->ui->label_spritePixmap, SLOT(setPixmap(QPixmap)));

        frame->ui->sprite->setVisible(false);

        QMap<QString, QString> field_labels;
        field_labels["script_label"] = "Script";
        field_labels["event_flag"] = "Event Flag";
        field_labels["replacement"] = "Replacement";
        field_labels["behavior"] = "Behavior";
        field_labels["radius_x"] = "Movement Radius X";
        field_labels["radius_y"] = "Movement Radius Y";
        field_labels["property"] = "Property";
        field_labels["sight_radius"] = "Sight Radius";
        field_labels["destination_warp"] = "Destination Warp";
        field_labels["destination_map_name"] = "Destination Map";
        field_labels["script_var"] = "Var";
        field_labels["script_var_value"] = "Var Value";
        field_labels["type"] = "Type";
        field_labels["item"] = "Item";
        field_labels["item_unknown5"] = "Unknown 5";
        field_labels["item_unknown6"] = "Unknown 6";
        field_labels["weather"] = "Weather";
        field_labels["flag"] = "Flag";
        field_labels["secret_base_map"] = "Secret Base Map";

        QStringList fields;

        if (event_type == "object") {

            frame->ui->sprite->setVisible(true);
            frame->ui->comboBox_sprite->addItems(map_obj_gfx_constants.keys());
            frame->ui->comboBox_sprite->setCurrentText(item->event->get("sprite"));
            connect(frame->ui->comboBox_sprite, SIGNAL(activated(QString)), item, SLOT(set_sprite(QString)));

            /*
            frame->ui->script->setVisible(true);
            frame->ui->comboBox_script->addItem(item->event->get("script_label"));
            frame->ui->comboBox_script->setCurrentText(item->event->get("script_label"));
            //item->bind(frame->ui->comboBox_script, "script_label");
            connect(frame->ui->comboBox_script, SIGNAL(activated(QString)), item, SLOT(set_script(QString)));
            //connect(frame->ui->comboBox_script, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), item, [item](QString script_label){ item->event->put("script_label", script_label); });
            //connect(item, SIGNAL(scriptChanged(QString)), frame->ui->comboBox_script, SLOT(setValue(QString)));
            */

            fields << "behavior";
            fields << "radius_x";
            fields << "radius_y";
            fields << "script_label";
            fields << "event_flag";
            fields << "replacement";
            fields << "property";
            fields << "sight_radius";
        }
        else if (event_type == "warp") {
            fields << "destination_warp";
            fields << "destination_map_name";
        }
        else if (event_type == "trap") {
            fields << "script_label";
            fields << "script_var";
            fields << "script_var_value";
        }
        else if (event_type == "trap_weather") {
            fields << "weather";
        }
        else if (event_type == "sign") {
            fields << "type";
            fields << "script_label";
        }
        else if (event_type == "event_hidden_item") {
            fields << "item";
            fields << "flag";
        }
        else if (event_type == "event_secret_base") {
            fields << "secret_base_map";
        }

        for (QString key : fields) {
            QWidget *widget = new QWidget(frame);
            QFormLayout *fl = new QFormLayout(widget);
            fl->setContentsMargins(9, 0, 9, 0);
            QComboBox *combo = new QComboBox(widget);
            combo->setEditable(true);

            QString value = item->event->get(key);
            if (key == "destination_map_name") {
                if (!editor->project->mapNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->mapNames);
            } else if (key == "item") {
                if (!editor->project->itemNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->itemNames);
            } else if (key == "flag") {
                if (!editor->project->flagNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->flagNames);
            } else if (key == "script_var") {
                if (!editor->project->varNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->varNames);
            } else {
                combo->addItem(value);
            }
            combo->setCurrentText(value);

            fl->addRow(new QLabel(field_labels[key], widget), combo);
            widget->setLayout(fl);
            frame->layout()->addWidget(widget);

            item->bind(combo, key);
        }

        frames.append(frame);

    }

    //int scroll = ui->scrollArea_4->verticalScrollBar()->value();

    QWidget *target = ui->scrollAreaWidgetContents;

    if (target->children().length()) {
        qDeleteAll(target->children());
    }

    QVBoxLayout *layout = new QVBoxLayout(target);
    target->setLayout(layout);
    ui->scrollArea_4->setWidgetResizable(true);
    ui->scrollArea_4->setWidget(target);

    for (ObjectPropertiesFrame *frame : frames) {
        layout->addWidget(frame);
    }

    layout->addStretch(1);

    // doesn't work
    //QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    //ui->scrollArea_4->ensureVisible(0, scroll);
}

void MainWindow::on_toolButton_deleteObject_clicked()
{
    if (editor && editor->selected_events) {
        if (editor->selected_events->length()) {
            for (DraggablePixmapItem *item : *editor->selected_events) {
                editor->deleteEvent(item->event);
                if (editor->scene->items().contains(item)) {
                    editor->scene->removeItem(item);
                }
                editor->selected_events->removeOne(item);
            }
            updateSelectedObjects();
        }
    }
}

void MainWindow::on_toolButton_Paint_clicked()
{
    editor->map_edit_mode = "paint";
    checkToolButtons();
}

void MainWindow::on_toolButton_Select_clicked()
{
    editor->map_edit_mode = "select";
    checkToolButtons();
}

void MainWindow::on_toolButton_Fill_clicked()
{
    editor->map_edit_mode = "fill";
    checkToolButtons();
}

void MainWindow::on_toolButton_Dropper_clicked()
{
    editor->map_edit_mode = "pick";
    checkToolButtons();
}

void MainWindow::checkToolButtons() {
    ui->toolButton_Paint->setChecked(editor->map_edit_mode == "paint");
    ui->toolButton_Select->setChecked(editor->map_edit_mode == "select");
    ui->toolButton_Fill->setChecked(editor->map_edit_mode == "fill");
    ui->toolButton_Dropper->setChecked(editor->map_edit_mode == "pick");
}

void MainWindow::onMapChanged(Map *map) {
    updateMapList();
}

void MainWindow::on_action_Export_Map_Image_triggered()
{
    QString defaultFilepath = QString("%1/%2.png").arg(editor->project->root).arg(editor->map->name);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Map Image", defaultFilepath, "Image Files (*.png *.jpg *.bmp)");
    if (!filepath.isEmpty()) {
        editor->map_item->pixmap().save(filepath);
    }
}

void MainWindow::on_comboBox_ConnectionDirection_currentIndexChanged(const QString &direction)
{
    editor->setCurrentConnectionDirection(direction);
}

void MainWindow::on_spinBox_ConnectionOffset_valueChanged(int offset)
{
    editor->updateConnectionOffset(offset);
}

void MainWindow::on_comboBox_ConnectedMap_currentTextChanged(const QString &mapName)
{
    editor->updateConnectionMap(mapName, ui->comboBox_ConnectionDirection->currentText().toLower());
}

void MainWindow::on_pushButton_AddConnection_clicked()
{
    editor->addNewConnection();
}

void MainWindow::on_pushButton_RemoveConnection_clicked()
{
    editor->removeCurrentConnection();
}
