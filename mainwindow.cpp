#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "project.h"

#include <QDebug>
#include <QFileDialog>
#include <QStandardItemModel>
#include <QShortcut>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("pretmap");

    editor = new Editor;

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this, SLOT(redo()));
    ui->setupUi(this);

    QSettings settings;
    QString key = "recent_projects";
    if (settings.contains(key)) {
        QString default_dir = settings.value(key).toStringList().last();
        openProject(default_dir);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openProject(QString dir) {
    bool already_open = (editor->project != NULL) && (editor->project->root == dir);
    if (!already_open) {
        editor->project = new Project;
        editor->project->root = dir;
        populateMapList();
        setMap(getDefaultMap());
    } else {
        populateMapList();
    }
}

QString MainWindow::getDefaultMap() {
    QSettings settings;
    QString key = "project:" + editor->project->root;
    if (settings.contains(key)) {
        QMap<QString, QVariant> qmap = settings.value(key).toMap();
        if (qmap.contains("recent_map")) {
            QString map_name = qmap.value("recent_map").toString();
            return map_name;
        }
    }
    // Failing that, just get the first map in the list.
    for (int i = 0; i < editor->project->groupedMapNames->length(); i++) {
        QStringList *list = editor->project->groupedMapNames->value(i);
        if (list->length()) {
            return list->value(0);
        }
    }
    return NULL;
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
    if (map_name.isNull()) {
        return;
    }
    editor->setMap(map_name);

    if (ui->tabWidget->currentIndex() == 1) {
        editor->setEditingObjects();
    } else {
        if (ui->tabWidget_2->currentIndex() == 1) {
            editor->setEditingCollision();
        } else {
            editor->setEditingMap();
        }
    }

    ui->graphicsView_Map->setScene(editor->scene);
    ui->graphicsView_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Map->setFixedSize(editor->scene->width() + 2, editor->scene->height() + 2);

    ui->graphicsView_Objects_Map->setScene(editor->scene);
    ui->graphicsView_Objects_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Objects_Map->setFixedSize(editor->scene->width() + 2, editor->scene->height() + 2);

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
    Map *map = editor->map;
    Project *project = editor->project;

    QStringList songs = project->getSongNames();
    ui->comboBox_Song->clear();
    ui->comboBox_Song->addItems(songs);
    QString song = map->song;
    if (!songs.contains(song)) {
        song = project->getSongName(song.toInt());
    }
    ui->comboBox_Song->setCurrentText(song);

    ui->comboBox_Location->clear();
    ui->comboBox_Location->addItems(project->getLocations());
    ui->comboBox_Location->setCurrentText(map->location);

    ui->comboBox_Visibility->clear();
    ui->comboBox_Visibility->addItems(project->getVisibilities());
    ui->comboBox_Visibility->setCurrentText(map->visibility);

    ui->comboBox_Weather->clear();
    ui->comboBox_Weather->addItems(project->getWeathers());
    ui->comboBox_Weather->setCurrentText(map->weather);

    ui->comboBox_Type->clear();
    ui->comboBox_Type->addItems(project->getMapTypes());
    ui->comboBox_Type->setCurrentText(map->type);

    ui->comboBox_BattleScene->clear();
    ui->comboBox_BattleScene->addItems(project->getBattleScenes());
    ui->comboBox_BattleScene->setCurrentText(map->battle_scene);

    ui->checkBox_ShowLocation->setChecked(map->show_location.toInt() > 0 || map->show_location == "TRUE");
}

void MainWindow::on_comboBox_Song_activated(const QString &song)
{
    editor->map->song = song;
}

void MainWindow::on_comboBox_Location_activated(const QString &location)
{
    editor->map->location = location;
}

void MainWindow::on_comboBox_Visibility_activated(const QString &visibility)
{
    editor->map->visibility = visibility;
}

void MainWindow::on_comboBox_Weather_activated(const QString &weather)
{
    editor->map->weather = weather;
}

void MainWindow::on_comboBox_Type_activated(const QString &type)
{
    editor->map->type = type;
}

void MainWindow::on_comboBox_BattleScene_activated(const QString &battle_scene)
{
    editor->map->battle_scene = battle_scene;
}

void MainWindow::on_checkBox_ShowLocation_clicked(bool checked)
{
    if (checked) {
        editor->map->show_location = "TRUE";
    } else {
        editor->map->show_location = "FALSE";
    }
}


void MainWindow::populateMapList() {
    Project *project = editor->project;

    QIcon mapFolderIcon;
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);

    QIcon folderIcon;
    folderIcon.addFile(QStringLiteral(":/icons/folder_closed.ico"), QSize(), QIcon::Normal, QIcon::Off);

    QIcon mapIcon;
    mapIcon.addFile(QStringLiteral(":/icons/map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapIcon.addFile(QStringLiteral(":/icons/image.ico"), QSize(), QIcon::Normal, QIcon::On);

    QStandardItemModel *model = new QStandardItemModel;

    QStandardItem *entry = new QStandardItem;
    entry->setText(project->getProjectTitle());
    entry->setIcon(folderIcon);
    entry->setEditable(false);
    model->appendRow(entry);

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
        maps->appendRow(group);
        QStringList *names = project->groupedMapNames->value(i);
        for (int j = 0; j < names->length(); j++) {
            QString map_name = names->value(j);
            QStandardItem *map = new QStandardItem;
            map->setText(QString("[%1.%2] ").arg(i).arg(j, 2, 10, QLatin1Char('0')) + map_name);
            map->setIcon(mapIcon);
            map->setEditable(false);
            map->setData(map_name, Qt::UserRole);
            group->appendRow(map);
            //ui->mapList->setExpanded(model->indexFromItem(map), false); // redundant
        }
    }

    ui->mapList->setModel(model);
    ui->mapList->setUpdatesEnabled(true);
    ui->mapList->expandToDepth(2);
    ui->mapList->repaint();
}

void MainWindow::on_mapList_activated(const QModelIndex &index)
{
    QVariant data = index.data(Qt::UserRole);
    if (!data.isNull()) {
        setMap(data.toString());
    }
    updateMapList();
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
                ui->mapList->setExpanded(index, true);
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
