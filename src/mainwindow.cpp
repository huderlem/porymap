#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutporymap.h"
#include "project.h"
#include "log.h"
#include "editor.h"
#include "prefabcreationdialog.h"
#include "eventframes.h"
#include "bordermetatilespixmapitem.h"
#include "currentselectedmetatilespixmapitem.h"
#include "customattributestable.h"
#include "scripting.h"
#include "adjustingstackedwidget.h"
#include "draggablepixmapitem.h"
#include "editcommands.h"
#include "flowlayout.h"
#include "shortcut.h"
#include "mapparser.h"
#include "prefab.h"
#include "montabwidget.h"
#include "imageexport.h"

#include <QFileDialog>
#include <QClipboard>
#include <QDirIterator>
#include <QStandardItemModel>
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
#include <QSysInfo>
#include <QDesktopServices>
#include <QTransform>
#include <QSignalBlocker>
#include <QSet>
#include <QLoggingCategory>

using OrderedJson = poryjson::Json;
using OrderedJsonDoc = poryjson::JsonDoc;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isProgrammaticEventTabChange(false)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("porymap");
    QApplication::setApplicationDisplayName("porymap");
    QApplication::setWindowIcon(QIcon(":/icons/porymap-icon-2.ico"));
    ui->setupUi(this);

    cleanupLargeLog();

    this->initWindow();
    if (this->openRecentProject())
        on_toolButton_Paint_clicked();

    // there is a bug affecting macOS users, where the trackpad deilveres a bad touch-release gesture
    // the warning is a bit annoying, so it is disabled here
    QLoggingCategory::setFilterRules(QStringLiteral("qt.pointer.dispatch=false"));
}

MainWindow::~MainWindow()
{
    delete label_MapRulerStatus;
    delete ui;
}

void MainWindow::setWindowDisabled(bool disabled) {
    for (auto action : findChildren<QAction *>())
        action->setDisabled(disabled);
    for (auto child : findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly))
        child->setDisabled(disabled);
    for (auto menu : ui->menuBar->findChildren<QMenu *>(QString(), Qt::FindDirectChildrenOnly))
        menu->setDisabled(disabled);
    ui->menuBar->setDisabled(false);
    ui->menuFile->setDisabled(false);
    ui->action_Open_Project->setDisabled(false);
    ui->menuOpen_Recent_Project->setDisabled(false);
    refreshRecentProjectsMenu();
    ui->action_Exit->setDisabled(false);
    ui->menuHelp->setDisabled(false);
    ui->actionAbout_Porymap->setDisabled(false);
    ui->actionOpen_Log_File->setDisabled(false);
    ui->actionOpen_Config_Folder->setDisabled(false);
    if (!disabled)
        togglePreferenceSpecificUi();
}

void MainWindow::initWindow() {
    porymapConfig.load();
    this->initCustomUI();
    this->initExtraSignals();
    this->initEditor();
    this->initMiscHeapObjects();
    this->initMapSortOrder();
    this->initShortcuts();
    this->restoreWindowState();

    setWindowDisabled(true);
}

void MainWindow::initShortcuts() {
    initExtraShortcuts();

    shortcutsConfig.load();
    shortcutsConfig.setDefaultShortcuts(shortcutableObjects());
    applyUserShortcuts();
}

void MainWindow::initExtraShortcuts() {
    ui->actionZoom_In->setShortcuts({ui->actionZoom_In->shortcut(), QKeySequence("Ctrl+=")});

    auto *shortcutReset_Zoom = new Shortcut(QKeySequence("Ctrl+0"), this, SLOT(resetMapViewScale()));
    shortcutReset_Zoom->setObjectName("shortcutZoom_Reset");
    shortcutReset_Zoom->setWhatsThis("Zoom Reset");

    auto *shortcutToggle_Grid = new Shortcut(QKeySequence("Ctrl+G"), ui->checkBox_ToggleGrid, SLOT(toggle()));
    shortcutToggle_Grid->setObjectName("shortcutToggle_Grid");
    shortcutToggle_Grid->setWhatsThis("Toggle Grid");

    auto *shortcutDuplicate_Events = new Shortcut(QKeySequence("Ctrl+D"), this, SLOT(duplicate()));
    shortcutDuplicate_Events->setObjectName("shortcutDuplicate_Events");
    shortcutDuplicate_Events->setWhatsThis("Duplicate Selected Event(s)");

    auto *shortcutDelete_Object = new Shortcut(
            {QKeySequence("Del"), QKeySequence("Backspace")}, this, SLOT(on_toolButton_deleteObject_clicked()));
    shortcutDelete_Object->setObjectName("shortcutDelete_Object");
    shortcutDelete_Object->setWhatsThis("Delete Selected Event(s)");

    auto *shortcutToggle_Border = new Shortcut(QKeySequence(), ui->checkBox_ToggleBorder, SLOT(toggle()));
    shortcutToggle_Border->setObjectName("shortcutToggle_Border");
    shortcutToggle_Border->setWhatsThis("Toggle Border");

    auto *shortcutToggle_Smart_Paths = new Shortcut(QKeySequence(), ui->checkBox_smartPaths, SLOT(toggle()));
    shortcutToggle_Smart_Paths->setObjectName("shortcutToggle_Smart_Paths");
    shortcutToggle_Smart_Paths->setWhatsThis("Toggle Smart Paths");

    auto *shortcutExpand_All = new Shortcut(QKeySequence(), this, SLOT(on_toolButton_ExpandAll_clicked()));
    shortcutExpand_All->setObjectName("shortcutExpand_All");
    shortcutExpand_All->setWhatsThis("Map List: Expand all folders");

    auto *shortcutCollapse_All = new Shortcut(QKeySequence(), this, SLOT(on_toolButton_CollapseAll_clicked()));
    shortcutCollapse_All->setObjectName("shortcutCollapse_All");
    shortcutCollapse_All->setWhatsThis("Map List: Collapse all folders");

    auto *shortcut_Open_Scripts = new Shortcut(QKeySequence(), ui->toolButton_Open_Scripts, SLOT(click()));
    shortcut_Open_Scripts->setObjectName("shortcut_Open_Scripts");
    shortcut_Open_Scripts->setWhatsThis("Open Map Scripts");

    copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence("Ctrl+C"));
    connect(copyAction, &QAction::triggered, this, &MainWindow::copy);
    ui->menuEdit->addSeparator();
    ui->menuEdit->addAction(copyAction);

    pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence("Ctrl+V"));
    connect(pasteAction, &QAction::triggered, this, &MainWindow::paste);
    ui->menuEdit->addAction(pasteAction);
}

QObjectList MainWindow::shortcutableObjects() const {
    QObjectList shortcutable_objects;

    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(shortcut));

    return shortcutable_objects;
}

void MainWindow::applyUserShortcuts() {
    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            action->setShortcuts(shortcutsConfig.userShortcuts(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcut->setKeys(shortcutsConfig.userShortcuts(shortcut));
}

void MainWindow::initCustomUI() {
    // Set up the tab bar
    while (ui->mainTabBar->count()) ui->mainTabBar->removeTab(0);
    ui->mainTabBar->addTab("Map");
    ui->mainTabBar->setTabIcon(0, QIcon(QStringLiteral(":/icons/map.ico")));
    ui->mainTabBar->addTab("Events");
    ui->mainTabBar->addTab("Header");
    ui->mainTabBar->addTab("Connections");
    ui->mainTabBar->addTab("Wild Pokemon");
    ui->mainTabBar->setTabIcon(4, QIcon(QStringLiteral(":/icons/tall_grass.ico")));
}

void MainWindow::initExtraSignals() {
    // Right-clicking on items in the map list tree view brings up a context menu.
    ui->mapList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->mapList, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onOpenMapListContextMenu);

    // other signals
    connect(ui->newEventToolButton, &NewEventToolButton::newEventAdded, this, &MainWindow::addNewEvent);
    connect(ui->tabWidget_EventType, &QTabWidget::currentChanged, this, &MainWindow::eventTabChanged);

    // Change pages on wild encounter groups
    QStackedWidget *stack = ui->stackedWidget_WildMons;
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;
    connect(labelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index){
        stack->setCurrentIndex(index);
    });

    // Convert the layout of the map tools' frame into an adjustable FlowLayout
    FlowLayout *flowLayout = new FlowLayout;
    flowLayout->setContentsMargins(ui->frame_mapTools->layout()->contentsMargins());
    flowLayout->setSpacing(ui->frame_mapTools->layout()->spacing());
    for (auto *child : ui->frame_mapTools->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        flowLayout->addWidget(child);
        child->setFixedHeight(
            ui->frame_mapTools->height() - flowLayout->contentsMargins().top() - flowLayout->contentsMargins().bottom()
        );
    }
    delete ui->frame_mapTools->layout();
    ui->frame_mapTools->setLayout(flowLayout);

    // Floating QLabel tool-window that displays over the map when the ruler is active
    label_MapRulerStatus = new QLabel(ui->graphicsView_Map);
    label_MapRulerStatus->setObjectName("label_MapRulerStatus");
    label_MapRulerStatus->setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    label_MapRulerStatus->setFrameShape(QFrame::Box);
    label_MapRulerStatus->setMargin(3);
    label_MapRulerStatus->setPalette(palette());
    label_MapRulerStatus->setAlignment(Qt::AlignCenter);
    label_MapRulerStatus->setTextFormat(Qt::PlainText);
    label_MapRulerStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
}

void MainWindow::initEditor() {
    this->editor = new Editor(ui);
    connect(this->editor, &Editor::objectsChanged, this, &MainWindow::updateObjects);
    connect(this->editor, &Editor::loadMapRequested, this, &MainWindow::onLoadMapRequested);
    connect(this->editor, &Editor::warpEventDoubleClicked, this, &MainWindow::openWarpMap);
    connect(this->editor, &Editor::currentMetatilesSelectionChanged, this, &MainWindow::currentMetatilesSelectionChanged);
    connect(this->editor, &Editor::wildMonDataChanged, this, &MainWindow::onWildMonDataChanged);
    connect(this->editor, &Editor::mapRulerStatusChanged, this, &MainWindow::onMapRulerStatusChanged);
    connect(this->editor, &Editor::editedMapData, this, &MainWindow::markMapEdited);
    connect(this->editor, &Editor::tilesetUpdated, this, &Scripting::cb_TilesetUpdated);
    connect(ui->toolButton_Open_Scripts, &QToolButton::pressed, this->editor, &Editor::openMapScripts);
    connect(ui->actionOpen_Project_in_Text_Editor, &QAction::triggered, this->editor, &Editor::openProjectInTextEditor);

    this->loadUserSettings();

    undoAction = editor->editGroup.createUndoAction(this, tr("&Undo"));
    undoAction->setObjectName("action_Undo");
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));

    redoAction = editor->editGroup.createRedoAction(this, tr("&Redo"));
    redoAction->setObjectName("action_Redo");
    redoAction->setShortcuts({QKeySequence("Ctrl+Y"), QKeySequence("Ctrl+Shift+Z")});

    ui->menuEdit->addAction(undoAction);
    ui->menuEdit->addAction(redoAction);

    QUndoView *undoView = new QUndoView(&editor->editGroup);
    undoView->setWindowTitle(tr("Edit History"));
    undoView->setAttribute(Qt::WA_QuitOnClose, false);

    // Show the EditHistory dialog with Ctrl+E
    QAction *showHistory = new QAction("Show Edit History...", this);
    showHistory->setObjectName("action_ShowEditHistory");
    showHistory->setShortcut(QKeySequence("Ctrl+E"));
    connect(showHistory, &QAction::triggered, [undoView](){ undoView->show(); });

    ui->menuEdit->addAction(showHistory);

    // Toggle an asterisk in the window title when the undo state is changed
    connect(&editor->editGroup, &QUndoGroup::cleanChanged, this, &MainWindow::showWindowTitle);

    // selecting objects from the spinners
    connect(this->ui->spinner_ObjectID, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->editor->selectedEventIndexChanged(value, Event::Group::Object);
    });
    connect(this->ui->spinner_WarpID, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->editor->selectedEventIndexChanged(value, Event::Group::Warp);
    });
    connect(this->ui->spinner_TriggerID, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->editor->selectedEventIndexChanged(value, Event::Group::Coord);
    });
    connect(this->ui->spinner_BgID, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->editor->selectedEventIndexChanged(value, Event::Group::Bg);
    });
    connect(this->ui->spinner_HealID, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->editor->selectedEventIndexChanged(value, Event::Group::Heal);
    });
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

    connect(mapSortOrderActionGroup, &QActionGroup::triggered, this, &MainWindow::mapSortOrder_changed);

    QAction* sortOrder = ui->toolButton_MapSortOrder->menu()->actions()[mapSortOrder];
    ui->toolButton_MapSortOrder->setIcon(sortOrder->icon());
    sortOrder->setChecked(true);
}

void MainWindow::showWindowTitle() {
    if (editor->map) {
        setWindowTitle(QString("%1%2 - %3")
            .arg(editor->map->hasUnsavedChanges() ? "* " : "")
            .arg(editor->map->name)
            .arg(editor->project->getProjectTitle())
        );
    }
}

void MainWindow::markMapEdited() {
    if (editor && editor->map) {
        editor->map->hasUnsavedDataChanges = true;
        showWindowTitle();
    }
}

void MainWindow::setWildEncountersUIEnabled(bool enabled) {
    ui->mainTabBar->setTabEnabled(4, enabled);
}

// Update the UI using information we've read from the user's project files.
void MainWindow::setProjectSpecificUI()
{
    this->setWildEncountersUIEnabled(userConfig.getEncounterJsonActive());

    bool hasFlags = projectConfig.getMapAllowFlagsEnabled();
    ui->checkBox_AllowRunning->setVisible(hasFlags);
    ui->checkBox_AllowBiking->setVisible(hasFlags);
    ui->checkBox_AllowEscaping->setVisible(hasFlags);
    ui->label_AllowRunning->setVisible(hasFlags);
    ui->label_AllowBiking->setVisible(hasFlags);
    ui->label_AllowEscaping->setVisible(hasFlags);

    ui->newEventToolButton->newWeatherTriggerAction->setVisible(projectConfig.getEventWeatherTriggerEnabled());
    ui->newEventToolButton->newSecretBaseAction->setVisible(projectConfig.getEventSecretBaseEnabled());
    ui->newEventToolButton->newCloneObjectAction->setVisible(projectConfig.getEventCloneObjectEnabled());

    bool floorNumEnabled = projectConfig.getFloorNumberEnabled();
    ui->spinBox_FloorNumber->setVisible(floorNumEnabled);
    ui->label_FloorNumber->setVisible(floorNumEnabled);

    Event::setIcons();
    editor->setCollisionGraphics();
    ui->spinBox_SelectedElevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_SelectedCollision->setMaximum(Block::getMaxCollision());
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
            applyMapListFilter(ui->lineEdit_filterBox->text());
        }
    }
}

void MainWindow::on_lineEdit_filterBox_textChanged(const QString &arg1)
{
    this->applyMapListFilter(arg1);
}

void MainWindow::applyMapListFilter(QString filterText)
{
    mapListProxyModel->setFilterRegularExpression(QRegularExpression(filterText, QRegularExpression::CaseInsensitiveOption));
    if (filterText.isEmpty()) {
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
    ui->checkBox_ToggleBorder->setChecked(porymapConfig.getShowBorder());
    ui->checkBox_ToggleGrid->setChecked(porymapConfig.getShowGrid());
    mapSortOrder = porymapConfig.getMapSortOrder();
    ui->horizontalSlider_CollisionTransparency->blockSignals(true);
    this->editor->collisionOpacity = static_cast<qreal>(porymapConfig.getCollisionOpacity()) / 100;
    ui->horizontalSlider_CollisionTransparency->setValue(porymapConfig.getCollisionOpacity());
    ui->horizontalSlider_CollisionTransparency->blockSignals(false);
    ui->horizontalSlider_MetatileZoom->blockSignals(true);
    ui->horizontalSlider_MetatileZoom->setValue(porymapConfig.getMetatilesZoom());
    ui->horizontalSlider_MetatileZoom->blockSignals(false);
    ui->horizontalSlider_CollisionZoom->blockSignals(true);
    ui->horizontalSlider_CollisionZoom->setValue(porymapConfig.getCollisionZoom());
    ui->horizontalSlider_CollisionZoom->blockSignals(false);
    setTheme(porymapConfig.getTheme());
}

void MainWindow::restoreWindowState() {
    logInfo("Restoring main window geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getMainGeometry();
    this->restoreGeometry(geometry.value("main_window_geometry"));
    this->restoreState(geometry.value("main_window_state"));
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
    if (!porymapConfig.getReopenOnLaunch())
        return false;

    QString default_dir = porymapConfig.getRecentProject();
    if (default_dir.isNull() || default_dir.length() <= 0)
        return false;

    if (!QDir(default_dir).exists()) {
        QString message = QString("Recent project directory '%1' doesn't exist.").arg(QDir::toNativeSeparators(default_dir));
        logWarn(message);
        this->statusBar()->showMessage(message);
        return false;
    }

    logInfo(QString("Opening recent project: '%1'").arg(default_dir));
    return openProject(default_dir);
}

bool MainWindow::openProject(QString dir) {
    if (dir.isNull()) {
        projectOpenFailure = true;
        setWindowDisabled(true);
        return false;
    }

    QString nativeDir = QDir::toNativeSeparators(dir);

    this->statusBar()->showMessage(QString("Opening project %1").arg(nativeDir));

    userConfig.setProjectDir(dir);
    userConfig.load();
    projectConfig.setProjectDir(dir);
    projectConfig.load();

    this->closeSupplementaryWindows();
    this->newMapDefaultsSet = false;

    Scripting::init(this);
    bool already_open = isProjectOpen() && (editor->project->root == dir);
    if (!already_open) {
        editor->closeProject();
        editor->project = new Project(this);
        QObject::connect(editor->project, &Project::reloadProject, this, &MainWindow::on_action_Reload_Project_triggered);
        QObject::connect(editor->project, &Project::mapCacheCleared, this, &MainWindow::onMapCacheCleared);
        QObject::connect(editor->project, &Project::disableWildEncountersUI, [this]() { this->setWildEncountersUIEnabled(false); });
        QObject::connect(editor->project, &Project::uncheckMonitorFilesAction, [this]() {
            porymapConfig.setMonitorFiles(false);
            if (this->preferenceEditor)
                this->preferenceEditor->updateFields();
        });
        editor->project->set_root(dir);
    } else {
        editor->project->fileWatcher.removePaths(editor->project->fileWatcher.files());
        editor->project->clearMapCache();
        editor->project->clearTilesetCache();
    }

    this->projectOpenFailure = !(loadDataStructures()
                              && populateMapList()
                              && setInitialMap());

    if (this->projectOpenFailure) {
        this->statusBar()->showMessage(QString("Failed to open project %1").arg(nativeDir));
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening the project %1. Please see %2 for full error details.\n\n%3")
                .arg(dir)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Project", errorMsg);
        setWindowDisabled(true);
        return false;
    }
    
    showWindowTitle();
    this->statusBar()->showMessage(QString("Opened project %1").arg(nativeDir));

    porymapConfig.addRecentProject(dir);
    refreshRecentProjectsMenu();

    prefab.initPrefabUI(
                editor->metatile_selector_item,
                ui->scrollAreaWidgetContents_Prefabs,
                ui->label_prefabHelp,
                editor->map);
    Scripting::cb_ProjectOpened(dir);
    setWindowDisabled(false);
    return true;
}

bool MainWindow::isProjectOpen() {
    return !projectOpenFailure && editor && editor->project;
}

bool MainWindow::setInitialMap() {
    QList<QStringList> names;
    if (editor && editor->project)
        names = editor->project->groupedMapNames;

    QString recentMap = userConfig.getRecentMap();
    if (!recentMap.isEmpty()) {
        // Make sure the recent map is still in the map list
        for (int i = 0; i < names.length(); i++) {
            if (names.value(i).contains(recentMap)) {
                return setMap(recentMap, true);
            }
        }
    }

    // Failing that, just get the first map in the list.
    for (int i = 0; i < names.length(); i++) {
        QStringList list = names.value(i);
        if (list.length()) {
            return setMap(list.value(0), true);
        }
    }

    logError("Failed to load any map names.");
    return false;
}

void MainWindow::refreshRecentProjectsMenu() {
    ui->menuOpen_Recent_Project->clear();
    QStringList recentProjects = porymapConfig.getRecentProjects();

    if (isProjectOpen()) {
        // Don't show the currently open project in this menu
        recentProjects.removeOne(this->editor->project->root);
    }

    // Add project paths to menu. Arbitrary limit of 10 items.
    const int numItems = qMin(10, recentProjects.length());
    for (int i = 0; i < numItems; i++) {
        const QString path = recentProjects.at(i);
        ui->menuOpen_Recent_Project->addAction(path, [this, path](){
           this->openProject(path);
        });
    }

    // Add action to clear list of paths
    if (!recentProjects.isEmpty()) ui->menuOpen_Recent_Project->addSeparator();
    QAction *clearAction = ui->menuOpen_Recent_Project->addAction("Clear Items", [this](){
        QStringList paths = QStringList();
        if (isProjectOpen())
            paths.append(this->editor->project->root);
        porymapConfig.setRecentProjects(paths);
        this->refreshRecentProjectsMenu();
    });
    clearAction->setEnabled(!recentProjects.isEmpty());
}

void MainWindow::openSubWindow(QWidget * window) {
    if (!window) return;

    if (!window->isVisible()) {
        window->show();
    } else if (window->isMinimized()) {
        window->showNormal();
    } else {
        window->raise();
        window->activateWindow();
    }
}

QString MainWindow::getExistingDirectory(QString dir) {
    return QFileDialog::getExistingDirectory(this, "Open Directory", dir, QFileDialog::ShowDirsOnly);
}

void MainWindow::on_action_Open_Project_triggered()
{
    QString recent = ".";
    if (!userConfig.getRecentMap().isEmpty()) {
        recent = userConfig.getRecentMap();
    }
    QString dir = getExistingDirectory(recent);
    if (!dir.isEmpty()) {
        if (this->editor && this->editor->project) {
            Scripting::cb_ProjectClosed(this->editor->project->root);
            this->ui->graphicsView_Map->clearOverlayMap();
        }
        openProject(dir);
    }
}

void MainWindow::on_action_Reload_Project_triggered() {
    // TODO: when undo history is complete show only if has unsaved changes
    QMessageBox warning(this);
    warning.setText("WARNING");
    warning.setInformativeText("Reloading this project will discard any unsaved changes.");
    warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    warning.setDefaultButton(QMessageBox::Cancel);
    warning.setIcon(QMessageBox::Warning);

    if (warning.exec() == QMessageBox::Ok)
        openProject(editor->project->root);
}

bool MainWindow::setMap(QString map_name, bool scrollTreeView) {
    logInfo(QString("Setting map to '%1'").arg(map_name));
    if (map_name.isEmpty()) {
        return false;
    }

    if (!editor || !editor->setMap(map_name)) {
        logWarn(QString("Failed to set map to '%1'").arg(map_name));
        return false;
    }

    if (editor->map != nullptr && !editor->map->name.isNull()) {
        ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(editor->map->name)), false);
    }

    this->lastSelectedEvent.clear();
    refreshMapScene();
    displayMapProperties();

    if (scrollTreeView) {
        // Make sure we clear the filter first so we actually have a scroll target
        mapListProxyModel->setFilterRegularExpression(QString());
        ui->mapList->setCurrentIndex(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)));
        ui->mapList->scrollTo(ui->mapList->currentIndex(), QAbstractItemView::PositionAtCenter);
    }

    ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)), true);

    showWindowTitle();

    connect(editor->map, &Map::mapChanged, this, &MainWindow::onMapChanged);
    connect(editor->map, &Map::mapNeedsRedrawing, this, &MainWindow::onMapNeedsRedrawing);
    connect(editor->map, &Map::modified, [this](){ this->markMapEdited(); });

    setRecentMap(map_name);
    updateMapList();

    Scripting::cb_MapOpened(map_name);
    prefab.updatePrefabUi(editor->map);
    updateTilesetEditor();
    return true;
}

void MainWindow::redrawMapScene()
{
    if (!editor->displayMap())
        return;

    this->refreshMapScene();
}

void MainWindow::refreshMapScene()
{
    on_mainTabBar_tabBarClicked(ui->mainTabBar->currentIndex());

    ui->graphicsView_Map->setScene(editor->scene);
    ui->graphicsView_Map->setSceneRect(editor->scene->sceneRect());
    ui->graphicsView_Map->editor = editor;

    ui->graphicsView_Connections->setScene(editor->scene);
    ui->graphicsView_Connections->setSceneRect(editor->scene->sceneRect());

    ui->graphicsView_Metatiles->setScene(editor->scene_metatiles);
    //ui->graphicsView_Metatiles->setSceneRect(editor->scene_metatiles->sceneRect());
    ui->graphicsView_Metatiles->setFixedSize(editor->metatile_selector_item->pixmap().width() + 2, editor->metatile_selector_item->pixmap().height() + 2);

    ui->graphicsView_BorderMetatile->setScene(editor->scene_selected_border_metatiles);
    ui->graphicsView_BorderMetatile->setFixedSize(editor->selected_border_metatiles_item->pixmap().width() + 2, editor->selected_border_metatiles_item->pixmap().height() + 2);

    ui->graphicsView_currentMetatileSelection->setScene(editor->scene_current_metatile_selection);
    ui->graphicsView_currentMetatileSelection->setFixedSize(editor->current_metatile_selection_item->pixmap().width() + 2, editor->current_metatile_selection_item->pixmap().height() + 2);

    ui->graphicsView_Collision->setScene(editor->scene_collision_metatiles);
    //ui->graphicsView_Collision->setSceneRect(editor->scene_collision_metatiles->sceneRect());
    ui->graphicsView_Collision->setFixedSize(editor->movement_permissions_selector_item->pixmap().width() + 2, editor->movement_permissions_selector_item->pixmap().height() + 2);

    on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
    on_horizontalSlider_CollisionZoom_valueChanged(ui->horizontalSlider_CollisionZoom->value());
}

void MainWindow::openWarpMap(QString map_name, int event_id, Event::Group event_group) {
    // Can't warp to dynamic maps
    if (map_name == DYNAMIC_MAP_NAME)
        return;

    // Ensure valid destination map name.
    if (!editor->project->mapNames.contains(map_name)) {
        logError(QString("Invalid map name '%1'").arg(map_name));
        return;
    }

    // Open the destination map.
    if (!setMap(map_name, true)) {
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening map %1. Please see %2 for full error details.\n\n%3")
                .arg(map_name)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Map", errorMsg);
        return;
    }

    // Select the target event.
    int index = event_id - Event::getIndexOffset(event_group);
    QList<Event*> events = editor->map->events[event_group];
    if (index < events.length() && index >= 0) {
        Event *event = events.at(index);
        for (DraggablePixmapItem *item : editor->getObjects()) {
            if (item->event == event) {
                editor->selected_events->clear();
                editor->selected_events->append(item);
                editor->updateSelectedEvents();
            }
        }
    } else {
        // Can still warp to this map, but can't select the specified event
        logWarn(QString("%1 %2 doesn't exist on map '%3'").arg(Event::eventGroupToString(event_group)).arg(event_id).arg(map_name));
    }
}

void MainWindow::setRecentMap(QString mapName) {
    userConfig.setRecentMap(mapName);
}

void MainWindow::displayMapProperties() {
    // Block signals to the comboboxes while they are being modified
    const QSignalBlocker blocker1(ui->comboBox_Song);
    const QSignalBlocker blocker2(ui->comboBox_Location);
    const QSignalBlocker blocker3(ui->comboBox_PrimaryTileset);
    const QSignalBlocker blocker4(ui->comboBox_SecondaryTileset);
    const QSignalBlocker blocker5(ui->comboBox_Weather);
    const QSignalBlocker blocker6(ui->comboBox_BattleScene);
    const QSignalBlocker blocker7(ui->comboBox_Type);
    const QSignalBlocker blocker8(ui->checkBox_Visibility);
    const QSignalBlocker blocker9(ui->checkBox_ShowLocation);
    const QSignalBlocker blockerA(ui->checkBox_AllowRunning);
    const QSignalBlocker blockerB(ui->checkBox_AllowBiking);
    const QSignalBlocker blockerC(ui->spinBox_FloorNumber);
    const QSignalBlocker blockerD(ui->checkBox_AllowEscaping);

    ui->checkBox_Visibility->setChecked(false);
    ui->checkBox_ShowLocation->setChecked(false);
    ui->checkBox_AllowRunning->setChecked(false);
    ui->checkBox_AllowBiking->setChecked(false);
    ui->checkBox_AllowEscaping->setChecked(false);
    if (!editor || !editor->map || !editor->project) {
        ui->frame_3->setEnabled(false);
        return;
    }

    ui->frame_3->setEnabled(true);
    Map *map = editor->map;

    ui->comboBox_PrimaryTileset->blockSignals(true);
    ui->comboBox_SecondaryTileset->blockSignals(true);
    ui->comboBox_PrimaryTileset->setCurrentText(map->layout->tileset_primary_label);
    ui->comboBox_SecondaryTileset->setCurrentText(map->layout->tileset_secondary_label);
    ui->comboBox_PrimaryTileset->blockSignals(false);
    ui->comboBox_SecondaryTileset->blockSignals(false);

    ui->comboBox_Song->setCurrentText(map->song);
    ui->comboBox_Location->setCurrentText(map->location);
    ui->checkBox_Visibility->setChecked(map->requiresFlash);
    ui->comboBox_Weather->setCurrentText(map->weather);
    ui->comboBox_Type->setCurrentText(map->type);
    ui->comboBox_BattleScene->setCurrentText(map->battle_scene);
    ui->checkBox_ShowLocation->setChecked(map->show_location);
    ui->checkBox_AllowRunning->setChecked(map->allowRunning);
    ui->checkBox_AllowBiking->setChecked(map->allowBiking);
    ui->checkBox_AllowEscaping->setChecked(map->allowEscaping);
    ui->spinBox_FloorNumber->setValue(map->floorNumber);

    // Custom fields table.
    ui->tableWidget_CustomHeaderFields->blockSignals(true);
    ui->tableWidget_CustomHeaderFields->setRowCount(0);
    for (auto it = map->customHeaders.begin(); it != map->customHeaders.end(); it++)
        CustomAttributesTable::addAttribute(ui->tableWidget_CustomHeaderFields, it.key(), it.value());
    ui->tableWidget_CustomHeaderFields->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_CustomHeaderFields->blockSignals(false);
}

void MainWindow::on_comboBox_Song_currentTextChanged(const QString &song)
{
    if (editor && editor->map) {
        editor->map->song = song;
        markMapEdited();
    }
}

void MainWindow::on_comboBox_Location_currentTextChanged(const QString &location)
{
    if (editor && editor->map) {
        editor->map->location = location;
        markMapEdited();
    }
}

void MainWindow::on_comboBox_Weather_currentTextChanged(const QString &weather)
{
    if (editor && editor->map) {
        editor->map->weather = weather;
        markMapEdited();
    }
}

void MainWindow::on_comboBox_Type_currentTextChanged(const QString &type)
{
    if (editor && editor->map) {
        editor->map->type = type;
        markMapEdited();
    }
}

void MainWindow::on_comboBox_BattleScene_currentTextChanged(const QString &battle_scene)
{
    if (editor && editor->map) {
        editor->map->battle_scene = battle_scene;
        markMapEdited();
    }
}

void MainWindow::on_checkBox_Visibility_stateChanged(int selected)
{
    if (editor && editor->map) {
        editor->map->requiresFlash = (selected == Qt::Checked);
        markMapEdited();
    }
}

void MainWindow::on_checkBox_ShowLocation_stateChanged(int selected)
{
    if (editor && editor->map) {
        editor->map->show_location = (selected == Qt::Checked);
        markMapEdited();
    }
}

void MainWindow::on_checkBox_AllowRunning_stateChanged(int selected)
{
    if (editor && editor->map) {
        editor->map->allowRunning = (selected == Qt::Checked);
        markMapEdited();
    }
}

void MainWindow::on_checkBox_AllowBiking_stateChanged(int selected)
{
    if (editor && editor->map) {
        editor->map->allowBiking = (selected == Qt::Checked);
        markMapEdited();
    }
}

void MainWindow::on_checkBox_AllowEscaping_stateChanged(int selected)
{
    if (editor && editor->map) {
        editor->map->allowEscaping = (selected == Qt::Checked);
        markMapEdited();
    }
}

void MainWindow::on_spinBox_FloorNumber_valueChanged(int offset)
{
    if (editor && editor->map) {
        editor->map->floorNumber = offset;
        markMapEdited();
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
                && project->readCoordEventWeatherNames()
                && project->readSecretBaseIds() 
                && project->readBgEventFacingDirections()
                && project->readTrainerTypes()
                && project->readMetatileBehaviors()
                && project->readFieldmapProperties()
                && project->readFieldmapMasks()
                && project->readTilesetLabels()
                && project->readTilesetMetatileLabels()
                && project->readHealLocations()
                && project->readMiscellaneousConstants()
                && project->readSpeciesIconPaths()
                && project->readWildMonData()
                && project->readEventScriptLabels()
                && project->readObjEventGfxConstants()
                && project->readEventGraphics()
                && project->readSongNames();

    project->applyParsedLimits();
    setProjectSpecificUI();
    Scripting::populateGlobalObject(this);

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
    ui->comboBox_Song->addItems(project->songNames);
    ui->comboBox_Location->clear();
    ui->comboBox_Location->addItems(project->mapSectionValueToName.values());
    ui->comboBox_PrimaryTileset->clear();
    ui->comboBox_PrimaryTileset->addItems(project->primaryTilesetLabels);
    ui->comboBox_SecondaryTileset->clear();
    ui->comboBox_SecondaryTileset->addItems(project->secondaryTilesetLabels);
    ui->comboBox_Weather->clear();
    ui->comboBox_Weather->addItems(project->weatherNames);
    ui->comboBox_BattleScene->clear();
    ui->comboBox_BattleScene->addItems(project->mapBattleScenes);
    ui->comboBox_Type->clear();
    ui->comboBox_Type->addItems(project->mapTypes);

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
            for (int i = 0; i < project->groupNames.length(); i++) {
                QString group_name = project->groupNames.value(i);
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
            for (int i = 0; i < project->groupNames.length(); i++) {
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
            for (int i = 0; i < project->groupNames.length(); i++) {
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
        connect(actions, &QActionGroup::triggered, this, &MainWindow::onAddNewMapToGroupClick);
        menu->exec(QCursor::pos());
    } else if (itemType == "map_sec") {
        QString secName = selectedItem->data(Qt::UserRole).toString();
        QMenu* menu = new QMenu(this);
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map to Area"))->setData(secName);
        connect(actions, &QActionGroup::triggered, this, &MainWindow::onAddNewMapToAreaClick);
        menu->exec(QCursor::pos());
    } else if (itemType == "map_layout") {
        QString layoutId = selectedItem->data(MapListUserRoles::TypeRole2).toString();
        QMenu* menu = new QMenu(this);
        QActionGroup* actions = new QActionGroup(menu);
        actions->addAction(menu->addAction("Add New Map with Layout"))->setData(layoutId);
        connect(actions, &QActionGroup::triggered, this, &MainWindow::onAddNewMapToLayoutClick);
        menu->exec(QCursor::pos());
    }
}

void MainWindow::onAddNewMapToGroupClick(QAction* triggeredAction)
{
    openNewMapPopupWindow();
    this->newMapPrompt->init(MapSortOrder::Group, triggeredAction->data());
}

void MainWindow::onAddNewMapToAreaClick(QAction* triggeredAction)
{
    openNewMapPopupWindow();
    this->newMapPrompt->init(MapSortOrder::Area, triggeredAction->data());
}

void MainWindow::onAddNewMapToLayoutClick(QAction* triggeredAction)
{
    openNewMapPopupWindow();
    this->newMapPrompt->init(MapSortOrder::Layout, triggeredAction->data());
}

void MainWindow::onNewMapCreated() {
    QString newMapName = this->newMapPrompt->map->name;
    int newMapGroup = this->newMapPrompt->group;
    Map *newMap = this->newMapPrompt->map;
    bool existingLayout = this->newMapPrompt->existingLayout;
    bool importedMap = this->newMapPrompt->importedMap;

    newMap = editor->project->addNewMapToGroup(newMapName, newMapGroup, newMap, existingLayout, importedMap);

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

    if (newMap->needsHealLocation) {
        addNewEvent(Event::Type::HealLocation);
        editor->project->saveHealLocations(newMap);
        editor->save();
    }

    disconnect(this->newMapPrompt, &NewMapPopup::applied, this, &MainWindow::onNewMapCreated);
    delete newMap;
}

void MainWindow::openNewMapPopupWindow() {
    if (!this->newMapDefaultsSet) {
        NewMapPopup::setDefaultSettings(this->editor->project);
        this->newMapDefaultsSet = true;
    }
    if (!this->newMapPrompt) {
        this->newMapPrompt = new NewMapPopup(this, this->editor->project);
    }

    openSubWindow(this->newMapPrompt);

    connect(this->newMapPrompt, &NewMapPopup::applied, this, &MainWindow::onNewMapCreated);
    this->newMapPrompt->setAttribute(Qt::WA_DeleteOnClose);
}

void MainWindow::on_action_NewMap_triggered() {
    openNewMapPopupWindow();
    this->newMapPrompt->init();
}

// Insert label for newly-created tileset into sorted list of existing labels
int MainWindow::insertTilesetLabel(QStringList * list, QString label) {
    int i = 0;
    for (; i < list->length(); i++)
        if (list->at(i) > label) break;
    list->insert(i, label);
    return i;
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
        QString fullDirectoryPath = editor->project->root + "/" + createTilesetDialog->path;
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
        if (editor->project->tilesetLabelsOrdered.contains(createTilesetDialog->fullSymbolName)) {
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
        Tileset newSet;
        newSet.name = createTilesetDialog->fullSymbolName;
        newSet.tilesImagePath = fullDirectoryPath + "/tiles.png";
        newSet.metatiles_path = fullDirectoryPath + "/metatiles.bin";
        newSet.metatile_attrs_path = fullDirectoryPath + "/metatile_attributes.bin";
        newSet.is_secondary = createTilesetDialog->isSecondary;
        int numMetatiles = createTilesetDialog->isSecondary ? (Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary()) : Project::getNumMetatilesPrimary();
        QImage tilesImage(":/images/blank_tileset.png");
        editor->project->loadTilesetTiles(&newSet, tilesImage);
        int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
        for(int i = 0; i < numMetatiles; ++i) {
            Metatile *mt = new Metatile();
            for(int j = 0; j < tilesPerMetatile; ++j){
                Tile tile = Tile();
                if (createTilesetDialog->checkerboardFill) {
                    // Create a checkerboard-style dummy tileset
                    if (((i / 8) % 2) == 0)
                        tile.tileId = ((i % 2) == 0) ? 1 : 2;
                    else
                        tile.tileId = ((i % 2) == 1) ? 1 : 2;
                }
                mt->tiles.append(tile);
            }
            newSet.metatiles.append(mt);
        }
        for(int i = 0; i < 16; ++i) {
            QList<QRgb> currentPal;
            for(int i = 0; i < 16;++i) {
                currentPal.append(qRgb(0,0,0));
            }
            newSet.palettes.append(currentPal);
            newSet.palettePreviews.append(currentPal);
            QString fileName = QString("%1.pal").arg(i, 2, 10, QLatin1Char('0'));
            newSet.palettePaths.append(fullDirectoryPath+"/palettes/" + fileName);
        }
        newSet.palettes[0][1] = qRgb(255,0,255);
        newSet.palettePreviews[0][1] = qRgb(255,0,255);
        exportIndexed4BPPPng(newSet.tilesImage, newSet.tilesImagePath);
        editor->project->saveTilesetMetatiles(&newSet);
        editor->project->saveTilesetMetatileAttributes(&newSet);
        editor->project->saveTilesetPalettes(&newSet);

        //append to tileset specific files
        newSet.appendToHeaders(editor->project->root, createTilesetDialog->friendlyName, editor->project->usingAsmTilesets);
        newSet.appendToGraphics(editor->project->root, createTilesetDialog->friendlyName, editor->project->usingAsmTilesets);
        newSet.appendToMetatiles(editor->project->root, createTilesetDialog->friendlyName, editor->project->usingAsmTilesets);

        if (!createTilesetDialog->isSecondary) {
            int index = insertTilesetLabel(&editor->project->primaryTilesetLabels, createTilesetDialog->fullSymbolName);
            this->ui->comboBox_PrimaryTileset->insertItem(index, createTilesetDialog->fullSymbolName);
        } else {
            int index = insertTilesetLabel(&editor->project->secondaryTilesetLabels, createTilesetDialog->fullSymbolName);
            this->ui->comboBox_SecondaryTileset->insertItem(index, createTilesetDialog->fullSymbolName);
        }
        insertTilesetLabel(&editor->project->tilesetLabelsOrdered, createTilesetDialog->fullSymbolName);

        QMessageBox msgBox(this);
        msgBox.setText("Successfully created tileset.");
        QString message = QString("Tileset \"%1\" was created successfully.").arg(createTilesetDialog->friendlyName);
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Information);
        msgBox.exec();
    }
}

void MainWindow::updateTilesetEditor() {
    if (this->tilesetEditor) {
        this->tilesetEditor->update(
            this->editor->map,
            editor->ui->comboBox_PrimaryTileset->currentText(),
            editor->ui->comboBox_SecondaryTileset->currentText()
        );
    }
}

void MainWindow::redrawMetatileSelection()
{
    double scale = pow(3.0, static_cast<double>(porymapConfig.getMetatilesZoom() - 30) / 30.0);
    QTransform transform;
    transform.scale(scale, scale);

    ui->graphicsView_currentMetatileSelection->setTransform(transform);
    ui->graphicsView_currentMetatileSelection->setFixedSize(editor->current_metatile_selection_item->pixmap().width() * scale + 2, editor->current_metatile_selection_item->pixmap().height() * scale + 2);

    QPoint size = editor->metatile_selector_item->getSelectionDimensions();
    if (size.x() == 1 && size.y() == 1) {
        MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
        QPoint pos = editor->metatile_selector_item->getMetatileIdCoordsOnWidget(selection.metatileItems.first().metatileId);
        pos *= scale;
        ui->scrollArea_2->ensureVisible(pos.x(), pos.y(), 8 * scale, 8 * scale);
    }
}

void MainWindow::currentMetatilesSelectionChanged()
{
    redrawMetatileSelection();
    if (this->tilesetEditor) {
        MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
        this->tilesetEditor->selectMetatile(selection.metatileItems.first().metatileId);
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
                if (editor->project && editor->project->mapCache.contains(map_name)) {
                    QStandardItem *map = mapListModel->itemFromIndex(mapListIndexes.value(map_name));
                    map->setIcon(*mapIcon);
                    if (editor->project->mapCache.value(map_name)->hasUnsavedChanges()) {
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

void MainWindow::on_action_Save_Project_triggered() {
    editor->saveProject();
    updateMapList();
    showWindowTitle();
}

void MainWindow::duplicate() {
    editor->duplicateSelectedEvents();
}

void MainWindow::copy() {
    auto focused = QApplication::focusWidget();
    if (focused) {
        QString objectName = focused->objectName();
        if (objectName == "graphicsView_currentMetatileSelection") {
            // copy the current metatile selection as json data
            OrderedJson::object copyObject;
            copyObject["object"] = "metatile_selection";
            OrderedJson::array metatiles;
            MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
            for (auto item : selection.metatileItems) {
                metatiles.append(static_cast<int>(item.metatileId));
            }
            OrderedJson::array collisions;
            if (selection.hasCollision) {
                for (auto item : selection.collisionItems) {
                    OrderedJson::object collision;
                    collision["collision"] = item.collision;
                    collision["elevation"] = item.elevation;
                    collisions.append(collision);
                }
            }
            if (collisions.length() != metatiles.length()) {
                // fill in collisions
                collisions.clear();
                for (int i = 0; i < metatiles.length(); i++) {
                    OrderedJson::object collision;
                    collision["collision"] = projectConfig.getDefaultCollision();
                    collision["elevation"] = projectConfig.getDefaultElevation();
                    collisions.append(collision);
                }
            }
            copyObject["metatile_selection"] = metatiles;
            copyObject["collision_selection"] = collisions;
            copyObject["width"] = editor->metatile_selector_item->getSelectionDimensions().x();
            copyObject["height"] = editor->metatile_selector_item->getSelectionDimensions().y();
            setClipboardData(copyObject);
            logInfo("Copied metatile selection to clipboard");
        }
        else if (objectName == "graphicsView_Map") {
            // which tab are we in?
            switch (ui->mainTabBar->currentIndex())
            {
            default:
                break;
            case 0:
            {
                // copy the map image
                QPixmap pixmap = editor->map ? editor->map->render(true) : QPixmap();
                setClipboardData(pixmap.toImage());
                logInfo("Copied current map image to clipboard");
                break;
            }
            case 1:
            {
                if (!editor || !editor->project) break;

                // copy the currently selected event(s) as a json object
                OrderedJson::object copyObject;
                copyObject["object"] = "events";

                QList<DraggablePixmapItem *> events;
                if (editor->selected_events && editor->selected_events->length()) {
                    events = *editor->selected_events;
                }

                OrderedJson::array eventsArray;

                for (auto item : events) {
                    Event *event = item->event;

                    if (event->getEventType() == Event::Type::HealLocation) {
                        // no copy on heal locations
                        logWarn(QString("Copying heal location events is not allowed."));
                        continue;
                    }

                    OrderedJson::object eventContainer;
                    eventContainer["event_type"] = Event::eventTypeToString(event->getEventType());
                    OrderedJson::object eventJson = event->buildEventJson(editor->project);
                    eventContainer["event"] = eventJson;
                    eventsArray.append(eventContainer);
                }

                if (!eventsArray.isEmpty()) {
                    copyObject["events"] = eventsArray;
                    setClipboardData(copyObject);
                    logInfo("Copied currently selected events to clipboard");
                }
                break;
            }
            }
        }
        else if (this->ui->mainTabBar->currentIndex() == 4) {
            QWidget *w = this->ui->stackedWidget_WildMons->currentWidget();
            if (w) {
                MonTabWidget *mtw = static_cast<MonTabWidget *>(w);
                mtw->copy(mtw->currentIndex());
            }
        }
    }
}

void MainWindow::setClipboardData(OrderedJson::object object) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString newText;
    int indent = 0;
    object["application"] = "porymap";
    OrderedJson data(object);
    data.dump(newText, &indent);
    clipboard->setText(newText);
}

void MainWindow::setClipboardData(QImage image) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
}

void MainWindow::paste() {
    if (!editor || !editor->project || !editor->map) return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    QString clipboardText(clipboard->text());

    if (ui->mainTabBar->currentIndex() == 4) {
        QWidget *w = this->ui->stackedWidget_WildMons->currentWidget();
        if (w) {
            w->setFocus();
            MonTabWidget *mtw = static_cast<MonTabWidget *>(w);
            mtw->paste(mtw->currentIndex());
        }
    }
    else if (!clipboardText.isEmpty()) {
        // we only can paste json text
        // so, check if clipboard text is valid json
        QString parseError;
        QJsonDocument pasteJsonDoc = QJsonDocument::fromJson(clipboardText.toUtf8());

        // test empty
        QJsonObject pasteObject = pasteJsonDoc.object();

        //OrderedJson::object pasteObject = pasteJson.object_items();
        if (pasteObject["application"].toString() != "porymap") {
            return;
        }

        logInfo("Attempting to paste from JSON in clipboard");

        switch (ui->mainTabBar->currentIndex())
            {
            default:
                break;
            case 0:
            {
                // can only paste currently selected metatiles on this tab
                if (pasteObject["object"].toString() != "metatile_selection") {
                    return;
                }
                QJsonArray metatilesArray = pasteObject["metatile_selection"].toArray();
                QJsonArray collisionsArray = pasteObject["collision_selection"].toArray();
                int width = ParseUtil::jsonToInt(pasteObject["width"]);
                int height = ParseUtil::jsonToInt(pasteObject["height"]);
                QList<uint16_t> metatiles;
                QList<QPair<uint16_t, uint16_t>> collisions;
                for (auto tile : metatilesArray) {
                    metatiles.append(static_cast<uint16_t>(tile.toInt()));
                }
                for (QJsonValue collision : collisionsArray) {
                    collisions.append({static_cast<uint16_t>(collision["collision"].toInt()), static_cast<uint16_t>(collision["elevation"].toInt())});
                }
                editor->metatile_selector_item->setExternalSelection(width, height, metatiles, collisions);
                break;
            }
            case 1:
            {
                // can only paste events to this tab
                if (pasteObject["object"].toString() != "events") {
                    return;
                }

                QList<Event *> newEvents;

                QJsonArray events = pasteObject["events"].toArray();
                for (QJsonValue event : events) {
                    // paste the event to the map
                    Event *pasteEvent = nullptr;

                    Event::Type type = Event::eventTypeFromString(event["event_type"].toString());

                    if (this->editor->eventLimitReached(type)) {
                        logWarn(QString("Cannot paste event, the limit for type '%1' has been reached.").arg(event["event_type"].toString()));
                        break;
                    }

                    switch (type) {
                    case Event::Type::Object:
                        pasteEvent = new ObjectEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::CloneObject:
                        pasteEvent = new CloneObjectEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::Warp:
                        pasteEvent = new WarpEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::Trigger:
                        pasteEvent = new TriggerEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::WeatherTrigger:
                        pasteEvent = new WeatherTriggerEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::Sign:
                        pasteEvent = new SignEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::HiddenItem:
                        pasteEvent = new HiddenItemEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    case Event::Type::SecretBase:
                        pasteEvent = new SecretBaseEvent();
                        pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                        break;
                    default:
                        break;
                    }

                    if (pasteEvent) {
                        pasteEvent->setMap(this->editor->map);
                        newEvents.append(pasteEvent);
                    }
                }

                if (!newEvents.empty()) {
                    editor->map->editHistory.push(new EventPaste(this->editor, editor->map, newEvents));
                    updateObjects();
                }

                break;
            }
        }
    }
}

void MainWindow::on_action_Save_triggered() {
    editor->save();
    updateMapList();
    showWindowTitle();
}

void MainWindow::on_mapViewTab_tabBarClicked(int index)
{
    int oldIndex = ui->mapViewTab->currentIndex();
    ui->mapViewTab->setCurrentIndex(index);
    if (index != oldIndex)
        Scripting::cb_MapViewTabChanged(oldIndex, index);

    if (index == 0) {
        editor->setEditingMap();
    } else if (index == 1) {
        editor->setEditingCollision();
    } else if (index == 2) {
        editor->setEditingMap();
        if (projectConfig.getPrefabFilepath().isEmpty() && !projectConfig.getPrefabImportPrompted()) {
            // User hasn't set up prefabs and hasn't been prompted before.
            // Ask if they'd like to import the default prefabs file.
            if (prefab.tryImportDefaultPrefabs(this, projectConfig.getBaseGameVersion()))
                prefab.updatePrefabUi(this->editor->map);
        } 
    }
    editor->setCursorRectVisible(false);
}

void MainWindow::on_action_Exit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_mainTabBar_tabBarClicked(int index)
{
    int oldIndex = ui->mainTabBar->currentIndex();
    ui->mainTabBar->setCurrentIndex(index);
    if (index != oldIndex)
        Scripting::cb_MainTabChanged(oldIndex, index);

    int tabIndexToStackIndex[5] = {0, 0, 1, 2, 3};
    ui->mainStackedWidget->setCurrentIndex(tabIndexToStackIndex[index]);

    if (index == 0) {
        ui->stackedWidget_MapEvents->setCurrentIndex(0);
        on_mapViewTab_tabBarClicked(ui->mapViewTab->currentIndex());
        clickToolButtonFromEditMode(editor->map_edit_mode);
    } else if (index == 1) {
        ui->stackedWidget_MapEvents->setCurrentIndex(1);
        editor->setEditingObjects();
        clickToolButtonFromEditMode(editor->obj_edit_mode);
    } else if (index == 3) {
        editor->setEditingConnections();
    }
    if (index != 4) {
        if (userConfig.getEncounterJsonActive())
            editor->saveEncounterTabData();
    }
    if (index != 1) {
        editor->map_ruler->setEnabled(false);
    }
}

void MainWindow::on_actionZoom_In_triggered() {
    editor->scaleMapView(1);
}

void MainWindow::on_actionZoom_Out_triggered() {
    editor->scaleMapView(-1);
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
    if ((this->editor->map_item && this->editor->map_item->has_mouse)
     || (this->editor->collision_item && this->editor->collision_item->has_mouse)) {
        this->editor->playerViewRect->setVisible(enabled);
        ui->graphicsView_Map->scene()->update();
    }
}

void MainWindow::on_actionCursor_Tile_Outline_triggered()
{
    bool enabled = ui->actionCursor_Tile_Outline->isChecked();
    porymapConfig.setShowCursorTile(enabled);
    this->editor->settings->cursorTileRectEnabled = enabled;
    if ((this->editor->map_item && this->editor->map_item->has_mouse)
     || (this->editor->collision_item && this->editor->collision_item->has_mouse)) {
        this->editor->cursorMapTileRect->setVisible(enabled && this->editor->cursorMapTileRect->getActive());
        ui->graphicsView_Map->scene()->update();
    }
}

void MainWindow::on_actionShortcuts_triggered()
{
    if (!shortcutsEditor)
        initShortcutsEditor();

    openSubWindow(shortcutsEditor);
}

void MainWindow::initShortcutsEditor() {
    shortcutsEditor = new ShortcutsEditor(this);
    connect(shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
            this, &MainWindow::applyUserShortcuts);

    connectSubEditorsToShortcutsEditor();

    shortcutsEditor->setShortcutableObjects(shortcutableObjects());
}

void MainWindow::connectSubEditorsToShortcutsEditor() {
    /* Initialize sub-editors so that their children are added to MainWindow's object tree and will
     * be returned by shortcutableObjects() to be passed to ShortcutsEditor. */
    if (!tilesetEditor)
        initTilesetEditor();
    connect(shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
            tilesetEditor, &TilesetEditor::applyUserShortcuts);

    if (!regionMapEditor)
        initRegionMapEditor(true);
    if (regionMapEditor)
        connect(shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
                regionMapEditor, &RegionMapEditor::applyUserShortcuts);

    if (!customScriptsEditor)
        initCustomScriptsEditor();
    connect(shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
            customScriptsEditor, &CustomScriptsEditor::applyUserShortcuts);
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

void MainWindow::resetMapViewScale() {
    editor->scaleMapView(0);
}

void MainWindow::addNewEvent(Event::Type type) {
    if (editor && editor->project) {
        DraggablePixmapItem *object = editor->addNewEvent(type);
        if (object) {
            auto halfSize = ui->graphicsView_Map->size() / 2;
            auto centerPos = ui->graphicsView_Map->mapToScene(halfSize.width(), halfSize.height());
            object->moveTo(Metatile::coordFromPixmapCoord(centerPos));
            updateObjects();
            editor->selectMapEvent(object, false);
        } else {
            QMessageBox msgBox(this);
            msgBox.setText("Failed to add new event");
            if (Event::typeToGroup(type) == Event::Group::Object) {
                msgBox.setInformativeText(QString("The limit for object events (%1) has been reached.\n\n"
                                                  "This limit can be adjusted with %2 in '%3'.")
                                          .arg(editor->project->getMaxObjectEvents())
                                          .arg(projectConfig.getIdentifier(ProjectIdentifier::define_obj_event_count))
                                          .arg(projectConfig.getFilePath(ProjectFilePath::constants_global)));
            }
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Icon::Warning);
            msgBox.exec();
        }
    }
}

void MainWindow::tryAddEventTab(QWidget * tab, Event::Group group) {
    if (editor->map->events.value(group).length())
        ui->tabWidget_EventType->addTab(tab, QString("%1s").arg(Event::eventGroupToString(group)));
}

void MainWindow::displayEventTabs() {
    const QSignalBlocker blocker(ui->tabWidget_EventType);

    ui->tabWidget_EventType->clear();
    tryAddEventTab(eventTabObjectWidget,   Event::Group::Object);
    tryAddEventTab(eventTabWarpWidget,     Event::Group::Warp);
    tryAddEventTab(eventTabTriggerWidget,  Event::Group::Coord);
    tryAddEventTab(eventTabBGWidget,       Event::Group::Bg);
    tryAddEventTab(eventTabHealspotWidget, Event::Group::Heal);
}

void MainWindow::updateObjects() {
    QList<DraggablePixmapItem *> all_objects = editor->getObjects();
    for (auto i = this->lastSelectedEvent.cbegin(), end = this->lastSelectedEvent.cend(); i != end; i++) {
        if (i.value() && !all_objects.contains(i.value()))
            this->lastSelectedEvent.insert(i.key(), nullptr);
    }
    displayEventTabs();
    updateSelectedObjects();
}

void MainWindow::updateSelectedObjects() {
    QList<DraggablePixmapItem *> all_events = editor->getObjects();
    QList<DraggablePixmapItem *> events;

    if (editor->selected_events && editor->selected_events->length()) {
        events = *editor->selected_events;
    }
    else {
        QList<Event *> all_events;
        if (editor->map) {
            all_events = editor->map->getAllEvents();
        }
        if (all_events.length()) {
            DraggablePixmapItem *selectedEvent = all_events.first()->getPixmapItem();
            if (selectedEvent) {
                editor->selected_events->append(selectedEvent);
                editor->redrawObject(selectedEvent);
                events.append(selectedEvent);
            }
        }
    }

    QScrollArea *scrollTarget = ui->scrollArea_Multiple;
    QWidget *target = ui->scrollAreaWidgetContents_Multiple;

    this->isProgrammaticEventTabChange = true;

    if (events.length() == 1) {
        // single selected event case
        Event *current = events[0]->event;
        Event::Group eventGroup = current->getEventGroup();
        int event_offs = Event::getIndexOffset(eventGroup);

        if (eventGroup != Event::Group::None)
            this->lastSelectedEvent.insert(eventGroup, current->getPixmapItem());

        switch (eventGroup) {
        case Event::Group::Object: {
            scrollTarget = ui->scrollArea_Objects;
            target = ui->scrollAreaWidgetContents_Objects;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Objects);

            QSignalBlocker b(this->ui->spinner_ObjectID);
            this->ui->spinner_ObjectID->setMinimum(event_offs);
            this->ui->spinner_ObjectID->setMaximum(current->getMap()->events.value(eventGroup).length() + event_offs - 1);
            this->ui->spinner_ObjectID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Warp: {
            scrollTarget = ui->scrollArea_Warps;
            target = ui->scrollAreaWidgetContents_Warps;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Warps);

            QSignalBlocker b(this->ui->spinner_WarpID);
            this->ui->spinner_WarpID->setMinimum(event_offs);
            this->ui->spinner_WarpID->setMaximum(current->getMap()->events.value(eventGroup).length() + event_offs - 1);
            this->ui->spinner_WarpID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Coord: {
            scrollTarget = ui->scrollArea_Triggers;
            target = ui->scrollAreaWidgetContents_Triggers;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Triggers);

            QSignalBlocker b(this->ui->spinner_TriggerID);
            this->ui->spinner_TriggerID->setMinimum(event_offs);
            this->ui->spinner_TriggerID->setMaximum(current->getMap()->events.value(eventGroup).length() + event_offs - 1);
            this->ui->spinner_TriggerID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Bg: {
            scrollTarget = ui->scrollArea_BGs;
            target = ui->scrollAreaWidgetContents_BGs;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_BGs);

            QSignalBlocker b(this->ui->spinner_BgID);
            this->ui->spinner_BgID->setMinimum(event_offs);
            this->ui->spinner_BgID->setMaximum(current->getMap()->events.value(eventGroup).length() + event_offs - 1);
            this->ui->spinner_BgID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Heal: {
            scrollTarget = ui->scrollArea_Healspots;
            target = ui->scrollAreaWidgetContents_Healspots;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Healspots);

            QSignalBlocker b(this->ui->spinner_HealID);
            this->ui->spinner_HealID->setMinimum(event_offs);
            this->ui->spinner_HealID->setMaximum(current->getMap()->events.value(eventGroup).length() + event_offs - 1);
            this->ui->spinner_HealID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        default:
            break;
        }
        ui->tabWidget_EventType->removeTab(ui->tabWidget_EventType->indexOf(ui->tab_Multiple));
    }
    else if (events.length() > 1) {
        ui->tabWidget_EventType->addTab(ui->tab_Multiple, "Multiple");
        ui->tabWidget_EventType->setCurrentWidget(ui->tab_Multiple);
    }

    this->isProgrammaticEventTabChange = false;

    QList<QFrame *> frames;
    for (DraggablePixmapItem *item : events) {
        Event *event = item->event;
        EventFrame *eventFrame = event->createEventFrame();
        eventFrame->populate(this->editor->project);
        eventFrame->initialize();
        eventFrame->connectSignals(this);
        frames.append(eventFrame);
    }

    if (target->layout() && target->children().length()) {
        for (QFrame *frame : target->findChildren<EventFrame *>()) {
            if (!frames.contains(frame))
                frame->hide();
        }
        delete target->layout();
    }

    if (!events.empty()) {
        QVBoxLayout *layout = new QVBoxLayout;
        target->setLayout(layout);
        scrollTarget->setWidgetResizable(true);
        scrollTarget->setWidget(target);

        for (QFrame *frame : frames) {
            layout->addWidget(frame);
        }
        layout->addStretch(1);
        // Show the frames after the vertical spacer is added to avoid visual jank
        // where the frame would stretch to the bottom of the layout.
        for (QFrame *frame : frames) {
            frame->show();
        }

        ui->label_NoEvents->hide();
        ui->tabWidget_EventType->show();
    }
    else {
        ui->tabWidget_EventType->hide();
        ui->label_NoEvents->show();
    }
}

Event::Group MainWindow::getEventGroupFromTabWidget(QWidget *tab)
{
    Event::Group ret = Event::Group::None;
    if (tab == eventTabObjectWidget)
    {
        ret = Event::Group::Object;
    }
    else if (tab == eventTabWarpWidget)
    {
        ret = Event::Group::Warp;
    }
    else if (tab == eventTabTriggerWidget)
    {
        ret = Event::Group::Coord;
    }
    else if (tab == eventTabBGWidget)
    {
        ret = Event::Group::Bg;
    }
    else if (tab == eventTabHealspotWidget)
    {
        ret = Event::Group::Heal;
    }
    return ret;
}

void MainWindow::eventTabChanged(int index) {
    if (editor->map) {
        Event::Group group = getEventGroupFromTabWidget(ui->tabWidget_EventType->widget(index));
        DraggablePixmapItem *selectedEvent = this->lastSelectedEvent.value(group, nullptr);

        switch (group) {
        case Event::Group::Object:
            ui->newEventToolButton->setDefaultAction(ui->newEventToolButton->newObjectAction);
            break;
        case Event::Group::Warp:
            ui->newEventToolButton->setDefaultAction(ui->newEventToolButton->newWarpAction);
            break;
        case Event::Group::Coord:
            ui->newEventToolButton->setDefaultAction(ui->newEventToolButton->newTriggerAction);
            break;
        case Event::Group::Bg:
            ui->newEventToolButton->setDefaultAction(ui->newEventToolButton->newSignAction);
            break;
        default:
            break;
        }

        if (!isProgrammaticEventTabChange) {
            if (!selectedEvent && editor->map->events.value(group).count()) {
                Event *event = editor->map->events.value(group).at(0);
                for (QGraphicsItem *child : editor->events_group->childItems()) {
                    DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
                    if (item->event == event) {
                        selectedEvent = item;
                        break;
                    }
                }
            }

            if (selectedEvent) editor->selectMapEvent(selectedEvent);
        }
    }

    isProgrammaticEventTabChange = false;
}

void MainWindow::on_horizontalSlider_CollisionTransparency_valueChanged(int value) {
    this->editor->collisionOpacity = static_cast<qreal>(value) / 100;
    porymapConfig.setCollisionOpacity(value);
    this->editor->collision_item->draw(true);
}

void MainWindow::on_toolButton_deleteObject_clicked() {
    if (ui->mainTabBar->currentIndex() != 1) {
        // do not delete an event when not on event tab
        return;
    }
    if (editor && editor->selected_events) {
        if (editor->selected_events->length()) {
            DraggablePixmapItem *nextSelectedEvent = nullptr;
            QList<Event *> selectedEvents;
            int numDeleted = 0;
            for (DraggablePixmapItem *item : *editor->selected_events) {
                Event::Group event_group = item->event->getEventGroup();
                if (event_group != Event::Group::Heal) {
                    numDeleted++;
                    item->event->setPixmapItem(item);
                    selectedEvents.append(item->event);
                }
                else { // don't allow deletion of heal locations
                    logWarn(QString("Cannot delete event of type '%1'").arg(Event::eventTypeToString(item->event->getEventType())));
                }
            }
            if (numDeleted) {
                // Get the index for the event that should be selected after this event has been deleted.
                // Select event at next smallest index when deleting a single event.
                // If deleting multiple events, just let editor work out next selected.
                if (numDeleted == 1) {
                    Event::Group event_group = selectedEvents[0]->getEventGroup();
                    int index = editor->map->events.value(event_group).indexOf(selectedEvents[0]);
                    if (index != editor->map->events.value(event_group).size() - 1)
                        index++;
                    else
                        index--;
                    Event *event = nullptr;
                    if (index >= 0)
                        event = editor->map->events.value(event_group).at(index);
                    for (QGraphicsItem *child : editor->events_group->childItems()) {
                        DraggablePixmapItem *event_item = static_cast<DraggablePixmapItem *>(child);
                        if (event_item->event == event) {
                            nextSelectedEvent = event_item;
                            break;
                        }
                    }
                }
                editor->map->editHistory.push(new EventDelete(editor, editor->map, selectedEvents, nextSelectedEvent ? nextSelectedEvent->event : nullptr));
            }
        }
    }
}

void MainWindow::on_toolButton_Paint_clicked()
{
    if (ui->mainTabBar->currentIndex() == 0)
        editor->map_edit_mode = "paint";
    else
        editor->obj_edit_mode = "paint";

    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pencil_cursor.ico"), 10, 10);

    // do not stop single tile mode when editing collision
    if (ui->mapViewTab->currentIndex() != 1)
        editor->cursorMapTileRect->stopSingleTileMode();

    ui->graphicsView_Map->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView_Map->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->graphicsView_Map);
    ui->graphicsView_Map->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::MinimalViewportUpdate);
    ui->graphicsView_Map->setFocus();

    checkToolButtons();
}

void MainWindow::on_toolButton_Select_clicked()
{
    if (ui->mainTabBar->currentIndex() == 0)
        editor->map_edit_mode = "select";
    else
        editor->obj_edit_mode = "select";

    editor->settings->mapCursor = QCursor();
    editor->cursorMapTileRect->setSingleTileMode();

    ui->graphicsView_Map->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView_Map->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->graphicsView_Map);
    ui->graphicsView_Map->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::MinimalViewportUpdate);
    ui->graphicsView_Map->setFocus();

    checkToolButtons();
}

void MainWindow::on_toolButton_Fill_clicked()
{
    if (ui->mainTabBar->currentIndex() == 0)
        editor->map_edit_mode = "fill";
    else
        editor->obj_edit_mode = "fill";

    editor->settings->mapCursor = QCursor(QPixmap(":/icons/fill_color_cursor.ico"), 10, 10);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->graphicsView_Map->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView_Map->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->graphicsView_Map);
    ui->graphicsView_Map->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::MinimalViewportUpdate);
    ui->graphicsView_Map->setFocus();

    checkToolButtons();
}

void MainWindow::on_toolButton_Dropper_clicked()
{
    if (ui->mainTabBar->currentIndex() == 0)
        editor->map_edit_mode = "pick";
    else
        editor->obj_edit_mode = "pick";

    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pipette_cursor.ico"), 10, 10);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->graphicsView_Map->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView_Map->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->graphicsView_Map);
    ui->graphicsView_Map->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::MinimalViewportUpdate);
    ui->graphicsView_Map->setFocus();

    checkToolButtons();
}

void MainWindow::on_toolButton_Move_clicked()
{
    if (ui->mainTabBar->currentIndex() == 0)
        editor->map_edit_mode = "move";
    else
        editor->obj_edit_mode = "move";

    editor->settings->mapCursor = QCursor(QPixmap(":/icons/move.ico"), 7, 7);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->graphicsView_Map->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView_Map->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(ui->graphicsView_Map, QScroller::LeftMouseButtonGesture);
    ui->graphicsView_Map->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::FullViewportUpdate);
    ui->graphicsView_Map->setFocus();

    checkToolButtons();
}

void MainWindow::on_toolButton_Shift_clicked()
{
    if (ui->mainTabBar->currentIndex() == 0)
        editor->map_edit_mode = "shift";
    else
        editor->obj_edit_mode = "shift";

    editor->settings->mapCursor = QCursor(QPixmap(":/icons/shift_cursor.ico"), 10, 10);
    editor->cursorMapTileRect->setSingleTileMode();

    ui->graphicsView_Map->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->graphicsView_Map->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QScroller::ungrabGesture(ui->graphicsView_Map);
    ui->graphicsView_Map->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::MinimalViewportUpdate);
    ui->graphicsView_Map->setFocus();

    checkToolButtons();
}

void MainWindow::checkToolButtons() {
    QString edit_mode;
    if (ui->mainTabBar->currentIndex() == 0) {
        edit_mode = editor->map_edit_mode;
    } else {
        edit_mode = editor->obj_edit_mode;
        if (edit_mode == "select" && editor->map_ruler)
            editor->map_ruler->setEnabled(true);
        else if (editor->map_ruler)
            editor->map_ruler->setEnabled(false);
    }

    ui->toolButton_Paint->setChecked(edit_mode == "paint");
    ui->toolButton_Select->setChecked(edit_mode == "select");
    ui->toolButton_Fill->setChecked(edit_mode == "fill");
    ui->toolButton_Dropper->setChecked(edit_mode == "pick");
    ui->toolButton_Move->setChecked(edit_mode == "move");
    ui->toolButton_Shift->setChecked(edit_mode == "shift");
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

void MainWindow::onMapChanged(Map *) {
    updateMapList();
}

void MainWindow::onMapNeedsRedrawing() {
    redrawMapScene();
}

void MainWindow::onMapCacheCleared() {
    editor->map = nullptr;
}

void MainWindow::onTilesetsSaved(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    // If saved tilesets are currently in-use, update them and redraw
    // Otherwise overwrite the cache for the saved tileset
    bool updated = false;
    if (primaryTilesetLabel == this->editor->map->layout->tileset_primary_label) {
        this->editor->updatePrimaryTileset(primaryTilesetLabel, true);
        Scripting::cb_TilesetUpdated(primaryTilesetLabel);
        updated = true;
    } else {
        this->editor->project->getTileset(primaryTilesetLabel, true);
    }
    if (secondaryTilesetLabel == this->editor->map->layout->tileset_secondary_label)  {
        this->editor->updateSecondaryTileset(secondaryTilesetLabel, true);
        Scripting::cb_TilesetUpdated(secondaryTilesetLabel);
        updated = true;
    } else {
        this->editor->project->getTileset(secondaryTilesetLabel, true);
    }
    if (updated)
        redrawMapScene();
}

void MainWindow::onWildMonDataChanged() {
    editor->saveEncounterTabData();
    markMapEdited();
}

void MainWindow::onMapRulerStatusChanged(const QString &status) {
    if (status.isEmpty()) {
        label_MapRulerStatus->hide();
    } else if (label_MapRulerStatus->parentWidget()) {
        label_MapRulerStatus->setText(status);
        label_MapRulerStatus->adjustSize();
        label_MapRulerStatus->show();
        label_MapRulerStatus->move(label_MapRulerStatus->parentWidget()->mapToGlobal(QPoint(6, 6)));
    }
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    if (label_MapRulerStatus && label_MapRulerStatus->isVisible() && label_MapRulerStatus->parentWidget())
        label_MapRulerStatus->move(label_MapRulerStatus->parentWidget()->mapToGlobal(QPoint(6, 6)));
}

void MainWindow::on_action_Export_Map_Image_triggered() {
    showExportMapImageWindow(ImageExporterMode::Normal);
}

void MainWindow::on_actionExport_Stitched_Map_Image_triggered() {
    showExportMapImageWindow(ImageExporterMode::Stitch);
}

void MainWindow::on_actionExport_Map_Timelapse_Image_triggered() {
    showExportMapImageWindow(ImageExporterMode::Timelapse);
}

void MainWindow::on_actionImport_Map_from_Advance_Map_1_92_triggered(){
    importMapFromAdvanceMap1_92();
}

void MainWindow::importMapFromAdvanceMap1_92()
{
    QString filepath = QFileDialog::getOpenFileName(
                this,
                QString("Import Map from Advance Map 1.92"),
                this->editor->project->importExportPath,
                "Advance Map 1.92 Map Files (*.map)");
    if (filepath.isEmpty()) {
        return;
    }
    this->editor->project->setImportExportPath(filepath);
    MapParser parser;
    bool error = false;
    MapLayout *mapLayout = parser.parse(filepath, &error, editor->project);
    if (error) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import map from Advance Map 1.92 .map file.");
        QString message = QString("The .map file could not be processed. View porymap.log for specific errors.");
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    openNewMapPopupWindow();
    this->newMapPrompt->init(mapLayout);
}

void MainWindow::showExportMapImageWindow(ImageExporterMode mode) {
    if (!editor->project) return;

    if (this->mapImageExporter)
        delete this->mapImageExporter;

    this->mapImageExporter = new MapImageExporter(this, this->editor, mode);
    this->mapImageExporter->setAttribute(Qt::WA_DeleteOnClose);

    openSubWindow(this->mapImageExporter);
}

void MainWindow::on_comboBox_ConnectionDirection_currentTextChanged(const QString &direction)
{
    editor->updateCurrentConnectionDirection(direction);
    markMapEdited();
}

void MainWindow::on_spinBox_ConnectionOffset_valueChanged(int offset)
{
    editor->updateConnectionOffset(offset);
    markMapEdited();
}

void MainWindow::on_comboBox_ConnectedMap_currentTextChanged(const QString &mapName)
{
    if (mapName.isEmpty() || editor->project->mapNames.contains(mapName)) {
        editor->setConnectionMap(mapName);
        markMapEdited();
    }
}

void MainWindow::on_pushButton_AddConnection_clicked()
{
    editor->addNewConnection();
    markMapEdited();
}

void MainWindow::on_pushButton_RemoveConnection_clicked()
{
    editor->removeCurrentConnection();
    markMapEdited();
}

void MainWindow::on_pushButton_NewWildMonGroup_clicked() {
    editor->addNewWildMonGroup(this);
}

void MainWindow::on_pushButton_DeleteWildMonGroup_clicked() {
    editor->deleteWildMonGroup();
}

void MainWindow::on_pushButton_ConfigureEncountersJSON_clicked() {
    editor->configureEncounterJSON(this);
}

void MainWindow::on_comboBox_DiveMap_currentTextChanged(const QString &mapName)
{
    if (mapName.isEmpty() || editor->project->mapNames.contains(mapName)) {
        editor->updateDiveMap(mapName);
        markMapEdited();
    }
}

void MainWindow::on_comboBox_EmergeMap_currentTextChanged(const QString &mapName)
{
    if (mapName.isEmpty() || editor->project->mapNames.contains(mapName)) {
        editor->updateEmergeMap(mapName);
        markMapEdited();
    }
}

void MainWindow::on_comboBox_PrimaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->primaryTilesetLabels.contains(tilesetLabel) && editor->map) {
        editor->updatePrimaryTileset(tilesetLabel);
        redrawMapScene();
        on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->map);
        markMapEdited();
    }
}

void MainWindow::on_comboBox_SecondaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->secondaryTilesetLabels.contains(tilesetLabel) && editor->map) {
        editor->updateSecondaryTileset(tilesetLabel);
        redrawMapScene();
        on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->map);
        markMapEdited();
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
    widthSpinBox->setMaximum(editor->project->getMaxMapWidth());
    heightSpinBox->setMaximum(editor->project->getMaxMapHeight());
    bwidthSpinBox->setMaximum(MAX_BORDER_WIDTH);
    bheightSpinBox->setMaximum(MAX_BORDER_HEIGHT);
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
    errorLabel->setStyleSheet("QLabel { color: red }");
    errorLabel->setVisible(false);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, [&dialog, &widthSpinBox, &heightSpinBox, &errorLabel, this](){
        // Ensure width and height are an acceptable size.
        // The maximum number of metatiles in a map is the following:
        //    max = (width + 15) * (height + 14)
        // This limit can be found in fieldmap.c in pokeruby/pokeemerald/pokefirered.
        int numMetatiles = editor->project->getMapDataSize(widthSpinBox->value(), heightSpinBox->value());
        int maxMetatiles = editor->project->getMaxMapDataSize();
        if (numMetatiles <= maxMetatiles) {
            dialog.accept();
        } else {
            QString errorText = QString("Error: The specified width and height are too large.\n"
                    "The maximum map width and height is the following: (width + 15) * (height + 14) <= %1\n"
                    "The specified map width and height was: (%2 + 15) * (%3 + 14) = %4")
                        .arg(maxMetatiles)
                        .arg(widthSpinBox->value())
                        .arg(heightSpinBox->value())
                        .arg(numMetatiles);
            errorLabel->setText(errorText);
            errorLabel->setVisible(true);
        }
    });
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    form.addRow(errorLabel);

    if (dialog.exec() == QDialog::Accepted) {
        Map *map = editor->map;
        Blockdata oldMetatiles = map->layout->blockdata;
        Blockdata oldBorder = map->layout->border;
        QSize oldMapDimensions(map->getWidth(), map->getHeight());
        QSize oldBorderDimensions(map->getBorderWidth(), map->getBorderHeight());
        QSize newMapDimensions(widthSpinBox->value(), heightSpinBox->value());
        QSize newBorderDimensions(bwidthSpinBox->value(), bheightSpinBox->value());
        if (oldMapDimensions != newMapDimensions || oldBorderDimensions != newBorderDimensions) {
            editor->map->setDimensions(newMapDimensions.width(), newMapDimensions.height(), true, true);
            editor->map->setBorderDimensions(newBorderDimensions.width(), newBorderDimensions.height(), true, true);
            editor->map->editHistory.push(new ResizeMap(map,
                oldMapDimensions, newMapDimensions,
                oldMetatiles, map->layout->blockdata,
                oldBorderDimensions, newBorderDimensions,
                oldBorder, map->layout->border
            ));
        }
    }
}

void MainWindow::on_checkBox_smartPaths_stateChanged(int selected)
{
    bool enabled = selected == Qt::Checked;
    editor->settings->smartPathsEnabled = enabled;
    if (enabled) {
        editor->cursorMapTileRect->setSmartPathMode(true);
    } else {
        editor->cursorMapTileRect->setSmartPathMode(false);
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
        initTilesetEditor();
    }

    openSubWindow(this->tilesetEditor);

    MetatileSelection selection = this->editor->metatile_selector_item->getMetatileSelection();
    this->tilesetEditor->selectMetatile(selection.metatileItems.first().metatileId);
}

void MainWindow::initTilesetEditor() {
    this->tilesetEditor = new TilesetEditor(this->editor->project, this->editor->map, this);
    connect(this->tilesetEditor, &TilesetEditor::tilesetsSaved, this, &MainWindow::onTilesetsSaved);
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

void MainWindow::on_actionOpen_Log_File_triggered() {
    const QString logPath = getLogPath();
    const int lineCount = ParseUtil::textFileLineCount(logPath);
    this->editor->openInTextEditor(logPath, lineCount);
}

void MainWindow::on_actionOpen_Config_Folder_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)));
}

void MainWindow::on_actionPreferences_triggered() {
    if (!preferenceEditor) {
        preferenceEditor = new PreferenceEditor(this);
        connect(preferenceEditor, &PreferenceEditor::themeChanged,
                this, &MainWindow::setTheme);
        connect(preferenceEditor, &PreferenceEditor::themeChanged,
                editor, &Editor::maskNonVisibleConnectionTiles);
        connect(preferenceEditor, &PreferenceEditor::preferencesSaved,
                this, &MainWindow::togglePreferenceSpecificUi);
    }

    openSubWindow(preferenceEditor);
}

void MainWindow::togglePreferenceSpecificUi() {
    if (porymapConfig.getTextEditorOpenFolder().isEmpty())
        ui->actionOpen_Project_in_Text_Editor->setEnabled(false);
    else
        ui->actionOpen_Project_in_Text_Editor->setEnabled(true);
}

void MainWindow::openProjectSettingsEditor(int tab) {
    if (!this->projectSettingsEditor) {
        this->projectSettingsEditor = new ProjectSettingsEditor(this, this->editor->project);
        connect(this->projectSettingsEditor, &ProjectSettingsEditor::reloadProject,
                this, &MainWindow::on_action_Reload_Project_triggered);
    }
    this->projectSettingsEditor->setTab(tab);
    openSubWindow(this->projectSettingsEditor);
}

void MainWindow::on_actionProject_Settings_triggered() {
    this->openProjectSettingsEditor(porymapConfig.getProjectSettingsTab());
}

void MainWindow::onWarpBehaviorWarningClicked() {
    static const QString text = QString("Warp Events only function as exits on certain metatiles");
    static const QString informative = QString(
        "<html><head/><body><p>"
        "For instance, most floor metatiles in a cave have the metatile behavior <b>MB_CAVE</b>, but the floor space in front of an exit "
        "will have <b>MB_SOUTH_ARROW_WARP</b>, which is treated specially in your project's code to allow a Warp Event to warp the player. "
        "<br><br>"
        "You can see in the status bar what behavior a metatile has when you mouse over it, or by selecting it in the Tileset Editor. "
        "The warning will disappear when the warp is positioned on a metatile with a behavior known to allow warps."
        "<br><br>"
        "<b>Note</b>: Not all Warp Events that show this warning are incorrect! For example some warps may function "
        "as a 1-way entrance, and others may have the metatile underneath them changed programmatically."
        "<br><br>"
        "You can disable this warning or edit the list of behaviors that silence this warning under <b>Options -> Project Settings...</b>"
        "<br></html></body></p>"
    );
    QMessageBox msgBox(QMessageBox::Information, "porymap", text, QMessageBox::Close, this);
    QPushButton *settings = msgBox.addButton("Open Settings...", QMessageBox::ActionRole);
    msgBox.setDefaultButton(QMessageBox::Close);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setInformativeText(informative);
    msgBox.exec();
    if (msgBox.clickedButton() == settings)
        this->openProjectSettingsEditor(ProjectSettingsEditor::eventsTab);
}

void MainWindow::on_actionCustom_Scripts_triggered() {
    if (!this->customScriptsEditor)
        initCustomScriptsEditor();

    openSubWindow(this->customScriptsEditor);
}

void MainWindow::initCustomScriptsEditor() {
    this->customScriptsEditor = new CustomScriptsEditor(this);
    connect(this->customScriptsEditor, &CustomScriptsEditor::reloadScriptEngine,
            this, &MainWindow::reloadScriptEngine);
}

void MainWindow::reloadScriptEngine() {
    Scripting::init(this);
    this->ui->graphicsView_Map->clearOverlayMap();
    Scripting::populateGlobalObject(this);
    // Lying to the scripts here, simulating a project reload
    Scripting::cb_ProjectOpened(projectConfig.getProjectDir());
    if (editor && editor->map)
        Scripting::cb_MapOpened(editor->map->name);
}

void MainWindow::on_pushButton_AddCustomHeaderField_clicked()
{
    bool ok;
    QJsonValue value = CustomAttributesTable::pickType(this, &ok);
    if (ok){
        CustomAttributesTable::addAttribute(this->ui->tableWidget_CustomHeaderFields, "", value, true);
        this->editor->updateCustomMapHeaderValues(this->ui->tableWidget_CustomHeaderFields);
    }
}

void MainWindow::on_pushButton_DeleteCustomHeaderField_clicked()
{
    if (CustomAttributesTable::deleteSelectedAttributes(this->ui->tableWidget_CustomHeaderFields))
        this->editor->updateCustomMapHeaderValues(this->ui->tableWidget_CustomHeaderFields);
}

void MainWindow::on_tableWidget_CustomHeaderFields_cellChanged(int, int)
{
    this->editor->updateCustomMapHeaderValues(this->ui->tableWidget_CustomHeaderFields);
}

void MainWindow::on_horizontalSlider_MetatileZoom_valueChanged(int value) {
    porymapConfig.setMetatilesZoom(value);
    double scale = pow(3.0, static_cast<double>(value - 30) / 30.0);

    QTransform transform;
    transform.scale(scale, scale);
    QSize size(editor->metatile_selector_item->pixmap().width(), 
               editor->metatile_selector_item->pixmap().height());
    size *= scale;

    ui->graphicsView_Metatiles->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Metatiles->setTransform(transform);
    ui->graphicsView_Metatiles->setFixedSize(size.width() + 2, size.height() + 2);

    ui->graphicsView_BorderMetatile->setTransform(transform);
    ui->graphicsView_BorderMetatile->setFixedSize(ceil(static_cast<double>(editor->selected_border_metatiles_item->pixmap().width()) * scale) + 2,
                                                  ceil(static_cast<double>(editor->selected_border_metatiles_item->pixmap().height()) * scale) + 2);

    redrawMetatileSelection();
}

void MainWindow::on_horizontalSlider_CollisionZoom_valueChanged(int value) {
    porymapConfig.setCollisionZoom(value);
    double scale = pow(3.0, static_cast<double>(value - 30) / 30.0);

    QTransform transform;
    transform.scale(scale, scale);
    QSize size(editor->movement_permissions_selector_item->pixmap().width(),
               editor->movement_permissions_selector_item->pixmap().height());
    size *= scale;

    ui->graphicsView_Collision->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Collision->setTransform(transform);
    ui->graphicsView_Collision->setFixedSize(size.width() + 2, size.height() + 2);
}

void MainWindow::on_spinBox_SelectedCollision_valueChanged(int collision) {
    if (this->editor && this->editor->movement_permissions_selector_item)
        this->editor->movement_permissions_selector_item->select(collision, ui->spinBox_SelectedElevation->value());
}

void MainWindow::on_spinBox_SelectedElevation_valueChanged(int elevation) {
    if (this->editor && this->editor->movement_permissions_selector_item)
        this->editor->movement_permissions_selector_item->select(ui->spinBox_SelectedCollision->value(), elevation);
}

void MainWindow::on_actionRegion_Map_Editor_triggered() {
    if (!this->regionMapEditor) {
        if (!initRegionMapEditor()) {
            return;
        }
    }

    openSubWindow(this->regionMapEditor);
}

void MainWindow::on_pushButton_CreatePrefab_clicked() {
    PrefabCreationDialog dialog(this, this->editor->metatile_selector_item, this->editor->map);
    dialog.setWindowTitle("Create Prefab");
    dialog.setWindowModality(Qt::NonModal);
    if (dialog.exec() == QDialog::Accepted) {
        dialog.savePrefab();
    }
}

bool MainWindow::initRegionMapEditor(bool silent) {
    this->regionMapEditor = new RegionMapEditor(this, this->editor->project);
    bool success = this->regionMapEditor->load(silent);
    if (!success) {
        delete this->regionMapEditor;
        this->regionMapEditor = nullptr;
        if (!silent) {
            QMessageBox msgBox(this);
            QString errorMsg = QString("There was an error opening the region map data. Please see %1 for full error details.\n\n%3")
                    .arg(getLogPath())
                    .arg(getMostRecentError());
            msgBox.critical(nullptr, "Error Opening Region Map Editor", errorMsg);
        }
        return false;
    }

    return true;
}

void MainWindow::closeSupplementaryWindows() {
    delete this->tilesetEditor;
    delete this->regionMapEditor;
    delete this->mapImageExporter;
    delete this->newMapPrompt;
    delete this->shortcutsEditor;
    delete this->customScriptsEditor;
    if (this->projectSettingsEditor) this->projectSettingsEditor->closeQuietly();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (isProjectOpen()) {
        if (projectHasUnsavedChanges || (editor->map && editor->map->hasUnsavedChanges())) {
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
        projectConfig.save();
        userConfig.save();
    }

    porymapConfig.setMainGeometry(
        this->saveGeometry(),
        this->saveState(),
        this->ui->splitter_map->saveState(),
        this->ui->splitter_main->saveState()
    );
    porymapConfig.save();
    shortcutsConfig.save();

    QMainWindow::closeEvent(event);
}
