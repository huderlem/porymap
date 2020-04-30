#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutporymap.h"
#include "project.h"
#include "log.h"
#include "editor.h"
#include "eventpropertiesframe.h"
#include "ui_eventpropertiesframe.h"
#include "bordermetatilespixmapitem.h"
#include "currentselectedmetatilespixmapitem.h"
#include "adjustingstackedwidget.h"

#include <QFileDialog>
#include <QDirIterator>
#include <QStandardItemModel>
#include <QShortcut>
#include <QSpinBox>
#include <QTextEdit>
#include <QSpacerItem>
#include <QFont>
#include <QScrollBar>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QScroller>
#include <math.h>
#include <QProcess>
#include <QSysInfo>
#include <QDesktopServices>
#include <QMatrix>
#include <QSignalBlocker>
#include <QSet>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    selectedObject(nullptr),
    selectedWarp(nullptr),
    selectedTrigger(nullptr),
    selectedBG(nullptr),
    selectedHealspot(nullptr),
    isProgrammaticEventTabChange(false)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("porymap");
    QApplication::setApplicationDisplayName("porymap");
    QApplication::setWindowIcon(QIcon(":/icons/porymap-icon-2.ico"));
    ui->setupUi(this);

    this->initWindow();
    if (!this->openRecentProject()) {
        // Re-initialize everything to a blank slate if opening the recent project failed.
        this->initWindow();
    }
    on_toolButton_Paint_clicked();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initWindow() {
    porymapConfig.load();
    this->initCustomUI();
    this->initExtraSignals();
    this->initExtraShortcuts();
    this->initEditor();
    this->initMiscHeapObjects();
    this->initMapSortOrder();
    this->restoreWindowState();
}

void MainWindow::initExtraShortcuts() {
    new QShortcut(QKeySequence("Ctrl+Shift+Z"), this, SLOT(redo()));
    new QShortcut(QKeySequence("Ctrl+0"), this, SLOT(resetMapViewScale()));
    new QShortcut(QKeySequence("Ctrl+G"), ui->checkBox_ToggleGrid, SLOT(toggle()));
    ui->actionZoom_In->setShortcuts({QKeySequence("Ctrl++"), QKeySequence("Ctrl+=")});
}

void MainWindow::initCustomUI() {
    // Set up the tab bar
    ui->mainTabBar->addTab("Map");
    ui->mainTabBar->setTabIcon(0, QIcon(QStringLiteral(":/icons/map.ico")));
    ui->mainTabBar->addTab("Events");
    ui->mainTabBar->addTab("Header");
    ui->mainTabBar->addTab("Connections");
    ui->mainTabBar->addTab("Wild Pokemon");
    ui->mainTabBar->setTabIcon(4, QIcon(QStringLiteral(":/icons/tall_grass.ico")));

    // Right-clicking on items in the map list tree view brings up a context menu.
    ui->mapList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapList, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(onOpenMapListContextMenu(const QPoint &)));

    QStackedWidget *stack = ui->stackedWidget_WildMons;
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;
    connect(labelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        stack->setCurrentIndex(index);
    });
}

void MainWindow::initExtraSignals() {
    connect(ui->newEventToolButton, SIGNAL(newEventAdded(QString)), this, SLOT(addNewEvent(QString)));
    connect(ui->tabWidget_EventType, &QTabWidget::currentChanged, this, &MainWindow::eventTabChanged);
}

void MainWindow::initEditor() {
    this->editor = new Editor(ui);
    connect(this->editor, SIGNAL(objectsChanged()), this, SLOT(updateObjects()));
    connect(this->editor, SIGNAL(selectedObjectsChanged()), this, SLOT(updateSelectedObjects()));
    connect(this->editor, SIGNAL(loadMapRequested(QString, QString)), this, SLOT(onLoadMapRequested(QString, QString)));
    connect(this->editor, SIGNAL(tilesetChanged(QString)), this, SLOT(onTilesetChanged(QString)));
    connect(this->editor, SIGNAL(warpEventDoubleClicked(QString,QString)), this, SLOT(openWarpMap(QString,QString)));
    connect(this->editor, SIGNAL(currentMetatilesSelectionChanged()), this, SLOT(currentMetatilesSelectionChanged()));
    connect(this->editor, &Editor::wheelZoom, this, &MainWindow::scaleMapView);

    this->loadUserSettings();
}

void MainWindow::initMiscHeapObjects() {
    mapIcon = new QIcon(QStringLiteral(":/icons/map.ico"));
    mapEditedIcon = new QIcon(QStringLiteral(":/icons/map_edited.ico"));
    mapOpenedIcon = new QIcon(QStringLiteral(":/icons/map_opened.ico"));

    mapListModel = new QStandardItemModel;
    mapGroupItemsList = new QList<QStandardItem*>;
    mapListProxyModel = new FilterChildrenProxyModel;

    mapListProxyModel->setSourceModel(mapListModel);
    ui->mapList->setModel(mapListProxyModel);

    eventTabObjectWidget = ui->tab_Objects;
    eventTabWarpWidget = ui->tab_Warps;
    eventTabTriggerWidget = ui->tab_Triggers;
    eventTabBGWidget = ui->tab_BGs;
    eventTabHealspotWidget = ui->tab_Healspots;
    eventTabMultipleWidget = ui->tab_Multiple;
    ui->tabWidget_EventType->clear();
}

void MainWindow::initMapSortOrder() {
    QMenu *mapSortOrderMenu = new QMenu(this);
    QActionGroup *mapSortOrderActionGroup = new QActionGroup(ui->toolButton_MapSortOrder);

    mapSortOrderMenu->addAction(ui->actionSort_by_Group);
    mapSortOrderMenu->addAction(ui->actionSort_by_Area);
    mapSortOrderMenu->addAction(ui->actionSort_by_Layout);
    ui->toolButton_MapSortOrder->setMenu(mapSortOrderMenu);

    mapSortOrderActionGroup->addAction(ui->actionSort_by_Group);
    mapSortOrderActionGroup->addAction(ui->actionSort_by_Area);
    mapSortOrderActionGroup->addAction(ui->actionSort_by_Layout);

    connect(ui->toolButton_MapSortOrder, &QToolButton::triggered, this, &MainWindow::mapSortOrder_changed);

    QAction* sortOrder = ui->toolButton_MapSortOrder->menu()->actions()[mapSortOrder];
    ui->toolButton_MapSortOrder->setIcon(sortOrder->icon());
    sortOrder->setChecked(true);
}

void MainWindow::setProjectSpecificUIVisibility()
{
    ui->actionUse_Encounter_Json->setChecked(projectConfig.getEncounterJsonActive());
    ui->actionUse_Poryscript->setChecked(projectConfig.getUsePoryScript());

    ui->mainTabBar->setTabEnabled(4, projectConfig.getEncounterJsonActive());

    switch (projectConfig.getBaseGameVersion())
    {
    case BaseGameVersion::pokeruby:
        ui->checkBox_AllowRunning->setVisible(false);
        ui->checkBox_AllowBiking->setVisible(false);
        ui->checkBox_AllowEscapeRope->setVisible(false);
        ui->spinBox_FloorNumber->setVisible(false);
        ui->label_AllowRunning->setVisible(false);
        ui->label_AllowBiking->setVisible(false);
        ui->label_AllowEscapeRope->setVisible(false);
        ui->label_FloorNumber->setVisible(false);
        ui->newEventToolButton->newWeatherTriggerAction->setVisible(true);
        ui->newEventToolButton->newSecretBaseAction->setVisible(true);
        ui->actionRegion_Map_Editor->setVisible(true);
        break;
    case BaseGameVersion::pokeemerald:
        ui->checkBox_AllowRunning->setVisible(true);
        ui->checkBox_AllowBiking->setVisible(true);
        ui->checkBox_AllowEscapeRope->setVisible(true);
        ui->spinBox_FloorNumber->setVisible(false);
        ui->label_AllowRunning->setVisible(true);
        ui->label_AllowBiking->setVisible(true);
        ui->label_AllowEscapeRope->setVisible(true);
        ui->label_FloorNumber->setVisible(false);
        ui->newEventToolButton->newWeatherTriggerAction->setVisible(true);
        ui->newEventToolButton->newSecretBaseAction->setVisible(true);
        ui->actionRegion_Map_Editor->setVisible(true);
        break;
    case BaseGameVersion::pokefirered:
        ui->checkBox_AllowRunning->setVisible(true);
        ui->checkBox_AllowBiking->setVisible(true);
        ui->checkBox_AllowEscapeRope->setVisible(true);
        ui->spinBox_FloorNumber->setVisible(true);
        ui->label_AllowRunning->setVisible(true);
        ui->label_AllowBiking->setVisible(true);
        ui->label_AllowEscapeRope->setVisible(true);
        ui->label_FloorNumber->setVisible(true);
        ui->newEventToolButton->newWeatherTriggerAction->setVisible(false);
        ui->newEventToolButton->newSecretBaseAction->setVisible(false);
        // TODO: pokefirered is not set up for the Region Map Editor and vice versa. 
        //       porymap will crash on attempt. Remove below once resolved
        ui->actionRegion_Map_Editor->setVisible(false);
        break;
    }
}

void MainWindow::mapSortOrder_changed(QAction *action)
{
    QList<QAction*> items = ui->toolButton_MapSortOrder->menu()->actions();
    int i = 0;
    for (; i < items.count(); i++)
    {
        if (items[i] == action)
        {
            break;
        }
    }

    if (i != mapSortOrder)
    {
        ui->toolButton_MapSortOrder->setIcon(action->icon());
        mapSortOrder = static_cast<MapSortOrder>(i);
        porymapConfig.setMapSortOrder(mapSortOrder);
        if (isProjectOpen())
        {
            sortMapList();
        }
    }
}

void MainWindow::on_lineEdit_filterBox_textChanged(const QString &arg1)
{
    mapListProxyModel->setFilterRegExp(QRegExp(arg1, Qt::CaseInsensitive, QRegExp::FixedString));
    if (arg1.isEmpty()) {
        ui->mapList->collapseAll();
    } else {
        ui->mapList->expandToDepth(0);
    }
    ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(editor->map->name)), true);
    ui->mapList->scrollTo(mapListProxyModel->mapFromSource(mapListIndexes.value(editor->map->name)), QAbstractItemView::PositionAtCenter);
}

void MainWindow::loadUserSettings() {
    ui->actionBetter_Cursors->setChecked(porymapConfig.getPrettyCursors());
    this->editor->settings->betterCursors = porymapConfig.getPrettyCursors();
    ui->actionPlayer_View_Rectangle->setChecked(porymapConfig.getShowPlayerView());
    this->editor->settings->playerViewRectEnabled = porymapConfig.getShowPlayerView();
    ui->actionCursor_Tile_Outline->setChecked(porymapConfig.getShowCursorTile());
    this->editor->settings->cursorTileRectEnabled = porymapConfig.getShowCursorTile();
    mapSortOrder = porymapConfig.getMapSortOrder();
    ui->horizontalSlider_CollisionTransparency->blockSignals(true);
    this->editor->collisionOpacity = static_cast<qreal>(porymapConfig.getCollisionOpacity()) / 100;
    ui->horizontalSlider_CollisionTransparency->setValue(porymapConfig.getCollisionOpacity());
    ui->horizontalSlider_CollisionTransparency->blockSignals(false);
    ui->horizontalSlider_MetatileZoom->blockSignals(true);
    ui->horizontalSlider_MetatileZoom->setValue(porymapConfig.getMetatilesZoom());
    ui->horizontalSlider_MetatileZoom->blockSignals(false);
    ui->actionMonitor_Project_Files->setChecked(porymapConfig.getMonitorFiles());
    setTheme(porymapConfig.getTheme());
}

void MainWindow::restoreWindowState() {
    logInfo("Restoring window geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getGeometry();
    this->restoreGeometry(geometry.value("window_geometry"));
    this->restoreState(geometry.value("window_state"));
    this->ui->splitter_map->restoreState(geometry.value("map_splitter_state"));
    this->ui->splitter_main->restoreState(geometry.value("main_splitter_state"));
}

void MainWindow::setTheme(QString theme) {
    if (theme == "default") {
        setStyleSheet("");
    } else {
        QFile File(QString(":/themes/%1.qss").arg(theme));
        File.open(QFile::ReadOnly);
        QString stylesheet = QLatin1String(File.readAll());
        setStyleSheet(stylesheet);
    }
}

bool MainWindow::openRecentProject() {
    QString default_dir = porymapConfig.getRecentProject();
    if (!default_dir.isNull() && default_dir.length() > 0) {
        logInfo(QString("Opening recent project: '%1'").arg(default_dir));
        return openProject(default_dir);
    }

    return true;
}

bool MainWindow::openProject(QString dir) {
    if (dir.isNull()) {
        return false;
    }

    QString nativeDir = QDir::toNativeSeparators(dir);

    this->statusBar()->showMessage(QString("Opening project %1").arg(nativeDir));

    bool success = true;
    projectConfig.setProjectDir(dir);
    projectConfig.load();

    this->closeSupplementaryWindows();
    this->setProjectSpecificUIVisibility();

    bool already_open = isProjectOpen() && (editor->project->root == dir);
    if (!already_open) {
        editor->closeProject();
        editor->project = new Project(this);
        QObject::connect(editor->project, SIGNAL(reloadProject()), this, SLOT(on_action_Reload_Project_triggered()));
        QObject::connect(editor->project, &Project::uncheckMonitorFilesAction, [this] () { ui->actionMonitor_Project_Files->setChecked(false); });
        on_actionMonitor_Project_Files_triggered(porymapConfig.getMonitorFiles());
        editor->project->set_root(dir);
        success = loadDataStructures()
               && populateMapList()
               && setMap(getDefaultMap(), true);
    } else {
        QString open_map = editor->map->name;
        editor->project->fileWatcher.removePaths(editor->project->fileWatcher.files());
        editor->project->clearMapCache();
        editor->project->clearTilesetCache();
        success = loadDataStructures() && populateMapList() && setMap(open_map, true);
    }

    if (success) {
        setWindowTitle(editor->project->getProjectTitle());
        this->statusBar()->showMessage(QString("Opened project %1").arg(nativeDir));
    } else {
        this->statusBar()->showMessage(QString("Failed to open project %1").arg(nativeDir));
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening the project %1. Please see %2 for full error details.\n\n%3")
                .arg(dir)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Project", errorMsg);

    }

    return success;
}

bool MainWindow::isProjectOpen() {
    return editor != nullptr && editor->project != nullptr;
}

QString MainWindow::getDefaultMap() {
    if (editor && editor->project) {
        QList<QStringList> names = editor->project->groupedMapNames;
        if (!names.isEmpty()) {
            QString recentMap = porymapConfig.getRecentMap();
            if (!recentMap.isNull() && recentMap.length() > 0) {
                for (int i = 0; i < names.length(); i++) {
                    if (names.value(i).contains(recentMap)) {
                        return recentMap;
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
    QString recent = ".";
    if (!porymapConfig.getRecentMap().isNull() && porymapConfig.getRecentMap().length() > 0) {
        recent = porymapConfig.getRecentMap();
    }
    QString dir = getExistingDirectory(recent);
    if (!dir.isEmpty()) {
        porymapConfig.setRecentProject(dir);
        if (!openProject(dir)) {
            this->initWindow();
        }
    }
}

void MainWindow::on_action_Reload_Project_triggered() {
    // TODO: when undo history is complete show only if has unsaved changes
    QMessageBox warning(this);
    warning.setText("WARNING");
    warning.setInformativeText("Reloading this project will discard any unsaved changes.");
    warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    warning.setIcon(QMessageBox::Warning);

    if (warning.exec() == QMessageBox::Ok) {
        openProject(editor->project->root);
    }
}

bool MainWindow::setMap(QString map_name, bool scrollTreeView) {
    logInfo(QString("Setting map to '%1'").arg(map_name));
    if (map_name.isEmpty()) {
        return false;
    }

    if (!editor->setMap(map_name)) {
        logWarn(QString("Failed to set map to '%1'").arg(map_name));
        return false;
    }

    if (editor->map != nullptr && !editor->map->name.isNull()) {
        ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(editor->map->name)), false);
    }

    redrawMapScene();
    displayMapProperties();

    if (scrollTreeView) {
        // Make sure we clear the filter first so we actually have a scroll target
        mapListProxyModel->setFilterRegExp(QString());
        ui->mapList->setCurrentIndex(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)));
        ui->mapList->scrollTo(ui->mapList->currentIndex(), QAbstractItemView::PositionAtCenter);
    }

    ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)), true);

    setWindowTitle(map_name + " - " + editor->project->getProjectTitle());

    connect(editor->map, SIGNAL(mapChanged(Map*)), this, SLOT(onMapChanged(Map *)));
    connect(editor->map, SIGNAL(mapNeedsRedrawing()), this, SLOT(onMapNeedsRedrawing()));

    setRecentMap(map_name);
    updateMapList();
    updateTilesetEditor();
    return true;
}

void MainWindow::redrawMapScene()
{
    if (!editor->displayMap())
        return;

    on_mainTabBar_tabBarClicked(ui->mainTabBar->currentIndex());

    double base = editor->scale_base;
    double exp  = editor->scale_exp;

    int width = static_cast<int>(ceil((editor->scene->width()) * pow(base,exp))) + 2;
    int height = static_cast<int>(ceil((editor->scene->height()) * pow(base,exp))) + 2;

    ui->graphicsView_Map->setScene(editor->scene);
    ui->graphicsView_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Map->setFixedSize(width, height);
    ui->graphicsView_Map->editor = editor;

    ui->graphicsView_Connections->setScene(editor->scene);
    ui->graphicsView_Connections->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Connections->setFixedSize(width, height);

    ui->graphicsView_Metatiles->setScene(editor->scene_metatiles);
    //ui->graphicsView_Metatiles->setSceneRect(editor->scene_metatiles->sceneRect());
    ui->graphicsView_Metatiles->setFixedSize(editor->metatile_selector_item->pixmap().width() + 2, editor->metatile_selector_item->pixmap().height() + 2);

    ui->graphicsView_BorderMetatile->setScene(editor->scene_selected_border_metatiles);
    ui->graphicsView_BorderMetatile->setFixedSize(editor->selected_border_metatiles_item->pixmap().width() + 2, editor->selected_border_metatiles_item->pixmap().height() + 2);

    ui->graphicsView_currentMetatileSelection->setScene(editor->scene_current_metatile_selection);
    ui->graphicsView_currentMetatileSelection->setFixedSize(editor->scene_current_metatile_selection_item->pixmap().width() + 2, editor->scene_current_metatile_selection_item->pixmap().height() + 2);

    ui->graphicsView_Collision->setScene(editor->scene_collision_metatiles);
    //ui->graphicsView_Collision->setSceneRect(editor->scene_collision_metatiles->sceneRect());
    ui->graphicsView_Collision->setFixedSize(editor->movement_permissions_selector_item->pixmap().width() + 2, editor->movement_permissions_selector_item->pixmap().height() + 2);

    on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
}

void MainWindow::openWarpMap(QString map_name, QString warp_num) {
    // Ensure valid destination map name.
    if (!editor->project->mapNames->contains(map_name)) {
        logError(QString("Invalid warp destination map name '%1'").arg(map_name));
        return;
    }

    // Ensure valid destination warp number.
    bool ok;
    int warpNum = warp_num.toInt(&ok, 0);
    if (!ok) {
        logError(QString("Invalid warp number '%1' for destination map '%2'").arg(warp_num).arg(map_name));
        return;
    }

    // Open the destination map, and select the target warp event.
    if (!setMap(map_name, true)) {
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening map %1. Please see %2 for full error details.\n\n%3")
                .arg(map_name)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Map", errorMsg);
        return;
    }

    QList<Event*> warp_events = editor->map->events["warp_event_group"];
    if (warp_events.length() > warpNum) {
        Event *warp_event = warp_events.at(warpNum);
        QList<DraggablePixmapItem *> *all_events = editor->getObjects();
        for (DraggablePixmapItem *item : *all_events) {
            if (item->event == warp_event) {
                editor->selected_events->clear();
                editor->selected_events->append(item);
                editor->updateSelectedEvents();
            }
        }

        delete all_events;
    }
}

void MainWindow::setRecentMap(QString mapName) {
    porymapConfig.setRecentMap(mapName);
}

void MainWindow::displayMapProperties() {
    ui->checkBox_Visibility->setChecked(false);
    ui->checkBox_ShowLocation->setChecked(false);
    ui->checkBox_AllowRunning->setChecked(false);
    ui->checkBox_AllowBiking->setChecked(false);
    ui->checkBox_AllowEscapeRope->setChecked(false);
    if (!editor || !editor->map || !editor->project) {
        ui->frame_3->setEnabled(false);
        return;
    }
    ui->frame_3->setEnabled(true);
    Map *map = editor->map;

    ui->comboBox_Song->setCurrentText(map->song);
    ui->comboBox_Location->setCurrentText(map->location);
    ui->comboBox_PrimaryTileset->setCurrentText(map->layout->tileset_primary_label);
    ui->comboBox_SecondaryTileset->setCurrentText(map->layout->tileset_secondary_label);
    ui->checkBox_Visibility->setChecked(map->requiresFlash.toInt() > 0 || map->requiresFlash == "TRUE");
    ui->comboBox_Weather->setCurrentText(map->weather);
    ui->comboBox_Type->setCurrentText(map->type);
    ui->comboBox_BattleScene->setCurrentText(map->battle_scene);
    ui->checkBox_ShowLocation->setChecked(map->show_location.toInt() > 0 || map->show_location == "TRUE");
    ui->checkBox_AllowRunning->setChecked(map->allowRunning.toInt() > 0 || map->allowRunning == "TRUE");
    ui->checkBox_AllowBiking->setChecked(map->allowBiking.toInt() > 0 || map->allowBiking == "TRUE");
    ui->checkBox_AllowEscapeRope->setChecked(map->allowEscapeRope.toInt() > 0 || map->allowEscapeRope == "TRUE");
    ui->spinBox_FloorNumber->setValue(map->floorNumber);

    // Custom fields table.
    ui->tableWidget_CustomHeaderFields->blockSignals(true);
    ui->tableWidget_CustomHeaderFields->setRowCount(0);
    for (auto it = map->customHeaders.begin(); it != map->customHeaders.end(); it++) {
        int rowIndex = ui->tableWidget_CustomHeaderFields->rowCount();
        ui->tableWidget_CustomHeaderFields->insertRow(rowIndex);
        ui->tableWidget_CustomHeaderFields->setItem(rowIndex, 0, new QTableWidgetItem(it.key()));
        ui->tableWidget_CustomHeaderFields->setItem(rowIndex, 1, new QTableWidgetItem(it.value()));
    }
    ui->tableWidget_CustomHeaderFields->blockSignals(false);
}

void MainWindow::on_comboBox_Song_currentTextChanged(const QString &song)
{
    if (editor && editor->map) {
        editor->map->song = song;
    }
}

void MainWindow::on_comboBox_Location_currentTextChanged(const QString &location)
{
    if (editor && editor->map) {
        editor->map->location = location;
    }
}

void MainWindow::on_comboBox_Weather_currentTextChanged(const QString &weather)
{
    if (editor && editor->map) {
        editor->map->weather = weather;
    }
}

void MainWindow::on_comboBox_Type_currentTextChanged(const QString &type)
{
    if (editor && editor->map) {
        editor->map->type = type;
    }
}

void MainWindow::on_comboBox_BattleScene_currentTextChanged(const QString &battle_scene)
{
    if (editor && editor->map) {
        editor->map->battle_scene = battle_scene;
    }
}

void MainWindow::on_checkBox_Visibility_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->requiresFlash = "TRUE";
        } else {
            editor->map->requiresFlash = "FALSE";
        }
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

void MainWindow::on_checkBox_AllowRunning_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->allowRunning = "1";
        } else {
            editor->map->allowRunning = "0";
        }
    }
}

void MainWindow::on_checkBox_AllowBiking_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->allowBiking = "1";
        } else {
            editor->map->allowBiking = "0";
        }
    }
}

void MainWindow::on_checkBox_AllowEscapeRope_clicked(bool checked)
{
    if (editor && editor->map) {
        if (checked) {
            editor->map->allowEscapeRope = "1";
        } else {
            editor->map->allowEscapeRope = "0";
        }
    }
}

void MainWindow::on_spinBox_FloorNumber_valueChanged(int offset)
{
    if (editor && editor->map) {
        editor->map->floorNumber = offset;
    }
}

bool MainWindow::loadDataStructures() {
    Project *project = editor->project;
    bool success = project->readMapLayouts()
                && project->readRegionMapSections()
                && project->readItemNames()
                && project->readFlagNames()
                && project->readVarNames()
                && project->readMovementTypes()
                && project->readInitialFacingDirections()
                && project->readMapTypes()
                && project->readMapBattleScenes()
                && project->readWeatherNames()
                && project->readBgEventFacingDirections()
                && project->readTrainerTypes()
                && project->readMetatileBehaviors()
                && project->readTilesetProperties()
                && project->readHealLocations()
                && project->readMiscellaneousConstants()
                && project->readSpeciesIconPaths()
                && project->readWildMonData();
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokeemerald || projectConfig.getBaseGameVersion() == BaseGameVersion::pokeruby)
        success = success 
               && project->readSecretBaseIds() 
               && project->readCoordEventWeatherNames();
    
    return success && loadProjectCombos();
}

bool MainWindow::loadProjectCombos() {
    // set up project ui comboboxes
    Project *project = editor->project;

    // Block signals to the comboboxes while they are being modified
    const QSignalBlocker blocker1(ui->comboBox_Song);
    const QSignalBlocker blocker2(ui->comboBox_Location);
    const QSignalBlocker blocker3(ui->comboBox_PrimaryTileset);
    const QSignalBlocker blocker4(ui->comboBox_SecondaryTileset);
    const QSignalBlocker blocker5(ui->comboBox_Weather);
    const QSignalBlocker blocker6(ui->comboBox_BattleScene);
    const QSignalBlocker blocker7(ui->comboBox_Type);

    ui->comboBox_Song->clear();
    ui->comboBox_Song->addItems(project->getSongNames());
    ui->comboBox_Location->clear();
    ui->comboBox_Location->addItems(project->mapSectionValueToName.values());

    QMap<QString, QStringList> tilesets = project->getTilesetLabels();
    if (tilesets.isEmpty()) {
        return false;
    }

    ui->comboBox_PrimaryTileset->clear();
    ui->comboBox_PrimaryTileset->addItems(tilesets.value("primary"));
    ui->comboBox_SecondaryTileset->clear();
    ui->comboBox_SecondaryTileset->addItems(tilesets.value("secondary"));
    ui->comboBox_Weather->clear();
    ui->comboBox_Weather->addItems(*project->weatherNames);
    ui->comboBox_BattleScene->clear();
    ui->comboBox_BattleScene->addItems(*project->mapBattleScenes);
    ui->comboBox_Type->clear();
    ui->comboBox_Type->addItems(*project->mapTypes);

    return true;
}

bool MainWindow::populateMapList() {
    bool success = editor->project->readMapGroups();
    if (success) {
        sortMapList();
    }
    return success;
}

void MainWindow::sortMapList() {
    Project *project = editor->project;

    QIcon mapFolderIcon;
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);

    QIcon folderIcon;
    folderIcon.addFile(QStringLiteral(":/icons/folder_closed.ico"), QSize(), QIcon::Normal, QIcon::Off);
    //folderIcon.addFile(QStringLiteral(":/icons/folder.ico"), QSize(), QIcon::Normal, QIcon::On);

    ui->mapList->setUpdatesEnabled(false);
    mapListModel->clear();
    mapGroupItemsList->clear();
    QStandardItem *root = mapListModel->invisibleRootItem();

    switch (mapSortOrder)
    {
        case MapSortOrder::Group:
            for (int i = 0; i < project->groupNames->length(); i++) {
                QString group_name = project->groupNames->value(i);
                QStandardItem *group = new QStandardItem;
                group->setText(group_name);
                group->setIcon(mapFolderIcon);
                group->setEditable(false);
                group->setData(group_name, Qt::UserRole);
                group->setData("map_group", MapListUserRoles::TypeRole);
                group->setData(i, MapListUserRoles::GroupRole);
                root->appendRow(group);
                mapGroupItemsList->append(group);
                QStringList names = project->groupedMapNames.value(i);
                for (int j = 0; j < names.length(); j++) {
                    QString map_name = names.value(j);
                    QStandardItem *map = createMapItem(map_name, i, j);
                    group->appendRow(map);
                    mapListIndexes.insert(map_name, map->index());
                }
            }
            break;
        case MapSortOrder::Area:
        {
            QMap<QString, int> mapsecToGroupNum;
            for (int i = 0; i < project->mapSectionNameToValue.size(); i++) {
                QString mapsec_name = project->mapSectionValueToName.value(i);
                QStandardItem *mapsec = new QStandardItem;
                mapsec->setText(mapsec_name);
                mapsec->setIcon(folderIcon);
                mapsec->setEditable(false);
                mapsec->setData(mapsec_name, Qt::UserRole);
                mapsec->setData("map_sec", MapListUserRoles::TypeRole);
                mapsec->setData(i, MapListUserRoles::GroupRole);
                root->appendRow(mapsec);
                mapGroupItemsList->append(mapsec);
                mapsecToGroupNum.insert(mapsec_name, i);
            }
            for (int i = 0; i < project->groupNames->length(); i++) {
                QStringList names = project->groupedMapNames.value(i);
                for (int j = 0; j < names.length(); j++) {
                    QString map_name = names.value(j);
                    QStandardItem *map = createMapItem(map_name, i, j);
                    QString location = project->readMapLocation(map_name);
                    QStandardItem *mapsecItem = mapGroupItemsList->at(mapsecToGroupNum[location]);
                    mapsecItem->setIcon(mapFolderIcon);
                    mapsecItem->appendRow(map);
                    mapListIndexes.insert(map_name, map->index());
                }
            }
            break;
        }
        case MapSortOrder::Layout:
        {
            QMap<QString, int> layoutIndices;
            for (int i = 0; i < project->mapLayoutsTable.length(); i++) {
                QString layoutId = project->mapLayoutsTable.value(i);
                MapLayout *layout = project->mapLayouts.value(layoutId);
                QStandardItem *layoutItem = new QStandardItem;
                layoutItem->setText(layout->name);
                layoutItem->setIcon(folderIcon);
                layoutItem->setEditable(false);
                layoutItem->setData(layout->name, Qt::UserRole);
                layoutItem->setData("map_layout", MapListUserRoles::TypeRole);
                layoutItem->setData(layout->id, MapListUserRoles::TypeRole2);
                layoutItem->setData(i, MapListUserRoles::GroupRole);
                root->appendRow(layoutItem);
                mapGroupItemsList->append(layoutItem);
                layoutIndices[layoutId] = i;
            }
            for (int i = 0; i < project->groupNames->length(); i++) {
                QStringList names = project->groupedMapNames.value(i);
                for (int j = 0; j < names.length(); j++) {
                    QString map_name = names.value(j);
                    QStandardItem *map = createMapItem(map_name, i, j);
                    QString layoutId = project->readMapLayoutId(map_name);
                    QStandardItem *layoutItem = mapGroupItemsList->at(layoutIndices.value(layoutId));
                    layoutItem->setIcon(mapFolderIcon);
                    layoutItem->appendRow(map);
                    mapListIndexes.insert(map_name, map->index());
                }
            }
            break;
        }
    }

    ui->mapList->setUpdatesEnabled(true);
    ui->mapList->repaint();
    updateMapList();
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
    QModelIndex index = mapListProxyModel->mapToSource(ui->mapList->indexAt(point));
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
        QMenu* menu = new QMenu(this);
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map to Group"))->setData(groupNum);
        connect(actions, SIGNAL(triggered(QAction*)), this, SLOT(onAddNewMapToGroupClick(QAction*)));
        menu->exec(QCursor::pos());
    } else if (itemType == "map_sec") {
        QString secName = selectedItem->data(Qt::UserRole).toString();
        QMenu* menu = new QMenu(this);
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map to Area"))->setData(secName);
        connect(actions, SIGNAL(triggered(QAction*)), this, SLOT(onAddNewMapToAreaClick(QAction*)));
        menu->exec(QCursor::pos());
    } else if (itemType == "map_layout") {
        QString layoutId = selectedItem->data(MapListUserRoles::TypeRole2).toString();
        QMenu* menu = new QMenu(this);
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map with Layout"))->setData(layoutId);
        connect(actions, SIGNAL(triggered(QAction*)), this, SLOT(onAddNewMapToLayoutClick(QAction*)));
        menu->exec(QCursor::pos());
    }
}

void MainWindow::onAddNewMapToGroupClick(QAction* triggeredAction)
{
    int groupNum = triggeredAction->data().toInt();
    openNewMapPopupWindow(MapSortOrder::Group, groupNum);
}

void MainWindow::onAddNewMapToAreaClick(QAction* triggeredAction)
{
    QString secName = triggeredAction->data().toString();
    openNewMapPopupWindow(MapSortOrder::Area, secName);
}

void MainWindow::onAddNewMapToLayoutClick(QAction* triggeredAction)
{
    QString layoutId = triggeredAction->data().toString();
    openNewMapPopupWindow(MapSortOrder::Layout, layoutId);
}

void MainWindow::onNewMapCreated() {
    QString newMapName = this->newmapprompt->map->name;
    int newMapGroup = this->newmapprompt->group;
    Map *newMap_ = this->newmapprompt->map;
    bool existingLayout = this->newmapprompt->existingLayout;

    Map *newMap = editor->project->addNewMapToGroup(newMapName, newMapGroup, newMap_, existingLayout);

    logInfo(QString("Created a new map named %1.").arg(newMapName));

    editor->project->saveMap(newMap);
    editor->project->saveAllDataStructures();

    QStandardItem* groupItem = mapGroupItemsList->at(newMapGroup);
    int numMapsInGroup = groupItem->rowCount();

    QStandardItem *newMapItem = createMapItem(newMapName, newMapGroup, numMapsInGroup);
    groupItem->appendRow(newMapItem);
    mapListIndexes.insert(newMapName, newMapItem->index());

    sortMapList();
    setMap(newMapName, true);

    if (newMap->isFlyable == "TRUE") {
        addNewEvent("event_heal_location");
        editor->project->saveHealLocationStruct(newMap);
        editor->save();// required
    }

    disconnect(this->newmapprompt, SIGNAL(applied()), this, SLOT(onNewMapCreated()));
}

void MainWindow::openNewMapPopupWindow(int type, QVariant data) {
    if (!this->newmapprompt) {
        this->newmapprompt = new NewMapPopup(this, this->editor->project);
    }
    if (!this->newmapprompt->isVisible()) {
        this->newmapprompt->show();
    } else {
        this->newmapprompt->raise();
        this->newmapprompt->activateWindow();
    }
    switch (type)
    {
        case MapSortOrder::Group:
            this->newmapprompt->init(type, data.toInt(), QString(), QString());
            break;
        case MapSortOrder::Area:
            this->newmapprompt->init(type, 0, data.toString(), QString());
            break;
        case MapSortOrder::Layout:
            this->newmapprompt->init(type, 0, QString(), data.toString());
            break;
    }
    connect(this->newmapprompt, SIGNAL(applied()), this, SLOT(onNewMapCreated()));
    connect(this->newmapprompt, &QObject::destroyed, [=](QObject *) { this->newmapprompt = nullptr; });
            this->newmapprompt->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::on_action_NewMap_triggered() {
    openNewMapPopupWindow(MapSortOrder::Group, 0);
}

void MainWindow::on_actionNew_Tileset_triggered() {
    NewTilesetDialog *createTilesetDialog = new NewTilesetDialog(editor->project, this);
    if(createTilesetDialog->exec() == QDialog::Accepted){
        if(createTilesetDialog->friendlyName.isEmpty()) {
            logError(QString("Tried to create a directory with an empty name."));
            QMessageBox msgBox(this);
            msgBox.setText("Failed to add new tileset.");
            QString message = QString("The given name was empty.");
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        }
        QString fullDirectoryPath = editor->project->root + createTilesetDialog->path;
        QDir directory;
        if(directory.exists(fullDirectoryPath)) {
            logError(QString("Could not create tileset \"%1\", the folder \"%2\" already exists.").arg(createTilesetDialog->friendlyName, fullDirectoryPath));
            QMessageBox msgBox(this);
            msgBox.setText("Failed to add new tileset.");
            QString message = QString("The folder for tileset \"%1\" already exists. View porymap.log for specific errors.").arg(createTilesetDialog->friendlyName);
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        }
        QMap<QString, QStringList> tilesets = this->editor->project->getTilesetLabels();
        if(tilesets.value("primary").contains(createTilesetDialog->fullSymbolName) || tilesets.value("secondary").contains(createTilesetDialog->fullSymbolName)) {
            logError(QString("Could not create tileset \"%1\", the symbol \"%2\" already exists.").arg(createTilesetDialog->friendlyName, createTilesetDialog->fullSymbolName));
            QMessageBox msgBox(this);
            msgBox.setText("Failed to add new tileset.");
            QString message = QString("The symbol for tileset \"%1\" (\"%2\") already exists.").arg(createTilesetDialog->friendlyName, createTilesetDialog->fullSymbolName);
            msgBox.setInformativeText(message);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Critical);
            msgBox.exec();
            return;
        }
        directory.mkdir(fullDirectoryPath);
        directory.mkdir(fullDirectoryPath + "/palettes");
        Tileset *newSet = new Tileset();
        newSet->name = createTilesetDialog->fullSymbolName;
        newSet->tilesImagePath = fullDirectoryPath + "/tiles.png";
        newSet->metatiles_path = fullDirectoryPath + "/metatiles.bin";
        newSet->metatile_attrs_path = fullDirectoryPath + "/metatile_attributes.bin";
        newSet->is_secondary = createTilesetDialog->isSecondary ? "TRUE" : "FALSE";
        int numMetaTiles = createTilesetDialog->isSecondary ? (Project::getNumTilesTotal() - Project::getNumTilesPrimary()) : Project::getNumTilesPrimary();
        QImage *tilesImage = new QImage(":/images/blank_tileset.png");
        editor->project->loadTilesetTiles(newSet, *tilesImage);
        newSet->metatiles = new QList<Metatile*>();
        for(int i = 0; i < numMetaTiles; ++i) {
            Metatile *mt = new Metatile();
            for(int j = 0; j < 8; ++j){
                Tile *tile = new Tile();
                //Create a checkerboard-style dummy tileset
                if(((i / 8) % 2) == 0)
                    tile->tile = ((i % 2) == 0) ? 1 : 2;
                else
                    tile->tile = ((i % 2) == 1) ? 1 : 2;
                tile->xflip = false;
                tile->yflip = false;
                tile->palette = 0;
                mt->tiles->append(*tile);
            }
            mt->behavior = 0;
            mt->layerType = 0;
            mt->encounterType = 0;
            mt->terrainType = 0;

            newSet->metatiles->append(mt);
        }
        newSet->palettes = new QList<QList<QRgb>>();
        newSet->palettePaths = *new QList<QString>();
        for(int i = 0; i < 16; ++i) {
            QList<QRgb> *currentPal = new QList<QRgb>();
            for(int i = 0; i < 16;++i) {
                currentPal->append(qRgb(0,0,0));
            }
            newSet->palettes->append(*currentPal);
            QString fileName;
            fileName.sprintf("%02d.pal", i);
            newSet->palettePaths.append(fullDirectoryPath+"/palettes/" + fileName);
        }
        (*newSet->palettes)[0][1] = qRgb(255,0,255);
        newSet->is_compressed = "TRUE";
        newSet->padding = "0";
        editor->project->saveTilesetTilesImage(newSet);
        editor->project->saveTilesetMetatiles(newSet);
        editor->project->saveTilesetMetatileAttributes(newSet);
        editor->project->saveTilesetPalettes(newSet, !createTilesetDialog->isSecondary);

        //append to tileset specific files

        newSet->appendToHeaders(editor->project->root + "/data/tilesets/headers.inc", createTilesetDialog->friendlyName);
        newSet->appendToGraphics(editor->project->root + "/data/tilesets/graphics.inc", createTilesetDialog->friendlyName, !createTilesetDialog->isSecondary);
        newSet->appendToMetatiles(editor->project->root + "/data/tilesets/metatiles.inc", createTilesetDialog->friendlyName, !createTilesetDialog->isSecondary);
        if(!createTilesetDialog->isSecondary) {
            this->ui->comboBox_PrimaryTileset->addItem(createTilesetDialog->fullSymbolName);
        } else {
            this->ui->comboBox_SecondaryTileset->addItem(createTilesetDialog->fullSymbolName);
        }
        QMessageBox msgBox(this);
        msgBox.setText("Successfully created tileset.");
        QString message = QString("Tileset \"%1\" was created successfully.").arg(createTilesetDialog->friendlyName);
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        msgBox.exec();
    }
}

void MainWindow::onTilesetChanged(QString mapName)
{
    setMap(mapName);
}

void MainWindow::updateTilesetEditor() {
    if (this->tilesetEditor) {
        this->tilesetEditor->setTilesets(editor->ui->comboBox_PrimaryTileset->currentText(), editor->ui->comboBox_SecondaryTileset->currentText());
    }
}

void MainWindow::currentMetatilesSelectionChanged()
{
    double scale = pow(3.0, static_cast<double>(porymapConfig.getMetatilesZoom() - 30) / 30.0);
    ui->graphicsView_currentMetatileSelection->setFixedSize(editor->scene_current_metatile_selection_item->pixmap().width() * scale + 2, editor->scene_current_metatile_selection_item->pixmap().height() * scale + 2);
    ui->graphicsView_currentMetatileSelection->setSceneRect(0, 0, editor->scene_current_metatile_selection_item->pixmap().width() * scale, editor->scene_current_metatile_selection_item->pixmap().height() * scale);

    QPoint size = editor->metatile_selector_item->getSelectionDimensions();
    if (size.x() == 1 && size.y() == 1) {
        QPoint pos = editor->metatile_selector_item->getMetatileIdCoordsOnWidget(editor->metatile_selector_item->getSelectedMetatiles()->at(0));
        pos *= scale;
        ui->scrollArea_2->ensureVisible(pos.x(), pos.y(), 8 * scale, 8 * scale);
    }
}

void MainWindow::on_mapList_activated(const QModelIndex &index)
{
    QVariant data = index.data(Qt::UserRole);
    if (index.data(MapListUserRoles::TypeRole) == "map_name" && !data.isNull()) {
        QString mapName = data.toString();
        if (!setMap(mapName)) {
            QMessageBox msgBox(this);
            QString errorMsg = QString("There was an error opening map %1. Please see %2 for full error details.\n\n%3")
                    .arg(mapName)
                    .arg(getLogPath())
                    .arg(getMostRecentError());
            msgBox.critical(nullptr, "Error Opening Map", errorMsg);
        }
    }
}

void MainWindow::drawMapListIcons(QAbstractItemModel *model) {
    projectHasUnsavedChanges = false;
    QList<QModelIndex> list;
    list.append(QModelIndex());
    while (list.length()) {
        QModelIndex parent = list.takeFirst();
        for (int i = 0; i < model->rowCount(parent); i++) {
            QModelIndex index = model->index(i, 0, parent);
            if (model->hasChildren(index)) {
                list.append(index);
            }
            QVariant data = index.data(Qt::UserRole);
            if (!data.isNull()) {
                QString map_name = data.toString();
                if (editor->project && editor->project->mapCache->contains(map_name)) {
                    QStandardItem *map = mapListModel->itemFromIndex(mapListIndexes.value(map_name));
                    map->setIcon(*mapIcon);
                    if (editor->project->mapCache->value(map_name)->hasUnsavedChanges()) {
                        map->setIcon(*mapEditedIcon);
                        projectHasUnsavedChanges = true;
                    }
                    if (editor->map->name == map_name) {
                        map->setIcon(*mapOpenedIcon);
                    }
                }
            }
        }
    }
}

void MainWindow::updateMapList() {
    drawMapListIcons(mapListModel);
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

// Open current map scripts in system default editor for .inc files
void MainWindow::openInTextEditor() {
    QString path = QDir::cleanPath("file://" + editor->project->root + QDir::separator() + "data/maps/" + editor->map->name + "/scripts" + editor->project->getScriptFileExtension(projectConfig.getUsePoryScript()));
    QDesktopServices::openUrl(QUrl(path));
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
    editor->playerViewRect->setVisible(false);
    editor->cursorMapTileRect->setVisible(false);
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_mainTabBar_tabBarClicked(int index)
{
    ui->mainTabBar->setCurrentIndex(index);

    int tabIndexToStackIndex[5] = {0, 0, 1, 2, 3};
    ui->mainStackedWidget->setCurrentIndex(tabIndexToStackIndex[index]);

    if (index == 0) {
        ui->stackedWidget_MapEvents->setCurrentIndex(0);
        on_tabWidget_2_currentChanged(ui->tabWidget_2->currentIndex());
    } else if (index == 1) {
        ui->stackedWidget_MapEvents->setCurrentIndex(1);
        editor->setEditingObjects();
        QStringList validOptions = {"select", "move", "paint", "shift"};
        QString newEditMode = validOptions.contains(editor->map_edit_mode) ? editor->map_edit_mode : "select";
        clickToolButtonFromEditMode(newEditMode);
    } else if (index == 3) {
        editor->setEditingConnections();
    }
    if (index != 4) {
        if (projectConfig.getEncounterJsonActive())
            editor->saveEncounterTabData();
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

void MainWindow::on_actionZoom_In_triggered() {
    scaleMapView(1);
}

void MainWindow::on_actionZoom_Out_triggered() {
    scaleMapView(-1);
}

void MainWindow::on_actionBetter_Cursors_triggered() {
    porymapConfig.setPrettyCursors(ui->actionBetter_Cursors->isChecked());
    this->editor->settings->betterCursors = ui->actionBetter_Cursors->isChecked();
}

void MainWindow::on_actionPlayer_View_Rectangle_triggered()
{
    bool enabled = ui->actionPlayer_View_Rectangle->isChecked();
    porymapConfig.setShowPlayerView(enabled);
    this->editor->settings->playerViewRectEnabled = enabled;
}

void MainWindow::on_actionCursor_Tile_Outline_triggered()
{
    bool enabled = ui->actionCursor_Tile_Outline->isChecked();
    porymapConfig.setShowCursorTile(enabled);
    this->editor->settings->cursorTileRectEnabled = enabled;
}

void MainWindow::on_actionUse_Encounter_Json_triggered(bool checked)
{
    QMessageBox warning(this);
    warning.setText("You must reload the project for this setting to take effect.");
    warning.setIcon(QMessageBox::Information);
    warning.exec();
    projectConfig.setEncounterJsonActive(checked);
}

void MainWindow::on_actionMonitor_Project_Files_triggered(bool checked)
{
    porymapConfig.setMonitorFiles(checked);
}

void MainWindow::on_actionUse_Poryscript_triggered(bool checked)
{
    projectConfig.setUsePoryScript(checked);
}

void MainWindow::on_actionPencil_triggered()
{
    on_toolButton_Paint_clicked();
}

void MainWindow::on_actionPointer_triggered()
{
    on_toolButton_Select_clicked();
}

void MainWindow::on_actionFlood_Fill_triggered()
{
    on_toolButton_Fill_clicked();
}

void MainWindow::on_actionEyedropper_triggered()
{
    on_toolButton_Dropper_clicked();
}

void MainWindow::on_actionMove_triggered()
{
    on_toolButton_Move_clicked();
}

void MainWindow::on_actionMap_Shift_triggered()
{
    on_toolButton_Shift_clicked();
}

void MainWindow::scaleMapView(int s) {
    if ((editor->scale_exp + s) <= 5 && (editor->scale_exp + s) >= -2)    // sane limits
    {
        if (s == 0)
        {
            s = -editor->scale_exp;
        }

        editor->scale_exp += s;

        double base = editor->scale_base;
        double exp  = editor->scale_exp;
        double sfactor = pow(base,s);

        ui->graphicsView_Map->scale(sfactor,sfactor);
        ui->graphicsView_Connections->scale(sfactor,sfactor);

        int width = static_cast<int>(ceil((editor->scene->width()) * pow(base,exp))) + 2;
        int height = static_cast<int>(ceil((editor->scene->height()) * pow(base,exp))) + 2;
        ui->graphicsView_Map->setFixedSize(width, height);
        ui->graphicsView_Connections->setFixedSize(width, height);
    }
}

void MainWindow::resetMapViewScale() {
    scaleMapView(0);
}

void MainWindow::addNewEvent(QString event_type)
{
    if (editor) {
        DraggablePixmapItem *object = editor->addNewEvent(event_type);
        updateObjects();
        if (object) {
            editor->selectMapEvent(object, false);
        }
    }
}

void MainWindow::updateObjects() {
    selectedObject = nullptr;
    selectedWarp = nullptr;
    selectedTrigger = nullptr;
    selectedBG = nullptr;
    selectedHealspot = nullptr;
    ui->tabWidget_EventType->clear();

    bool hasObjects = false;
    bool hasWarps = false;
    bool hasTriggers = false;
    bool hasBGs = false;
    bool hasHealspots = false;

    for (DraggablePixmapItem *item : *editor->getObjects())
    {
        QString event_type = item->event->get("event_type");

        if (event_type == EventType::Object) {
            hasObjects = true;
        }
        else if (event_type == EventType::Warp) {
            hasWarps = true;
        }
        else if (event_type == EventType::Trigger || event_type == EventType::WeatherTrigger) {
            hasTriggers = true;
        }
        else if (event_type == EventType::Sign || event_type == EventType::HiddenItem || event_type == EventType::SecretBase) {
            hasBGs = true;
        }
        else if (event_type == EventType::HealLocation) {
            hasHealspots = true;
        }
    }

    if (hasObjects)
    {
        ui->tabWidget_EventType->addTab(eventTabObjectWidget, "Objects");
    }

    if (hasWarps)
    {
        ui->tabWidget_EventType->addTab(eventTabWarpWidget, "Warps");
    }

    if (hasTriggers)
    {
        ui->tabWidget_EventType->addTab(eventTabTriggerWidget, "Triggers");
    }

    if (hasBGs)
    {
        ui->tabWidget_EventType->addTab(eventTabBGWidget, "BGs");
    }

    if (hasHealspots)
    {
        ui->tabWidget_EventType->addTab(eventTabHealspotWidget, "Healspots");
    }

    updateSelectedObjects();
}

// Should probably just pass layout and let the editor work it out
void MainWindow::updateSelectedObjects() {
    QList<DraggablePixmapItem *> *all_events = editor->getObjects();
    QList<DraggablePixmapItem *> *events = nullptr;

    if (editor->selected_events && editor->selected_events->length()) {
        events = editor->selected_events;
    } else {
        events = new QList<DraggablePixmapItem*>;
        if (all_events && all_events->length()) {
            DraggablePixmapItem *selectedEvent = all_events->first();
            editor->selected_events->append(selectedEvent);
            editor->redrawObject(selectedEvent);
            events->append(selectedEvent);
        }
    }

    QMap<QString, int> event_obj_gfx_constants = editor->project->getEventObjGfxConstants();

    QList<EventPropertiesFrame *> frames;

    bool pokefirered = projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered;
    for (DraggablePixmapItem *item : *events) {
        EventPropertiesFrame *frame = new EventPropertiesFrame(item->event);
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

        QString event_type = item->event->get("event_type");
        QString event_group_type = item->event->get("event_group_type");
        QString map_name = item->event->get("map_name");
        int event_offs;
        if (event_type == "event_warp") { event_offs = 0; }
        else { event_offs = 1; }
        frame->ui->label_name->setText(QString("%1 Id").arg(event_type));

        if (events->count() == 1)
        {
            frame->ui->spinBox_index->setValue(editor->project->getMap(map_name)->events.value(event_group_type).indexOf(item->event) + event_offs);
            frame->ui->spinBox_index->setMinimum(event_offs);
            frame->ui->spinBox_index->setMaximum(editor->project->getMap(map_name)->events.value(event_group_type).length() + event_offs - 1);
            connect(frame->ui->spinBox_index, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::selectedEventIndexChanged);
        }
        else
        {
            frame->ui->spinBox_index->setVisible(false);
        }

        frame->ui->label_spritePixmap->setPixmap(item->event->pixmap);
        connect(item, SIGNAL(spriteChanged(QPixmap)), frame->ui->label_spritePixmap, SLOT(setPixmap(QPixmap)));

        frame->ui->sprite->setVisible(false);

        QMap<QString, QString> field_labels;
        field_labels["script_label"] = "Script";
        field_labels["event_flag"] = "Event Flag";
        field_labels["movement_type"] = "Movement";
        field_labels["radius_x"] = "Movement Radius X";
        field_labels["radius_y"] = "Movement Radius Y";
        field_labels["trainer_type"] = "Trainer Type";
        field_labels["sight_radius_tree_id"] = "Sight Radius / Berry Tree ID";
        field_labels["in_connection"] = "In Connection";
        field_labels["destination_warp"] = "Destination Warp";
        field_labels["destination_map_name"] = "Destination Map";
        field_labels["script_var"] = "Var";
        field_labels["script_var_value"] = "Var Value";
        field_labels["player_facing_direction"] = "Player Facing Direction";
        field_labels["item"] = "Item";
        field_labels["quantity"] = "Quantity";
        field_labels["underfoot"] = "Requires Itemfinder";
        field_labels["weather"] = "Weather";
        field_labels["flag"] = "Flag";
        field_labels["secret_base_id"] = "Secret Base Id";
        field_labels["respawn_map"] = "Respawn Map";
        field_labels["respawn_npc"] = "Respawn NPC";

        QStringList fields;

        if (event_type == EventType::Object) {

            frame->ui->sprite->setVisible(true);
            frame->ui->comboBox_sprite->addItems(event_obj_gfx_constants.keys());
            frame->ui->comboBox_sprite->setCurrentText(item->event->get("sprite"));
            connect(frame->ui->comboBox_sprite, &QComboBox::currentTextChanged, item, &DraggablePixmapItem::set_sprite);

            /*
            frame->ui->script->setVisible(true);
            frame->ui->comboBox_script->addItem(item->event->get("script_label"));
            frame->ui->comboBox_script->setCurrentText(item->event->get("script_label"));
            //item->bind(frame->ui->comboBox_script, "script_label");
            connect(frame->ui->comboBox_script, SIGNAL(activated(QString)), item, SLOT(set_script(QString)));
            //connect(frame->ui->comboBox_script, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), item, [item](QString script_label){ item->event->put("script_label", script_label); });
            //connect(item, SIGNAL(scriptChanged(QString)), frame->ui->comboBox_script, SLOT(setValue(QString)));
            */

            fields << "movement_type";
            fields << "radius_x";
            fields << "radius_y";
            fields << "script_label";
            fields << "event_flag";
            fields << "trainer_type";
            fields << "sight_radius_tree_id";
            if (pokefirered) {
                fields << "in_connection";
            }
        }
        else if (event_type == EventType::Warp) {
            fields << "destination_map_name";
            fields << "destination_warp";
        }
        else if (event_type == EventType::Trigger) {
            fields << "script_label";
            fields << "script_var";
            fields << "script_var_value";
        }
        else if (event_type == EventType::WeatherTrigger) {
            fields << "weather";
        }
        else if (event_type == EventType::Sign) {
            fields << "player_facing_direction";
            fields << "script_label";
        }
        else if (event_type == EventType::HiddenItem) {
            fields << "item";
            fields << "flag";
            if (pokefirered) {
                fields << "quantity";
                fields << "underfoot";
            }
        }
        else if (event_type == EventType::SecretBase) {
            fields << "secret_base_id";
        }
        else if (event_type == EventType::HealLocation) {
            // Hide elevation so users don't get impression that editing it is meaningful.
            frame->ui->spinBox_z->setVisible(false);
            frame->ui->label_z->setVisible(false);
            if (pokefirered) {
                fields << "respawn_map";
                fields << "respawn_npc";
            }
        }

        // Some keys shouldn't use a combobox
        QStringList spinKeys = {"quantity", "respawn_npc"};
        QStringList checkKeys = {"underfoot", "in_connection"};
        for (QString key : fields) {
            QString value = item->event->get(key);
            QWidget *widget = new QWidget(frame);
            QFormLayout *fl = new QFormLayout(widget);
            fl->setContentsMargins(9, 0, 9, 0);
            fl->setRowWrapPolicy(QFormLayout::WrapLongRows);

            NoScrollSpinBox *spin;
            NoScrollComboBox *combo;
            QCheckBox *check;

            if (spinKeys.contains(key)) {
                spin = new NoScrollSpinBox(widget);
            } else if (checkKeys.contains(key)) {
                check = new QCheckBox(widget);
            } else {
                combo = new NoScrollComboBox(widget);
                combo->setEditable(true);
            }

            if (key == "destination_map_name") {
                if (!editor->project->mapNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->mapNames);
                combo->setToolTip("The destination map name of the warp.");
            } else if (key == "destination_warp") {
                combo->setToolTip("The warp id on the destination map.");
            } else if (key == "item") {
                if (!editor->project->itemNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->itemNames);
            } else if (key == "quantity") {
                spin->setToolTip("The number of items received when the hidden item is picked up.");
                // Min 1 not needed. 0 is treated as a valid quantity and works as expected in-game.
                spin->setMaximum(127);
            } else if (key == "underfoot") {
                check->setToolTip("If checked, hidden item can only be picked up using the Itemfinder");
            } else if (key == "flag" || key == "event_flag") {
                if (!editor->project->flagNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->flagNames);
                if (key == "flag")
                    combo->setToolTip("The flag which is set when the hidden item is picked up.");
                else if (key == "event_flag")
                    combo->setToolTip("The flag which hides the object when set.");
            } else if (key == "script_var") {
                if (!editor->project->varNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->varNames);
                combo->setToolTip("The variable by which the script is triggered.\n"
                                  "The script is triggered when this variable's value matches 'Var Value'.");
            } else if (key == "script_var_value") {
                combo->setToolTip("The variable's value which triggers the script.");
            } else if (key == "movement_type") {
                if (!editor->project->movementTypes->contains(value)) {
                    combo->addItem(value);
                }
                connect(combo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
                        this, [this, item, frame](QString value){
                            item->event->setFrameFromMovement(editor->project->facingDirections.value(value));
                            item->updatePixmap();
                });
                combo->addItems(*editor->project->movementTypes);
                combo->setToolTip("The object's natural movement behavior when\n"
                                  "the player is not interacting with it.");
            } else if (key == "weather") {
                if (!editor->project->coordEventWeatherNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->coordEventWeatherNames);
                combo->setToolTip("The weather that starts when the player steps on this spot.");
            } else if (key == "secret_base_id") {
                if (!editor->project->secretBaseIds->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->secretBaseIds);
                combo->setToolTip("The secret base id which is inside this secret\n"
                                  "base entrance. Secret base ids are meant to be\n"
                                  "unique to each and every secret base entrance.");
            } else if (key == "player_facing_direction") {
                if (!editor->project->bgEventFacingDirections->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->bgEventFacingDirections);
                combo->setToolTip("The direction which the player must be facing\n"
                                  "to be able to interact with this event.");
            } else if (key == "radius_x") {
                combo->setToolTip("The maximum number of metatiles this object\n"
                                  "is allowed to move left or right during its\n"
                                  "normal movement behavior actions.");
                combo->setMinimumContentsLength(4);
            } else if (key == "radius_y") {
                combo->setToolTip("The maximum number of metatiles this object\n"
                                  "is allowed to move up or down during its\n"
                                  "normal movement behavior actions.");
                combo->setMinimumContentsLength(4);
            } else if (key == "script_label") {
                combo->setToolTip("The script which is executed with this event.");
            } else if (key == "trainer_type") {
                combo->addItems(*editor->project->trainerTypes);
                combo->setToolTip("The trainer type of this object event.\n"
                                  "If it is not a trainer, use NONE. SEE ALL DIRECTIONS\n"
                                  "should only be used with a sight radius of 1.");
            } else if (key == "sight_radius_tree_id") {
                combo->setToolTip("The maximum sight range of a trainer,\n"
                                  "OR the unique id of the berry tree.");
                combo->setMinimumContentsLength(4);
            } else if (key == "in_connection") {
                check->setToolTip("Check if object is positioned in the connection to another map.");
            } else if (key == "respawn_map") {
                if (!editor->project->mapNames->contains(value)) {
                    combo->addItem(value);
                }
                combo->addItems(*editor->project->mapNames);
                combo->setToolTip("The map where the player will respawn after whiteout.");
            } else if (key == "respawn_npc") {
                spin->setToolTip("event_object ID of the NPC the player interacts with\n" 
                                 "upon respawning after whiteout.");
                spin->setMinimum(1);
                spin->setMaximum(126);
            } else {
                combo->addItem(value);
            }

            // Keys using spin boxes
            if (spinKeys.contains(key)) {
                spin->setValue(value.toInt());

                fl->addRow(new QLabel(field_labels[key], widget), spin);
                widget->setLayout(fl);
                frame->layout()->addWidget(widget);

                connect(spin, QOverload<int>::of(&NoScrollSpinBox::valueChanged), [item, key](int value) {
                    item->event->put(key, value);
                });
            // Keys using check boxes
            } else if (checkKeys.contains(key)) {
                check->setChecked(value.toInt());

                fl->addRow(new QLabel(field_labels[key], widget), check);
                widget->setLayout(fl);
                frame->layout()->addWidget(widget);

                connect(check, &QCheckBox::stateChanged, [item, key](int state) {
                    switch (state)
                    {
                    case Qt::Checked:
                        item->event->put(key, true);
                        break;
                    case Qt::Unchecked:
                        item->event->put(key, false);
                        break;
                    }
                });
            // Keys using combo boxes
            } else {
                combo->setCurrentText(value);

                fl->addRow(new QLabel(field_labels[key], widget), combo);
                widget->setLayout(fl);
                frame->layout()->addWidget(widget);

                item->bind(combo, key);
            }
        }
        frames.append(frame);
    }

    //int scroll = ui->scrollArea_4->verticalScrollBar()->value();

    QScrollArea *scrollTarget = ui->scrollArea_Multiple;
    QWidget *target = ui->scrollAreaWidgetContents_Multiple;

    isProgrammaticEventTabChange = true;

    if (events->length() == 1)
    {
        QString event_group_type = (*events)[0]->event->get("event_group_type");

        if (event_group_type == "object_event_group") {
            scrollTarget = ui->scrollArea_Objects;
            target = ui->scrollAreaWidgetContents_Objects;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Objects);
        }
        else if (event_group_type == "warp_event_group") {
            scrollTarget = ui->scrollArea_Warps;
            target = ui->scrollAreaWidgetContents_Warps;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Warps);
        }
        else if (event_group_type == "coord_event_group") {
            scrollTarget = ui->scrollArea_Triggers;
            target = ui->scrollAreaWidgetContents_Triggers;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Triggers);
        }
        else if (event_group_type == "bg_event_group") {
            scrollTarget = ui->scrollArea_BGs;
            target = ui->scrollAreaWidgetContents_BGs;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_BGs);
        }
        else if (event_group_type == "heal_event_group") {
            scrollTarget = ui->scrollArea_Healspots;
            target = ui->scrollAreaWidgetContents_Healspots;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Healspots);
        }
        ui->tabWidget_EventType->removeTab(ui->tabWidget_EventType->indexOf(ui->tab_Multiple));
    }
    else if (events->length() > 1)
    {
        ui->tabWidget_EventType->addTab(ui->tab_Multiple, "Multiple");
        ui->tabWidget_EventType->setCurrentWidget(ui->tab_Multiple);
    }

    isProgrammaticEventTabChange = false;

    if (events->length() != 0)
    {
        if (target->children().length())
        {
            for (QObject *obj : target->children())
            {
                obj->deleteLater();
            }

            delete target->layout();
        }

        QVBoxLayout *layout = new QVBoxLayout(target);
        target->setLayout(layout);
        scrollTarget->setWidgetResizable(true);
        scrollTarget->setWidget(target);

        for (EventPropertiesFrame *frame : frames) {
            layout->addWidget(frame);
        }

        layout->addStretch(1);

        // doesn't work
        //QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        //ui->scrollArea_4->ensureVisible(0, scroll);

        ui->label_NoEvents->hide();
        ui->tabWidget_EventType->show();
    }
    else
    {
        ui->tabWidget_EventType->hide();
        ui->label_NoEvents->show();
    }
}

QString MainWindow::getEventGroupFromTabWidget(QWidget *tab)
{
    QString ret = "";
    if (tab == eventTabObjectWidget)
    {
        ret = "object_event_group";
    }
    else if (tab == eventTabWarpWidget)
    {
        ret = "warp_event_group";
    }
    else if (tab == eventTabTriggerWidget)
    {
        ret = "coord_event_group";
    }
    else if (tab == eventTabBGWidget)
    {
        ret = "bg_event_group";
    }
    else if (tab == eventTabHealspotWidget)
    {
        ret = "heal_event_group";
    }
    return ret;
}

void MainWindow::eventTabChanged(int index)
{
    if (!isProgrammaticEventTabChange && editor->map != nullptr)
    {
        QString group = getEventGroupFromTabWidget(ui->tabWidget_EventType->widget(index));
        DraggablePixmapItem *selectedEvent = nullptr;

        if (group == "object_event_group")
        {
            if (selectedObject == nullptr && editor->map->events.value(group).count())
            {
                Event *event = editor->map->events.value(group).at(0);
                for (QGraphicsItem *child : editor->events_group->childItems()) {
                    DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
                    if (item->event == event) {
                        selectedObject = item;
                        break;
                    }
                }
            }

            selectedEvent = selectedObject;
        }
        else if (group == "warp_event_group")
        {
            if (selectedWarp == nullptr && editor->map->events.value(group).count())
            {
                Event *event = editor->map->events.value(group).at(0);
                for (QGraphicsItem *child : editor->events_group->childItems()) {
                    DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
                    if (item->event == event) {
                        selectedWarp = item;
                        break;
                    }
                }
            }

            selectedEvent = selectedWarp;
        }
        else if (group == "coord_event_group")
        {
            if (selectedTrigger == nullptr && editor->map->events.value(group).count())
            {
                Event *event = editor->map->events.value(group).at(0);
                for (QGraphicsItem *child : editor->events_group->childItems()) {
                    DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
                    if (item->event == event) {
                        selectedTrigger = item;
                        break;
                    }
                }
            }

            selectedEvent = selectedTrigger;
        }
        else if (group == "bg_event_group")
        {
            if (selectedBG == nullptr && editor->map->events.value(group).count())
            {
                Event *event = editor->map->events.value(group).at(0);
                for (QGraphicsItem *child : editor->events_group->childItems()) {
                    DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
                    if (item->event == event) {
                        selectedBG = item;
                        break;
                    }
                }
            }

            selectedEvent = selectedBG;
        }
        else if (group == "heal_event_group")
        {
            if (selectedHealspot == nullptr && editor->map->events.value(group).count())
            {
                Event *event = editor->map->events.value(group).at(0);
                for (QGraphicsItem *child : editor->events_group->childItems()) {
                    DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
                    if (item->event == event) {
                        selectedHealspot = item;
                        break;
                    }
                }
            }

            selectedEvent = selectedHealspot;
        }

        if (selectedEvent != nullptr)
            editor->selectMapEvent(selectedEvent);
    }

    isProgrammaticEventTabChange = false;
}

void MainWindow::selectedEventIndexChanged(int index)
{
    QString group = getEventGroupFromTabWidget(ui->tabWidget_EventType->currentWidget());
    int event_offs = group == "warp_event_group" ? 0 : 1;
    Event *event = editor->map->events.value(group).at(index - event_offs);
    DraggablePixmapItem *selectedEvent = nullptr;
    for (QGraphicsItem *child : editor->events_group->childItems()) {
        DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
        if (item->event == event) {
            selectedEvent = item;
            break;
        }
    }

    if (selectedEvent != nullptr)
        editor->selectMapEvent(selectedEvent);
}

void MainWindow::on_horizontalSlider_CollisionTransparency_valueChanged(int value) {
    this->editor->collisionOpacity = static_cast<qreal>(value) / 100;
    porymapConfig.setCollisionOpacity(value);
    this->editor->collision_item->draw(true);
}

// TODO: connect this to the DEL key when undo/redo history is extended to events
void MainWindow::on_toolButton_deleteObject_clicked()
{
    if (editor && editor->selected_events) {
        if (editor->selected_events->length()) {
            DraggablePixmapItem *next_selected_event = nullptr;
            for (DraggablePixmapItem *item : *editor->selected_events) {
                QString event_group = item->event->get("event_group_type");
                int index = editor->map->events.value(event_group).indexOf(item->event);
                // Get the index for the event that should be selected after this event has been deleted.
                // If it's at the end of the list, select the previous event, otherwise select the next one.
                if (index != editor->map->events.value(event_group).size() - 1)
                    index++;
                else
                    index--;
                Event *event = nullptr;
                if (index >= 0)
                    event = editor->map->events.value(event_group).at(index);
                if (event_group != "heal_event_group") {
                    for (QGraphicsItem *child : editor->events_group->childItems()) {
                        DraggablePixmapItem *event_item = static_cast<DraggablePixmapItem *>(child);
                        if (event_item->event == event) {
                            next_selected_event = event_item;
                            break;
                        }
                    }
                    editor->deleteEvent(item->event);
                    if (editor->scene->items().contains(item)) {
                        editor->scene->removeItem(item);
                    }
                    editor->selected_events->removeOne(item);
                }
                else { // don't allow deletion of heal locations
                    logWarn(QString("Cannot delete event of type '%1'").arg(item->event->get("event_type")));
                }
            }
            if (next_selected_event) {
                editor->selectMapEvent(next_selected_event);
            }
            else {
                updateObjects();
            }
        }
    }
}

void MainWindow::on_toolButton_Open_Scripts_clicked()
{
    openInTextEditor();
}

void MainWindow::on_toolButton_Paint_clicked()
{
    editor->map_edit_mode = "paint";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pencil_cursor.ico"), 10, 10);

    // do not stop single tile mode when editing collision
    if (ui->tabWidget_2->currentIndex() == 0)
        editor->cursorMapTileRect->stopSingleTileMode();

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Select_clicked()
{
    editor->map_edit_mode = "select";
    editor->settings->mapCursor = QCursor();
    editor->cursorMapTileRect->setSingleTileMode();

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Fill_clicked()
{
    editor->map_edit_mode = "fill";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/fill_color_cursor.ico"), 10, 10);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Dropper_clicked()
{
    editor->map_edit_mode = "pick";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pipette_cursor.ico"), 10, 10);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::on_toolButton_Move_clicked()
{
    editor->map_edit_mode = "move";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/move.ico"), 7, 7);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(ui->scrollArea, QScroller::LeftMouseButtonGesture);

    checkToolButtons();
}

void MainWindow::on_toolButton_Shift_clicked()
{
    editor->map_edit_mode = "shift";
    editor->settings->mapCursor = QCursor(QPixmap(":/icons/shift_cursor.ico"), 10, 10);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->scrollArea);

    checkToolButtons();
}

void MainWindow::checkToolButtons() {
    ui->toolButton_Paint->setChecked(editor->map_edit_mode == "paint");
    ui->toolButton_Select->setChecked(editor->map_edit_mode == "select");
    ui->toolButton_Fill->setChecked(editor->map_edit_mode == "fill");
    ui->toolButton_Dropper->setChecked(editor->map_edit_mode == "pick");
    ui->toolButton_Move->setChecked(editor->map_edit_mode == "move");
    ui->toolButton_Shift->setChecked(editor->map_edit_mode == "shift");
}

void MainWindow::clickToolButtonFromEditMode(QString editMode) {
    if (editMode == "paint") {
        on_toolButton_Paint_clicked();
    } else if (editMode == "select") {
        on_toolButton_Select_clicked();
    } else if (editMode == "fill") {
        on_toolButton_Fill_clicked();
    } else if (editMode == "pick") {
        on_toolButton_Dropper_clicked();
    } else if (editMode == "move") {
        on_toolButton_Move_clicked();
    } else if (editMode == "shift") {
        on_toolButton_Shift_clicked();
    }
}

void MainWindow::onLoadMapRequested(QString mapName, QString fromMapName) {
    if (!setMap(mapName, true)) {
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening map %1. Please see %2 for full error details.\n\n%3")
                .arg(mapName)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Map", errorMsg);
        return;
    }
    editor->setSelectedConnectionFromMap(fromMapName);
}

void MainWindow::onMapChanged(Map *map) {
    map->layout->has_unsaved_changes = true;
    updateMapList();
}

void MainWindow::onMapNeedsRedrawing() {
    redrawMapScene();
}

void MainWindow::onTilesetsSaved(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    this->editor->updatePrimaryTileset(primaryTilesetLabel, true);
    this->editor->updateSecondaryTileset(secondaryTilesetLabel, true);
}

void MainWindow::on_action_Export_Map_Image_triggered() {
    showExportMapImageWindow(false);
}

void MainWindow::on_actionExport_Stitched_Map_Image_triggered() {
   showExportMapImageWindow(true);
}

void MainWindow::showExportMapImageWindow(bool stitchMode) {
    if (this->mapImageExporter)
        delete this->mapImageExporter;

    this->mapImageExporter = new MapImageExporter(this, this->editor, stitchMode);
    connect(this->mapImageExporter, &QObject::destroyed, [=](QObject *) { this->mapImageExporter = nullptr; });
    this->mapImageExporter->setAttribute(Qt::WA_DeleteOnClose);

    if (!this->mapImageExporter->isVisible()) {
        this->mapImageExporter->show();
    } else if (this->mapImageExporter->isMinimized()) {
        this->mapImageExporter->showNormal();
    } else {
        this->mapImageExporter->activateWindow();
    }
}

void MainWindow::on_comboBox_ConnectionDirection_currentIndexChanged(const QString &direction)
{
    editor->updateCurrentConnectionDirection(direction);
}

void MainWindow::on_spinBox_ConnectionOffset_valueChanged(int offset)
{
    editor->updateConnectionOffset(offset);
}

void MainWindow::on_comboBox_ConnectedMap_currentTextChanged(const QString &mapName)
{
    if (editor->project->mapNames->contains(mapName))
        editor->setConnectionMap(mapName);
}

void MainWindow::on_pushButton_AddConnection_clicked()
{
    editor->addNewConnection();
}

void MainWindow::on_pushButton_RemoveConnection_clicked()
{
    editor->removeCurrentConnection();
}

void MainWindow::on_pushButton_NewWildMonGroup_clicked() {
    editor->addNewWildMonGroup(this);
}

void MainWindow::on_pushButton_ConfigureEncountersJSON_clicked() {
    editor->configureEncounterJSON(this);
}

void MainWindow::on_comboBox_DiveMap_currentTextChanged(const QString &mapName)
{
    if (editor->project->mapNames->contains(mapName))
        editor->updateDiveMap(mapName);
}

void MainWindow::on_comboBox_EmergeMap_currentTextChanged(const QString &mapName)
{
    if (editor->project->mapNames->contains(mapName))
        editor->updateEmergeMap(mapName);
}

void MainWindow::on_comboBox_PrimaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->tilesetLabels["primary"].contains(tilesetLabel) && editor->map) {
        editor->updatePrimaryTileset(tilesetLabel);
        on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
    }
}

void MainWindow::on_comboBox_SecondaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->tilesetLabels["secondary"].contains(tilesetLabel) && editor->map) {
        editor->updateSecondaryTileset(tilesetLabel);
        on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
    }
}

void MainWindow::on_pushButton_ChangeDimensions_clicked()
{
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("Change Map Dimensions");
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);

    QSpinBox *widthSpinBox = new QSpinBox();
    QSpinBox *heightSpinBox = new QSpinBox();
    QSpinBox *bwidthSpinBox = new QSpinBox();
    QSpinBox *bheightSpinBox = new QSpinBox();
    widthSpinBox->setMinimum(1);
    heightSpinBox->setMinimum(1);
    bwidthSpinBox->setMinimum(1);
    bheightSpinBox->setMinimum(1);
    // See below for explanation of maximum map dimensions
    widthSpinBox->setMaximum(0x1E7);
    heightSpinBox->setMaximum(0x1D1);
    // Maximum based only on data type (u8) of map border width/height
    bwidthSpinBox->setMaximum(255);
    bheightSpinBox->setMaximum(255);
    widthSpinBox->setValue(editor->map->getWidth());
    heightSpinBox->setValue(editor->map->getHeight());
    bwidthSpinBox->setValue(editor->map->getBorderWidth());
    bheightSpinBox->setValue(editor->map->getBorderHeight());
    if (projectConfig.getUseCustomBorderSize()) {
        form.addRow(new QLabel("Map Width"), widthSpinBox);
        form.addRow(new QLabel("Map Height"), heightSpinBox);
        form.addRow(new QLabel("Border Width"), bwidthSpinBox);
        form.addRow(new QLabel("Border Height"), bheightSpinBox);
    } else {
        form.addRow(new QLabel("Width"), widthSpinBox);
        form.addRow(new QLabel("Height"), heightSpinBox);
    }

    QLabel *errorLabel = new QLabel();
    QPalette errorPalette;
    errorPalette.setColor(QPalette::WindowText, Qt::red);
    errorLabel->setPalette(errorPalette);
    errorLabel->setVisible(false);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, [&dialog, &widthSpinBox, &heightSpinBox, &errorLabel](){
        // Ensure width and height are an acceptable size.
        // The maximum number of metatiles in a map is the following:
        //    max = (width + 15) * (height + 14)
        // This limit can be found in fieldmap.c in pokeruby/pokeemerald.
        int realWidth = widthSpinBox->value() + 15;
        int realHeight = heightSpinBox->value() + 14;
        int numMetatiles = realWidth * realHeight;
        if (numMetatiles <= 0x2800) {
            dialog.accept();
        } else {
            QString errorText = QString("Error: The specified width and height are too large.\n"
                    "The maximum map width and height is the following: (width + 15) * (height + 14) <= 10240\n"
                    "The specified map width and height was: (%1 + 15) * (%2 + 14) = %3")
                        .arg(widthSpinBox->value())
                        .arg(heightSpinBox->value())
                        .arg(numMetatiles);
            errorLabel->setText(errorText);
            errorLabel->setVisible(true);
        }
    });
    connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    form.addRow(errorLabel);

    if (dialog.exec() == QDialog::Accepted) {
        editor->map->setDimensions(widthSpinBox->value(), heightSpinBox->value());
        editor->map->setBorderDimensions(bwidthSpinBox->value(), bheightSpinBox->value());
        editor->map->commit();
        onMapNeedsRedrawing();
    }
}

void MainWindow::on_checkBox_smartPaths_stateChanged(int selected)
{
    bool enabled = selected == Qt::Checked;
    editor->settings->smartPathsEnabled = enabled;
    if (enabled) {
        editor->cursorMapTileRect->setSmartPathMode();
    } else {
        editor->cursorMapTileRect->setNormalPathMode();
    }
}

void MainWindow::on_checkBox_ToggleBorder_stateChanged(int selected)
{
    bool visible = selected != 0;
    editor->toggleBorderVisibility(visible);
}

void MainWindow::on_actionTileset_Editor_triggered()
{
    if (!this->tilesetEditor) {
        this->tilesetEditor = new TilesetEditor(this->editor->project, this->editor->map->layout->tileset_primary_label, this->editor->map->layout->tileset_secondary_label, this);
        connect(this->tilesetEditor, SIGNAL(tilesetsSaved(QString, QString)), this, SLOT(onTilesetsSaved(QString, QString)));
        connect(this->tilesetEditor, &QObject::destroyed, [=](QObject *) { this->tilesetEditor = nullptr; });
        this->tilesetEditor->setAttribute(Qt::WA_DeleteOnClose);
    }

    if (!this->tilesetEditor->isVisible()) {
        this->tilesetEditor->show();
    } else if (this->tilesetEditor->isMinimized()) {
        this->tilesetEditor->showNormal();
    } else {
        this->tilesetEditor->activateWindow();
    }
}

void MainWindow::on_toolButton_ExpandAll_clicked()
{
    if (ui->mapList) {
        ui->mapList->expandToDepth(0);
    }
}

void MainWindow::on_toolButton_CollapseAll_clicked()
{
    if (ui->mapList) {
        ui->mapList->collapseAll();
    }
}

void MainWindow::on_actionAbout_Porymap_triggered()
{
    AboutPorymap *window = new AboutPorymap(this);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->show();
}

void MainWindow::on_actionThemes_triggered()
{
    QStringList themes;
    QRegularExpression re(":/themes/([A-z0-9_-]+).qss");
    themes.append("default");
    QDirIterator it(":/themes", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString themeName = re.match(it.next()).captured(1);
        themes.append(themeName);
    }

    QDialog themeSelectorWindow(this);
    QFormLayout form(&themeSelectorWindow);

    NoScrollComboBox *themeSelector = new NoScrollComboBox();
    themeSelector->addItems(themes);
    themeSelector->setCurrentText(porymapConfig.getTheme());
    form.addRow(new QLabel("Themes"), themeSelector);

    QDialogButtonBox buttonBox(QDialogButtonBox::Apply | QDialogButtonBox::Close, Qt::Horizontal, &themeSelectorWindow);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::clicked, [&themeSelectorWindow, &buttonBox, themeSelector, this](QAbstractButton *button){
        if (button == buttonBox.button(QDialogButtonBox::Apply)) {
            QString theme = themeSelector->currentText();
            porymapConfig.setTheme(theme);
            this->setTheme(theme);
        }
    });
    connect(&buttonBox, SIGNAL(rejected()), &themeSelectorWindow, SLOT(reject()));

    themeSelectorWindow.exec();
}

void MainWindow::on_pushButton_AddCustomHeaderField_clicked()
{
    int rowIndex = this->ui->tableWidget_CustomHeaderFields->rowCount();
    this->ui->tableWidget_CustomHeaderFields->insertRow(rowIndex);
    this->ui->tableWidget_CustomHeaderFields->selectRow(rowIndex);
    this->editor->updateCustomMapHeaderValues(this->ui->tableWidget_CustomHeaderFields);
}

void MainWindow::on_pushButton_DeleteCustomHeaderField_clicked()
{
    int rowCount = this->ui->tableWidget_CustomHeaderFields->rowCount();
    if (rowCount > 0) {
        QModelIndexList indexList = ui->tableWidget_CustomHeaderFields->selectionModel()->selectedIndexes();
        QList<QPersistentModelIndex> persistentIndexes;
        for (QModelIndex index : indexList) {
            QPersistentModelIndex persistentIndex(index);
            persistentIndexes.append(persistentIndex);
        }

        for (QPersistentModelIndex index : persistentIndexes) {
            this->ui->tableWidget_CustomHeaderFields->removeRow(index.row());
        }

        if (this->ui->tableWidget_CustomHeaderFields->rowCount() > 0) {
            this->ui->tableWidget_CustomHeaderFields->selectRow(0);
        }

        this->editor->updateCustomMapHeaderValues(this->ui->tableWidget_CustomHeaderFields);
    }
}

void MainWindow::on_tableWidget_CustomHeaderFields_cellChanged(int, int)
{
    this->editor->updateCustomMapHeaderValues(this->ui->tableWidget_CustomHeaderFields);
}

void MainWindow::on_horizontalSlider_MetatileZoom_valueChanged(int value) {
    porymapConfig.setMetatilesZoom(value);
    double scale = pow(3.0, static_cast<double>(value - 30) / 30.0);

    QMatrix matrix;
    matrix.scale(scale, scale);
    QSize size(editor->metatile_selector_item->pixmap().width(), 
               editor->metatile_selector_item->pixmap().height());
    size *= scale;

    ui->graphicsView_Metatiles->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Metatiles->setMatrix(matrix);
    ui->graphicsView_Metatiles->setFixedSize(size.width() + 2, size.height() + 2);

    ui->graphicsView_BorderMetatile->setMatrix(matrix);
    ui->graphicsView_BorderMetatile->setFixedSize(ceil(static_cast<double>(editor->selected_border_metatiles_item->pixmap().width()) * scale) + 2,
                                                  ceil(static_cast<double>(editor->selected_border_metatiles_item->pixmap().height()) * scale) + 2);

    ui->graphicsView_currentMetatileSelection->setMatrix(matrix);
    currentMetatilesSelectionChanged();
}

void MainWindow::on_actionRegion_Map_Editor_triggered() {
    if (!this->regionMapEditor) {
        this->regionMapEditor = new RegionMapEditor(this, this->editor->project);
        bool success = this->regionMapEditor->loadRegionMapData()
                    && this->regionMapEditor->loadCityMaps();
        if (!success) {
            delete this->regionMapEditor;
            this->regionMapEditor = nullptr;
            QMessageBox msgBox(this);
            QString errorMsg = QString("There was an error opening the region map data. Please see %1 for full error details.\n\n%3")
                    .arg(getLogPath())
                    .arg(getMostRecentError());
            msgBox.critical(nullptr, "Error Opening Region Map Editor", errorMsg);
            return;
        }
        connect(this->regionMapEditor, &QObject::destroyed, [=](QObject *) { this->regionMapEditor = nullptr; });
        this->regionMapEditor->setAttribute(Qt::WA_DeleteOnClose);
    }

    if (!this->regionMapEditor->isVisible()) {
        this->regionMapEditor->show();
    } else if (this->regionMapEditor->isMinimized()) {
        this->regionMapEditor->showNormal();
    } else {
        this->regionMapEditor->activateWindow();
    }
}

void MainWindow::closeSupplementaryWindows() {
    if (this->tilesetEditor)
        delete this->tilesetEditor;
    if (this->regionMapEditor)
        delete this->regionMapEditor;
    if (this->mapImageExporter)
        delete this->mapImageExporter;
    if (this->newmapprompt)
        delete this->newmapprompt;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (projectHasUnsavedChanges || editor->map->hasUnsavedChanges()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, "porymap", "The project has been modified, save changes?",
            QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

        if (result == QMessageBox::Yes) {
            editor->saveProject();
        } else if (result == QMessageBox::No) {
            logWarn("Closing porymap with unsaved changes.");
        } else if (result == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    porymapConfig.setGeometry(
        this->saveGeometry(),
        this->saveState(),
        this->ui->splitter_map->saveState(),
        this->ui->splitter_main->saveState()
    );
    porymapConfig.save();
    projectConfig.save();

    QMainWindow::closeEvent(event);
}
