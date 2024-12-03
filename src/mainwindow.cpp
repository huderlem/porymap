#include "mainwindow.h"
#include "ui_mainwindow.h"
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
#include "advancemapparser.h"
#include "prefab.h"
#include "montabwidget.h"
#include "imageexport.h"
#include "maplistmodels.h"
#include "eventfilters.h"
#include "newmapconnectiondialog.h"
#include "config.h"
#include "filedialog.h"
#include "newmapdialog.h"
#include "newlayoutdialog.h"

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

// We only publish release binaries for Windows and macOS.
// This is relevant for the update promoter, which alerts users of a new release.
// TODO: Currently the update promoter is disabled on our Windows releases because
//       the pre-compiled Qt build doesn't link OpenSSL. Re-enable below once this is fixed.
#if /*defined(Q_OS_WIN) || */defined(Q_OS_MACOS)
#define RELEASE_PLATFORM
#endif

using OrderedJson = poryjson::Json;
using OrderedJsonDoc = poryjson::JsonDoc;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isProgrammaticEventTabChange(false)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("porymap");
    QCoreApplication::setApplicationVersion(PORYMAP_VERSION);
    QApplication::setApplicationDisplayName("porymap");
    QApplication::setWindowIcon(QIcon(":/icons/porymap-icon-2.ico"));
    ui->setupUi(this);

    cleanupLargeLog();
    logInfo(QString("Launching Porymap v%1").arg(QCoreApplication::applicationVersion()));

    this->initWindow();
    if (porymapConfig.reopenOnLaunch && !porymapConfig.projectManuallyClosed && this->openProject(porymapConfig.getRecentProject(), true))
        on_toolButton_Paint_clicked();

    // there is a bug affecting macOS users, where the trackpad deilveres a bad touch-release gesture
    // the warning is a bit annoying, so it is disabled here
    QLoggingCategory::setFilterRules(QStringLiteral("qt.pointer.dispatch=false"));

    if (porymapConfig.checkForUpdates)
        this->checkForUpdates(false);
}

MainWindow::~MainWindow()
{
    // Some config settings are updated as subwindows are destroyed (e.g. their geometry),
    // so we need to ensure that the configs are saved after this happens.
    saveGlobalConfigs();

    delete label_MapRulerStatus;
    delete editor;
    delete ui;
}

void MainWindow::saveGlobalConfigs() {
    porymapConfig.setMainGeometry(
        this->saveGeometry(),
        this->saveState(),
        this->ui->splitter_map->saveState(),
        this->ui->splitter_main->saveState(),
        this->ui->splitter_Metatiles->saveState()
    );
    porymapConfig.save();
    shortcutsConfig.save();
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
    ui->actionCheck_for_Updates->setDisabled(false);
    if (!disabled)
        togglePreferenceSpecificUi();
}

void MainWindow::initWindow() {
    porymapConfig.load();
    this->initCustomUI();
    this->initExtraSignals();
    this->initEditor();
    this->initMiscHeapObjects();
    this->initMapList();
    this->initShortcuts();
    this->restoreWindowState();

#ifndef RELEASE_PLATFORM
    ui->actionCheck_for_Updates->setVisible(false);
#endif

#ifdef DISABLE_CHARTS_MODULE
    ui->pushButton_SummaryChart->setVisible(false);
#endif

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

    auto *shortcutDuplicate_Events = new Shortcut(QKeySequence("Ctrl+D"), this, SLOT(duplicate()));
    shortcutDuplicate_Events->setObjectName("shortcutDuplicate_Events");
    shortcutDuplicate_Events->setWhatsThis("Duplicate Selected Event(s)");

    auto *shortcutToggle_Border = new Shortcut(QKeySequence(), ui->checkBox_ToggleBorder, SLOT(toggle()));
    shortcutToggle_Border->setObjectName("shortcutToggle_Border");
    shortcutToggle_Border->setWhatsThis("Toggle Border");

    auto *shortcutToggle_Smart_Paths = new Shortcut(QKeySequence(), ui->checkBox_smartPaths, SLOT(toggle()));
    shortcutToggle_Smart_Paths->setObjectName("shortcutToggle_Smart_Paths");
    shortcutToggle_Smart_Paths->setWhatsThis("Toggle Smart Paths");

    auto *shortcutHide_Show = new Shortcut(QKeySequence(), this, SLOT(mapListShortcut_ToggleEmptyFolders()));
    shortcutHide_Show->setObjectName("shortcutHide_Show");
    shortcutHide_Show->setWhatsThis("Map List: Hide/Show Empty Folders");

    auto *shortcutExpand_All = new Shortcut(QKeySequence(), this, SLOT(mapListShortcut_ExpandAll()));
    shortcutExpand_All->setObjectName("shortcutExpand_All");
    shortcutExpand_All->setWhatsThis("Map List: Expand all folders");

    auto *shortcutCollapse_All = new Shortcut(QKeySequence(), this, SLOT(mapListShortcut_CollapseAll()));
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
    static const QMap<int, QString> mainTabNames = {
        {MainTab::Map, "Map"},
        {MainTab::Events, "Events"},
        {MainTab::Header, "Header"},
        {MainTab::Connections, "Connections"},
        {MainTab::WildPokemon, "Wild Pokemon"},
    };

    static const QMap<int, QIcon> mainTabIcons = {
        {MainTab::Map, QIcon(QStringLiteral(":/icons/minimap.ico"))},
        {MainTab::Events, QIcon(QStringLiteral(":/icons/viewsprites.ico"))},
        {MainTab::Header, QIcon(QStringLiteral(":/icons/application_form_edit.ico"))},
        {MainTab::Connections, QIcon(QStringLiteral(":/icons/connections.ico"))},
        {MainTab::WildPokemon, QIcon(QStringLiteral(":/icons/tall_grass.ico"))},
    };

    // Set up the tab bar
    while (ui->mainTabBar->count()) ui->mainTabBar->removeTab(0);

    for (int i = 0; i < mainTabNames.count(); i++) {
        ui->mainTabBar->addTab(mainTabNames.value(i));
        ui->mainTabBar->setTabIcon(i, mainTabIcons.value(i));
    }

    // Create map header data widget
    this->mapHeaderForm = new MapHeaderForm();
    ui->layout_HeaderData->addWidget(this->mapHeaderForm);
}

void MainWindow::initExtraSignals() {
    // other signals
    connect(ui->newEventToolButton, &NewEventToolButton::newEventAdded, this, &MainWindow::addNewEvent);
    connect(ui->tabWidget_EventType, &QTabWidget::currentChanged, this, &MainWindow::eventTabChanged);

    // Change pages on wild encounter groups
    connect(ui->comboBox_EncounterGroupLabel, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index){
        ui->stackedWidget_WildMons->setCurrentIndex(index);
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

    connect(ui->action_NewMap, &QAction::triggered, this, &MainWindow::openNewMapDialog);
    connect(ui->action_NewLayout, &QAction::triggered, this, &MainWindow::openNewLayoutDialog);
}

void MainWindow::on_actionCheck_for_Updates_triggered() {
    checkForUpdates(true);
}

#ifdef RELEASE_PLATFORM
void MainWindow::checkForUpdates(bool requestedByUser) {
    if (!this->networkAccessManager)
        this->networkAccessManager = new NetworkAccessManager(this);

    if (!this->updatePromoter) {
        this->updatePromoter = new UpdatePromoter(this, this->networkAccessManager);
        connect(this->updatePromoter, &UpdatePromoter::changedPreferences, [this] {
            if (this->preferenceEditor)
                this->preferenceEditor->updateFields();
        });
    }


    if (requestedByUser) {
        openSubWindow(this->updatePromoter);
    } else {
        // This is an automatic update check. Only run if we haven't done one in the last 5 minutes
        QDateTime lastCheck = porymapConfig.lastUpdateCheckTime;
        if (lastCheck.addSecs(5*60) >= QDateTime::currentDateTime())
            return;
    }
    this->updatePromoter->checkForUpdates();
    porymapConfig.lastUpdateCheckTime = QDateTime::currentDateTime();
}
#else
void MainWindow::checkForUpdates(bool) {}
#endif

void MainWindow::initEditor() {
    this->editor = new Editor(ui);
    connect(this->editor, &Editor::objectsChanged, this, &MainWindow::updateObjects);
    connect(this->editor, &Editor::openConnectedMap, this, &MainWindow::onOpenConnectedMap);
    connect(this->editor, &Editor::warpEventDoubleClicked, this, &MainWindow::openWarpMap);
    connect(this->editor, &Editor::currentMetatilesSelectionChanged, this, &MainWindow::currentMetatilesSelectionChanged);
    connect(this->editor, &Editor::wildMonTableEdited, [this] { this->markMapEdited(); });
    connect(this->editor, &Editor::mapRulerStatusChanged, this, &MainWindow::onMapRulerStatusChanged);
    connect(this->editor, &Editor::tilesetUpdated, this, &Scripting::cb_TilesetUpdated);
    connect(ui->toolButton_deleteObject, &QAbstractButton::clicked, this->editor, &Editor::deleteSelectedEvents);

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
    connect(showHistory, &QAction::triggered, [this, undoView](){ openSubWindow(undoView); });

    ui->menuEdit->addAction(showHistory);

    // Toggle an asterisk in the window title when the undo state is changed
    connect(&editor->editGroup, &QUndoGroup::indexChanged, this, &MainWindow::updateWindowTitle);

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
    ui->tabWidget_EventType->clear();
}

void MainWindow::initMapList() {
    ui->mapListContainer->setCurrentIndex(porymapConfig.mapListTab);

    WheelFilter *wheelFilter = new WheelFilter(this);
    ui->mainTabBar->installEventFilter(wheelFilter);
    ui->mapListContainer->tabBar()->installEventFilter(wheelFilter);

    // Create buttons for adding and removing items from the mapList
    QFrame *buttonFrame = new QFrame(this->ui->mapListContainer);
    buttonFrame->setFrameShape(QFrame::NoFrame);

    QHBoxLayout *layout = new QHBoxLayout(buttonFrame);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create add map/layout button
    // TODO: Tool tip
    QPushButton *buttonAdd = new QPushButton(QIcon(":/icons/add.ico"), "");
    connect(buttonAdd, &QPushButton::clicked, this, &MainWindow::openNewMapDialog);
    layout->addWidget(buttonAdd);

    /* TODO: Remove button disabled, no current support for deleting maps/layouts
    // Create remove map/layout button
    QPushButton *buttonRemove = new QPushButton(QIcon(":/icons/delete.ico"), "");
    connect(buttonRemove, &QPushButton::clicked, this, &MainWindow::deleteCurrentMapOrLayout);
    layout->addWidget(buttonRemove);
    */

    ui->mapListContainer->setCornerWidget(buttonFrame, Qt::TopRightCorner);

    // Connect tool bars to lists
    ui->mapListToolBar_Groups->setList(ui->mapList);
    ui->mapListToolBar_Areas->setList(ui->areaList);
    ui->mapListToolBar_Layouts->setList(ui->layoutList);

    // Left-clicking on items in the map list opens the corresponding map/layout.
    connect(ui->mapList,    &QAbstractItemView::activated, this, &MainWindow::openMapListItem);
    connect(ui->areaList,   &QAbstractItemView::activated, this, &MainWindow::openMapListItem);
    connect(ui->layoutList, &QAbstractItemView::activated, this, &MainWindow::openMapListItem);

    // Right-clicking on items in the map list brings up a context menu.
    connect(ui->mapList, &QTreeView::customContextMenuRequested, this, &MainWindow::onOpenMapListContextMenu);
    connect(ui->areaList, &QTreeView::customContextMenuRequested, this, &MainWindow::onOpenMapListContextMenu);
    connect(ui->layoutList, &QTreeView::customContextMenuRequested, this, &MainWindow::onOpenMapListContextMenu);

    // Only the groups list allows reorganizing folder contents, editing folder names, etc.
    ui->mapListToolBar_Areas->setEditsAllowedButtonVisible(false);
    ui->mapListToolBar_Layouts->setEditsAllowedButtonVisible(false);

    // When map list search filter is cleared we want the current map/layout in the editor to be visible in the list.
    connect(ui->mapListToolBar_Groups,  &MapListToolBar::filterCleared, this, &MainWindow::scrollMapListToCurrentMap);
    connect(ui->mapListToolBar_Areas,   &MapListToolBar::filterCleared, this, &MainWindow::scrollMapListToCurrentMap);
    connect(ui->mapListToolBar_Layouts, &MapListToolBar::filterCleared, this, &MainWindow::scrollMapListToCurrentLayout);

    // Connect the "add folder" button in each of the map lists
    connect(ui->mapListToolBar_Groups,  &MapListToolBar::addFolderClicked, this, &MainWindow::mapListAddGroup);
    connect(ui->mapListToolBar_Areas,   &MapListToolBar::addFolderClicked, this, &MainWindow::mapListAddArea);
    connect(ui->mapListToolBar_Layouts, &MapListToolBar::addFolderClicked, this, &MainWindow::openNewLayoutDialog);

    connect(ui->mapListContainer, &QTabWidget::currentChanged, this, &MainWindow::saveMapListTab);
}

void MainWindow::updateWindowTitle() {
    if (!editor || !editor->project) {
        setWindowTitle(QCoreApplication::applicationName());
        return;
    }

    const QString projectName = editor->project->getProjectTitle();
    if (!editor->layout) {
        setWindowTitle(projectName);
        return;
    }

    if (editor->map) {
        setWindowTitle(QString("%1%2 - %3")
            .arg(editor->map->hasUnsavedChanges() ? "* " : "")
            .arg(editor->map->name())
            .arg(projectName)
        );
    } else {
        setWindowTitle(QString("%1%2 - %3")
            .arg(editor->layout->hasUnsavedChanges() ? "* " : "")
            .arg(editor->layout->name)
            .arg(projectName)
        );
    }

    // For some reason (perhaps on Qt < 6?) we had to clear the icon first here or mainTabBar wouldn't display correctly.
    ui->mainTabBar->setTabIcon(MainTab::Map, QIcon());

    QPixmap pixmap = editor->layout->pixmap;
    if (!pixmap.isNull()) {
        ui->mainTabBar->setTabIcon(MainTab::Map, QIcon(pixmap));
    } else {
        ui->mainTabBar->setTabIcon(MainTab::Map, QIcon(QStringLiteral(":/icons/map.ico")));
    }
}

void MainWindow::markMapEdited() {
    if (editor) markSpecificMapEdited(editor->map);
}

void MainWindow::markSpecificMapEdited(Map* map) {
    if (!map)
        return;
    map->setHasUnsavedDataChanges(true);

    if (editor && editor->map == map)
        updateWindowTitle();
    updateMapList();
}

void MainWindow::loadUserSettings() {
    // Better Cursors
    ui->actionBetter_Cursors->setChecked(porymapConfig.prettyCursors);
    this->editor->settings->betterCursors = porymapConfig.prettyCursors;

    // Player view rectangle
    ui->actionPlayer_View_Rectangle->setChecked(porymapConfig.showPlayerView);
    this->editor->settings->playerViewRectEnabled = porymapConfig.showPlayerView;

    // Cursor tile outline
    ui->actionCursor_Tile_Outline->setChecked(porymapConfig.showCursorTile);
    this->editor->settings->cursorTileRectEnabled = porymapConfig.showCursorTile;

    // Border
    ui->checkBox_ToggleBorder->setChecked(porymapConfig.showBorder);

    // Grid
    const QSignalBlocker b_Grid(ui->checkBox_ToggleGrid);
    ui->actionShow_Grid->setChecked(porymapConfig.showGrid);
    ui->checkBox_ToggleGrid->setChecked(porymapConfig.showGrid);

    // Mirror connections
    ui->checkBox_MirrorConnections->setChecked(porymapConfig.mirrorConnectingMaps);

    // Collision opacity/transparency
    const QSignalBlocker b_CollisionTransparency(ui->horizontalSlider_CollisionTransparency);
    this->editor->collisionOpacity = static_cast<qreal>(porymapConfig.collisionOpacity) / 100;
    ui->horizontalSlider_CollisionTransparency->setValue(porymapConfig.collisionOpacity);

    // Dive map opacity/transparency
    const QSignalBlocker b_DiveEmergeOpacity(ui->slider_DiveEmergeMapOpacity);
    const QSignalBlocker b_DiveMapOpacity(ui->slider_DiveMapOpacity);
    const QSignalBlocker b_EmergeMapOpacity(ui->slider_EmergeMapOpacity);
    ui->slider_DiveEmergeMapOpacity->setValue(porymapConfig.diveEmergeMapOpacity);
    ui->slider_DiveMapOpacity->setValue(porymapConfig.diveMapOpacity);
    ui->slider_EmergeMapOpacity->setValue(porymapConfig.emergeMapOpacity);

    // Zoom
    const QSignalBlocker b_MetatileZoom(ui->horizontalSlider_MetatileZoom);
    const QSignalBlocker b_CollisionZoom(ui->horizontalSlider_CollisionZoom);
    ui->horizontalSlider_MetatileZoom->setValue(porymapConfig.metatilesZoom);
    ui->horizontalSlider_CollisionZoom->setValue(porymapConfig.collisionZoom);

    setTheme(porymapConfig.theme);
    setDivingMapsVisible(porymapConfig.showDiveEmergeMaps);
}

void MainWindow::restoreWindowState() {
    logInfo("Restoring main window geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getMainGeometry();
    this->restoreGeometry(geometry.value("main_window_geometry"));
    this->restoreState(geometry.value("main_window_state"));
    this->ui->splitter_map->restoreState(geometry.value("map_splitter_state"));
    this->ui->splitter_main->restoreState(geometry.value("main_splitter_state"));
    this->ui->splitter_Metatiles->restoreState(geometry.value("metatiles_splitter_state"));
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

bool MainWindow::openProject(QString dir, bool initial) {
    if (dir.isNull() || dir.length() <= 0) {
        // If this happened on startup it's because the user has no recent projects, which is fine.
        // This shouldn't happen otherwise, but if it does then display an error.
        if (!initial) {
            logError("Failed to open project: Directory name cannot be empty");
            showProjectOpenFailure();
        }
        return false;
    }

    const QString projectString = QString("%1project '%2'").arg(initial ? "recent " : "").arg(QDir::toNativeSeparators(dir));

    if (!QDir(dir).exists()) {
        const QString errorMsg = QString("Failed to open %1: No such directory").arg(projectString);
        this->statusBar()->showMessage(errorMsg);
        if (initial) {
            // Graceful startup if recent project directory is missing
            logWarn(errorMsg);
        } else {
            logError(errorMsg);
            showProjectOpenFailure();
        }
        return false;
    }

    // The above checks can fail and the user will be allowed to continue with their currently-opened project (if there is one).
    // We close the current project below, after which either the new project will open successfully or the window will be disabled.
    if (!closeProject()) {
        logInfo("Aborted project open.");
        return false;
    }

    const QString openMessage = QString("Opening %1").arg(projectString);
    this->statusBar()->showMessage(openMessage);
    logInfo(openMessage);

    userConfig.projectDir = dir;
    userConfig.load();
    projectConfig.projectDir = dir;
    projectConfig.load();

    Scripting::init(this);

    // Create the project
    auto project = new Project(editor);
    project->set_root(dir);
    connect(project, &Project::fileChanged, this, &MainWindow::showFileWatcherWarning);
    connect(project, &Project::mapLoaded, this, &MainWindow::onMapLoaded);
    connect(project, &Project::mapCreated, this, &MainWindow::onNewMapCreated);
    connect(project, &Project::layoutCreated, this, &MainWindow::onNewLayoutCreated);
    connect(project, &Project::mapGroupAdded, this, &MainWindow::onNewMapGroupCreated);
    connect(project, &Project::mapSectionAdded, this, &MainWindow::onNewMapSectionCreated);
    connect(project, &Project::mapSectionIdNamesChanged, this->mapHeaderForm, &MapHeaderForm::setLocations);
    this->editor->setProject(project);

    // Make sure project looks reasonable before attempting to load it
    if (!checkProjectSanity()) {
        delete this->editor->project;
        return false;
    }

    // Load the project
    if (!(loadProjectData() && setProjectUI() && setInitialMap())) {
        this->statusBar()->showMessage(QString("Failed to open %1").arg(projectString));
        showProjectOpenFailure();
        delete this->editor->project;
        // TODO: Allow changing project settings at this point
        return false;
    }

    // Only create the config files once the project has opened successfully in case the user selected an invalid directory
    this->editor->project->saveConfig();
    
    updateWindowTitle();
    this->statusBar()->showMessage(QString("Opened %1").arg(projectString));

    porymapConfig.projectManuallyClosed = false;
    porymapConfig.addRecentProject(dir);
    refreshRecentProjectsMenu();

    prefab.initPrefabUI(
                editor->metatile_selector_item,
                ui->scrollAreaWidgetContents_Prefabs,
                ui->label_prefabHelp,
                editor->layout);
    Scripting::cb_ProjectOpened(dir);
    setWindowDisabled(false);
    return true;
}

bool MainWindow::loadProjectData() {
    bool success = editor->project->load();
    Scripting::populateGlobalObject(this);
    return success;
}

bool MainWindow::checkProjectSanity() {
    if (editor->project->sanityCheck())
        return true;

    logWarn(QString("The directory '%1' failed the project sanity check.").arg(editor->project->root));

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(QString("The selected directory appears to be invalid."));
    msgBox.setInformativeText(QString("The directory '%1' is missing key files.\n\n"
                                      "Make sure you selected the correct project directory "
                                      "(the one used to make your .gba file, e.g. 'pokeemerald').").arg(editor->project->root));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    auto tryAnyway = msgBox.addButton("Try Anyway", QMessageBox::ActionRole);
    msgBox.exec();
    if (msgBox.clickedButton() == tryAnyway) {
        // The user has chosen to try to load this project anyway.
        // This will almost certainly fail, but they'll get a more specific error message.
        return true;
    }
    return false;
}

void MainWindow::showProjectOpenFailure() {
    QString errorMsg = QString("There was an error opening the project. Please see %1 for full error details.").arg(getLogPath());
    QMessageBox error(QMessageBox::Critical, "porymap", errorMsg, QMessageBox::Ok, this);
    error.setDetailedText(getMostRecentError());
    error.exec();
}

bool MainWindow::isProjectOpen() {
    return editor && editor->project;
}

bool MainWindow::setInitialMap() {
    const QString recent = userConfig.recentMapOrLayout;
    if (editor->project->mapNames.contains(recent)) {
        // User recently had a map open that still exists.
        if (setMap(recent))
            return true;
    } else if (editor->project->layoutIds.contains(recent)) {
        // User recently had a layout open that still exists.
        if (setLayout(recent))
            return true;
    }

    // Failed to open recent map/layout, or no recent map/layout. Try opening maps then layouts sequentially.
    for (const auto &name : editor->project->mapNames) {
        if (name != recent && setMap(name))
            return true;
    }
    for (const auto &id : editor->project->layoutIds) {
        if (id != recent && setLayout(id))
            return true;
    }

    logError("Failed to load any maps or layouts.");
    return false;
}

void MainWindow::refreshRecentProjectsMenu() {
    ui->menuOpen_Recent_Project->clear();
    QStringList recentProjects = porymapConfig.getRecentProjects();

    if (isProjectOpen()) {
        // Don't show the currently open project in this menu
        recentProjects.removeOne(this->editor->project->root);
    }

    // Add project paths to menu. Skip any paths to folders that don't exist
    for (int i = 0; i < recentProjects.length(); i++) {
        const QString path = recentProjects.at(i);
        if (QDir(path).exists()) {
            ui->menuOpen_Recent_Project->addAction(path, [this, path](){
                this->openProject(path);
            });
        }
        // Arbitrary limit of 10 items.
        if (ui->menuOpen_Recent_Project->actions().length() >= 10)
            break;
    }

    // Add action to clear list of paths
    if (!ui->menuOpen_Recent_Project->actions().isEmpty())
         ui->menuOpen_Recent_Project->addSeparator();
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

void MainWindow::showFileWatcherWarning(QString filepath) {
    if (!porymapConfig.monitorFiles || !isProjectOpen())
        return;

    Project *project = this->editor->project;
    if (project->modifiedFileTimestamps.contains(filepath)) {
        if (QDateTime::currentMSecsSinceEpoch() < project->modifiedFileTimestamps[filepath]) {
            return;
        }
        project->modifiedFileTimestamps.remove(filepath);
    }

    static bool showing = false;
    if (showing) return;

    QMessageBox notice(this);
    notice.setText("File Changed");
    notice.setInformativeText(QString("The file %1 has changed on disk. Would you like to reload the project?")
                              .arg(filepath.remove(project->root + "/")));
    notice.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    notice.setDefaultButton(QMessageBox::No);
    notice.setIcon(QMessageBox::Question);

    QCheckBox showAgainCheck("Do not ask again.");
    notice.setCheckBox(&showAgainCheck);

    showing = true;
    int choice = notice.exec();
    if (choice == QMessageBox::Yes) {
        on_action_Reload_Project_triggered();
    } else if (choice == QMessageBox::No) {
        if (showAgainCheck.isChecked()) {
            porymapConfig.monitorFiles = false;
            if (this->preferenceEditor)
                this->preferenceEditor->updateFields();
        }
    }
    showing = false;
}

QString MainWindow::getExistingDirectory(QString dir) {
    return FileDialog::getExistingDirectory(this, "Open Directory", dir, QFileDialog::ShowDirsOnly);
}

void MainWindow::on_action_Open_Project_triggered()
{
    QString dir = getExistingDirectory(!projectConfig.projectDir.isEmpty() ? userConfig.projectDir : ".");
    if (!dir.isEmpty())
        openProject(dir);
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

void MainWindow::on_action_Close_Project_triggered() {
    closeProject();
    porymapConfig.projectManuallyClosed = true;
}

void MainWindow::unsetMap() {
    this->editor->unsetMap();
    setLayoutOnlyMode(true);
}

// setMap, but with a visible error message in case of failure.
// Use when the user is specifically requesting a map to open.
bool MainWindow::userSetMap(QString map_name) {
    if (editor->map && editor->map->name() == map_name)
        return true; // Already set

    if (map_name == editor->project->getDynamicMapName()) {
        QMessageBox msgBox(this);
        QString errorMsg = QString("The map '%1' can't be opened, it's a placeholder to indicate the specified map will be set programmatically.").arg(map_name);
        msgBox.warning(nullptr, "Cannot Open Map", errorMsg);
        return false;
    }

    if (!setMap(map_name)) {
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening map %1. Please see %2 for full error details.\n\n%3")
                .arg(map_name)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Map", errorMsg);
        return false;
    }
    return true;
}

bool MainWindow::setMap(QString map_name) {
    if (!editor || !editor->project || map_name.isEmpty() || map_name == editor->project->getDynamicMapName()) {
        logWarn(QString("Ignored setting map to '%1'").arg(map_name));
        return false;
    }

    logInfo(QString("Setting map to '%1'").arg(map_name));
    if (!editor->setMap(map_name)) {
        logWarn(QString("Failed to set map to '%1'").arg(map_name));
        return false;
    }

    setLayoutOnlyMode(false);
    this->lastSelectedEvent.clear();

    refreshMapScene();
    displayMapProperties();
    updateWindowTitle();
    updateMapList();
    resetMapListFilters();

    connect(editor->map, &Map::modified, this, &MainWindow::markMapEdited, Qt::UniqueConnection);

    connect(editor->layout, &Layout::layoutChanged, this, &MainWindow::onLayoutChanged, Qt::UniqueConnection);
    connect(editor->layout, &Layout::needsRedrawing, this, &MainWindow::redrawMapScene, Qt::UniqueConnection);

    userConfig.recentMapOrLayout = map_name;

    Scripting::cb_MapOpened(map_name);
    prefab.updatePrefabUi(editor->layout);
    updateTilesetEditor();
    return true;
}

// These parts of the UI only make sense when editing maps.
// When editing in layout-only mode they are disabled.
void MainWindow::setLayoutOnlyMode(bool layoutOnly) {
    bool mapEditingEnabled = !layoutOnly;
    this->ui->mainTabBar->setTabEnabled(MainTab::Events, mapEditingEnabled);
    this->ui->mainTabBar->setTabEnabled(MainTab::Header, mapEditingEnabled);
    this->ui->mainTabBar->setTabEnabled(MainTab::Connections, mapEditingEnabled);
    this->ui->mainTabBar->setTabEnabled(MainTab::WildPokemon, mapEditingEnabled);

    this->ui->comboBox_LayoutSelector->setEnabled(mapEditingEnabled);
}

// setLayout, but with a visible error message in case of failure.
// Use when the user is specifically requesting a layout to open.
// TODO: Update the various functions taking layout IDs to take layout names (to mirror the equivalent map functions, this discrepancy is confusing atm)
bool MainWindow::userSetLayout(QString layoutId) {
    if (!setLayout(layoutId)) {
        QMessageBox msgBox(this);
        QString errorMsg = QString("There was an error opening layout %1. Please see %2 for full error details.\n\n%3")
                .arg(layoutId)
                .arg(getLogPath())
                .arg(getMostRecentError());
        msgBox.critical(nullptr, "Error Opening Layout", errorMsg);
        return false;
    }

    // Only the Layouts tab of the map list shows Layouts, so if we're not already on that tab we'll open it now.
    ui->mapListContainer->setCurrentIndex(MapListTab::Layouts);

    return true;
}

bool MainWindow::setLayout(QString layoutId) {
    // Prefer logging the name of the layout as displayed in the map list.
    const Layout* layout = this->editor->project ? this->editor->project->mapLayouts.value(layoutId) : nullptr;
    logInfo(QString("Setting layout to '%1'").arg(layout ? layout->name : layoutId));

    if (!this->editor->setLayout(layoutId)) {
        return false;
    }

    if (this->editor->map)
        logInfo("Switching to layout-only editing mode. Disabling map-related edits.");

    unsetMap();
    refreshMapScene();
    updateWindowTitle();
    updateMapList();
    resetMapListFilters();

    connect(editor->layout, &Layout::needsRedrawing, this, &MainWindow::redrawMapScene, Qt::UniqueConnection);

    updateTilesetEditor();

    userConfig.recentMapOrLayout = layoutId;

    return true;
}

void MainWindow::redrawMapScene() {
    editor->displayMap();
    editor->displayLayout();
    refreshMapScene();
}

void MainWindow::refreshMapScene() {
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
    // Open the destination map.
    if (!userSetMap(map_name))
        return;

    // Select the target event.
    int index = event_id - Event::getIndexOffset(event_group);
    Event* event = editor->map->getEvent(event_group, index);
    if (event) {
        for (DraggablePixmapItem *item : editor->getObjects()) {
            if (item->event == event) {
                editor->selected_events->clear();
                editor->selected_events->append(item);
                editor->updateSelectedEvents();
                return;
            }
        }
    }
    // Can still warp to this map, but can't select the specified event
    logWarn(QString("%1 %2 doesn't exist on map '%3'").arg(Event::eventGroupToString(event_group)).arg(event_id).arg(map_name));
}

void MainWindow::displayMapProperties() {
    this->mapHeaderForm->clear();
    if (!editor || !editor->map || !editor->project) {
        ui->frame_HeaderData->setEnabled(false);
        return;
    }
    ui->frame_HeaderData->setEnabled(true);
    this->mapHeaderForm->setHeader(editor->map->header());

    const QSignalBlocker b_PrimaryTileset(ui->comboBox_PrimaryTileset);
    const QSignalBlocker b_SecondaryTileset(ui->comboBox_SecondaryTileset);
    ui->comboBox_PrimaryTileset->setCurrentText(editor->map->layout()->tileset_primary_label);
    ui->comboBox_SecondaryTileset->setCurrentText(editor->map->layout()->tileset_secondary_label);


    // Custom fields table.
/* // TODO: Re-enable
    ui->tableWidget_CustomHeaderFields->blockSignals(true);
    ui->tableWidget_CustomHeaderFields->setRowCount(0);
    for (auto it = map->customHeaders.begin(); it != map->customHeaders.end(); it++)
        CustomAttributesTable::addAttribute(ui->tableWidget_CustomHeaderFields, it.key(), it.value());
    ui->tableWidget_CustomHeaderFields->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidget_CustomHeaderFields->blockSignals(false);
*/
}

void MainWindow::on_comboBox_LayoutSelector_currentTextChanged(const QString &text) {
    if (editor && editor->project && editor->map) {
        if (editor->project->mapLayouts.contains(text)) {
            editor->map->setLayout(editor->project->loadLayout(text));
            setMap(editor->map->name());
            markMapEdited();
        }
    }
}

// Update the UI using information we've read from the user's project files.
bool MainWindow::setProjectUI() {
    Project *project = editor->project;

    this->mapHeaderForm->init(project);

    // Set up project comboboxes
    const QSignalBlocker b_PrimaryTileset(ui->comboBox_PrimaryTileset);
    ui->comboBox_PrimaryTileset->clear();
    ui->comboBox_PrimaryTileset->addItems(project->primaryTilesetLabels);

    const QSignalBlocker b_SecondaryTileset(ui->comboBox_SecondaryTileset);
    ui->comboBox_SecondaryTileset->clear();
    ui->comboBox_SecondaryTileset->addItems(project->secondaryTilesetLabels);

    const QSignalBlocker b_LayoutSelector(ui->comboBox_LayoutSelector);
    ui->comboBox_LayoutSelector->clear();
    ui->comboBox_LayoutSelector->addItems(project->layoutIds);

    const QSignalBlocker b_DiveMap(ui->comboBox_DiveMap);
    ui->comboBox_DiveMap->clear();
    ui->comboBox_DiveMap->addItems(project->mapNames);
    ui->comboBox_DiveMap->setClearButtonEnabled(true);
    ui->comboBox_DiveMap->setFocusedScrollingEnabled(false);

    const QSignalBlocker b_EmergeMap(ui->comboBox_EmergeMap);
    ui->comboBox_EmergeMap->clear();
    ui->comboBox_EmergeMap->addItems(project->mapNames);
    ui->comboBox_EmergeMap->setClearButtonEnabled(true);
    ui->comboBox_EmergeMap->setFocusedScrollingEnabled(false);

    // Show/hide parts of the UI that are dependent on the user's project settings

    // Wild Encounters tab
    ui->mainTabBar->setTabEnabled(MainTab::WildPokemon, editor->project->wildEncountersLoaded);

    ui->newEventToolButton->newWeatherTriggerAction->setVisible(projectConfig.eventWeatherTriggerEnabled);
    ui->newEventToolButton->newSecretBaseAction->setVisible(projectConfig.eventSecretBaseEnabled);
    ui->newEventToolButton->newCloneObjectAction->setVisible(projectConfig.eventCloneObjectEnabled);

    Event::setIcons();
    editor->setCollisionGraphics();
    ui->spinBox_SelectedElevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_SelectedCollision->setMaximum(Block::getMaxCollision());

    // map models
    this->mapGroupModel = new MapGroupModel(editor->project);
    this->groupListProxyModel = new FilterChildrenProxyModel();
    groupListProxyModel->setSourceModel(this->mapGroupModel);
    ui->mapList->setModel(groupListProxyModel);

    this->ui->mapList->setItemDelegateForColumn(0, new GroupNameDelegate(this->editor->project, this));
    connect(this->mapGroupModel, &MapGroupModel::dragMoveCompleted, this->ui->mapList, &MapTree::removeSelected);

    this->mapAreaModel = new MapAreaModel(editor->project);
    this->areaListProxyModel = new FilterChildrenProxyModel();
    areaListProxyModel->setSourceModel(this->mapAreaModel);
    ui->areaList->setModel(areaListProxyModel);

    this->layoutTreeModel = new LayoutTreeModel(editor->project);
    this->layoutListProxyModel = new FilterChildrenProxyModel();
    this->layoutListProxyModel->setSourceModel(this->layoutTreeModel);
    ui->layoutList->setModel(layoutListProxyModel);

    return true;
}

void MainWindow::clearProjectUI() {
    // Clear project comboboxes
    const QSignalBlocker b_PrimaryTileset(ui->comboBox_PrimaryTileset);
    ui->comboBox_PrimaryTileset->clear();

    const QSignalBlocker b_SecondaryTileset(ui->comboBox_SecondaryTileset);
    ui->comboBox_SecondaryTileset->clear();

    const QSignalBlocker b_DiveMap(ui->comboBox_DiveMap);
    ui->comboBox_DiveMap->clear();

    const QSignalBlocker b_EmergeMap(ui->comboBox_EmergeMap);
    ui->comboBox_EmergeMap->clear();

    const QSignalBlocker b_LayoutSelector(ui->comboBox_LayoutSelector);
    ui->comboBox_LayoutSelector->clear();

    this->mapHeaderForm->clear();

    // Clear map models
    delete this->mapGroupModel;
    delete this->groupListProxyModel;
    delete this->mapAreaModel;
    delete this->areaListProxyModel;
    delete this->layoutTreeModel;
    delete this->layoutListProxyModel;
    resetMapListFilters();

    Event::clearIcons();
}

void MainWindow::scrollMapList(MapTree *list, const QString &itemName) {
    if (!list || itemName.isEmpty())
        return;
    auto model = static_cast<FilterChildrenProxyModel*>(list->model());
    auto sourceModel = static_cast<MapListModel*>(model->sourceModel());
    QModelIndex sourceIndex = sourceModel->indexOf(itemName);
    if (!sourceIndex.isValid())
        return;
    QModelIndex index = model->mapFromSource(sourceIndex);
    if (!index.isValid())
        return;

    list->setCurrentIndex(index);
    list->setExpanded(index, true);
    list->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void MainWindow::scrollMapListToCurrentMap(MapTree *list) {
    if (this->editor->map) {
        scrollMapList(list, this->editor->map->name());
    }
}

void MainWindow::scrollMapListToCurrentLayout(MapTree *list) {
    if (this->editor->layout) {
        scrollMapList(list, this->editor->layout->id);
    }
}

void MainWindow::onOpenMapListContextMenu(const QPoint &point) {
    // Get selected item from list
    auto list = getCurrentMapList();
    if (!list) return;
    auto model = static_cast<FilterChildrenProxyModel*>(list->model());
    QModelIndex index = model->mapToSource(list->indexAt(point));
    if (!index.isValid()) return;
    auto sourceModel = static_cast<MapListModel*>(model->sourceModel());
    QStandardItem *selectedItem = sourceModel->itemFromIndex(index);
    const QString itemType = selectedItem->data(MapListUserRoles::TypeRole).toString();
    const QString itemName = selectedItem->data(MapListUserRoles::NameRole).toString();

    QMenu menu(this);
    QAction* addToFolderAction = nullptr;
    QAction* deleteFolderAction = nullptr;
    QAction* openItemAction = nullptr;
    QAction* copyDisplayNameAction = nullptr;
    QAction* copyToolTipAction = nullptr;

    if (itemType == "map_name") {
        // Right-clicking on a map.
        openItemAction = menu.addAction("Open Map");
        menu.addSeparator();
        copyDisplayNameAction = menu.addAction("Copy Map Name");
        copyToolTipAction = menu.addAction("Copy Map ID");
        menu.addSeparator();
        connect(menu.addAction("Duplicate Map"), &QAction::triggered, [this, itemName] {
            auto dialog = new NewMapDialog(this->editor->project, this->editor->project->getMap(itemName), this);
            dialog->open();
        });
        //menu.addSeparator();
        //connect(menu.addAction("Delete Map"), &QAction::triggered, [this, index] { deleteMapListItem(index); }); // TODO: No support for deleting maps
    } else if (itemType == "map_group") {
        // Right-clicking on a map group folder
        addToFolderAction = menu.addAction("Add New Map to Group");
        menu.addSeparator();
        deleteFolderAction = menu.addAction("Delete Map Group");
    } else if (itemType == "map_section") {
        // Right-clicking on a MAPSEC folder
        addToFolderAction = menu.addAction("Add New Map to Area");
        menu.addSeparator();
        copyDisplayNameAction = menu.addAction("Copy Area Name");
        menu.addSeparator();
        deleteFolderAction = menu.addAction("Delete Area");
        if (itemName == this->editor->project->getEmptyMapsecName())
            deleteFolderAction->setEnabled(false); // Disallow deleting the default name
    } else if (itemType == "map_layout") {
        // Right-clicking on a map layout
        openItemAction = menu.addAction("Open Layout");
        menu.addSeparator();
        copyDisplayNameAction = menu.addAction("Copy Layout Name");
        copyToolTipAction = menu.addAction("Copy Layout ID");
        menu.addSeparator();
        connect(menu.addAction("Duplicate Layout"), &QAction::triggered, [this, itemName] {
            auto layout = this->editor->project->loadLayout(itemName);
            if (layout) {
                auto dialog = new NewLayoutDialog(this->editor->project, layout, this);
                connect(dialog, &NewLayoutDialog::applied, this, &MainWindow::userSetLayout);
                dialog->open();
            }
        });
        addToFolderAction = menu.addAction("Add New Map with Layout");
        //menu.addSeparator();
        //deleteFolderAction = menu.addAction("Delete Layout"); // TODO: No support for deleting layouts
    }

    if (addToFolderAction) {
        // All folders only contain maps, so adding an item to any folder is adding a new map.
        connect(addToFolderAction, &QAction::triggered, [this, itemName] {
            auto dialog = new NewMapDialog(this->editor->project, ui->mapListContainer->currentIndex(), itemName, this);
            dialog->open();
        });
    }
    if (deleteFolderAction) {
        connect(deleteFolderAction, &QAction::triggered, [sourceModel, index] {
            sourceModel->removeItemAt(index);
        });
        if (selectedItem->hasChildren()){
            // TODO: No support for deleting maps, so you may only delete folders if they don't contain any maps.
            deleteFolderAction->setEnabled(false);
        }
    }

    if (openItemAction) {
        connect(openItemAction, &QAction::triggered, [this, index] {
            openMapListItem(index);
        });
    }
    if (copyDisplayNameAction) {
        connect(copyDisplayNameAction, &QAction::triggered, [this, sourceModel, index] {
            setClipboardData(sourceModel->data(index, Qt::DisplayRole).toString());
        });
    }
    if (copyToolTipAction) {
        connect(copyToolTipAction, &QAction::triggered, [this, sourceModel, index] {
            setClipboardData(sourceModel->data(index, Qt::ToolTipRole).toString());
        });
    }

    if (menu.actions().length() != 0)
        menu.exec(QCursor::pos());
}

void MainWindow::mapListAddGroup() {
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowModality(Qt::ApplicationModal);
    QDialogButtonBox newItemButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&newItemButtonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    QLineEdit *newNameEdit = new QLineEdit(&dialog);
    newNameEdit->setClearButtonEnabled(true);

    static const QRegularExpression re_validChars("[A-Za-z_]+[\\w]*");
    newNameEdit->setValidator(new QRegularExpressionValidator(re_validChars, newNameEdit));

    QLabel *errorMessageLabel = new QLabel(&dialog);
    errorMessageLabel->setVisible(false);
    errorMessageLabel->setStyleSheet("QLabel { background-color: rgba(255, 0, 0, 25%) }");

    connect(&newItemButtonBox, &QDialogButtonBox::accepted, [&](){
        const QString mapGroupName = newNameEdit->text();
        if (!this->editor->project->isIdentifierUnique(mapGroupName)) {
            errorMessageLabel->setText(QString("The name '%1' is not unique.").arg(mapGroupName));
            errorMessageLabel->setVisible(true);
        } else {
            dialog.accept();
        }
    });

    QFormLayout form(&dialog);

    form.addRow("New Group Name", newNameEdit);
    form.addRow("", errorMessageLabel);
    form.addRow(&newItemButtonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString newFieldName = newNameEdit->text();
        if (newFieldName.isEmpty()) return;
        this->editor->project->addNewMapGroup(newFieldName);
    }
}

void MainWindow::mapListAddArea() {
    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowModality(Qt::ApplicationModal);
    QDialogButtonBox newItemButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&newItemButtonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // TODO: This would be a little more seamless with a single line edit that enforces the MAPSEC prefix, rather than a separate label for the actual name.
    const QString prefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_section_prefix);
    auto newNameEdit = new QLineEdit(&dialog);
    auto newNameDisplay = new QLabel(&dialog);
    newNameDisplay->setText(prefix);
    connect(newNameEdit, &QLineEdit::textEdited, [newNameDisplay, prefix] (const QString &text) {
        // As the user types a name, update the label to show the name with the prefix.
        newNameDisplay->setText(prefix + text);
    });

    QLabel *errorMessageLabel = new QLabel(&dialog);
    errorMessageLabel->setVisible(false);
    errorMessageLabel->setStyleSheet("QLabel { background-color: rgba(255, 0, 0, 25%) }");

    static const QRegularExpression re_validChars("[A-Za-z_]+[\\w]*");
    newNameEdit->setValidator(new QRegularExpressionValidator(re_validChars, newNameEdit));

    connect(&newItemButtonBox, &QDialogButtonBox::accepted, [&](){
        const QString newAreaName = newNameDisplay->text();
        if (!this->editor->project->isIdentifierUnique(newAreaName)) {
            errorMessageLabel->setText(QString("The name '%1' is not unique.").arg(newAreaName));
            errorMessageLabel->setVisible(true);
        } else {
            dialog.accept();
        }
    });

    QLabel *newNameEditLabel = new QLabel("New Area Name", &dialog);
    QLabel *newNameDisplayLabel = new QLabel("Constant Name", &dialog);

    QFormLayout form(&dialog);
    form.addRow(newNameEditLabel, newNameEdit);
    form.addRow(newNameDisplayLabel, newNameDisplay);
    form.addRow("", errorMessageLabel);
    form.addRow(&newItemButtonBox);

    if (dialog.exec() == QDialog::Accepted) {
        if (newNameEdit->text().isEmpty()) return;
        this->editor->project->addNewMapsec(newNameDisplay->text());
    }
}

void MainWindow::onNewMapCreated(Map *newMap, const QString &groupName) {
    logInfo(QString("Created a new map named %1.").arg(newMap->name()));

    // TODO: Creating a new map shouldn't be automatically saved.
    //       For one, it takes away the option to discard the new map.
    //       For two, if the new map uses an existing layout, any unsaved changes to that layout will also be saved.
    editor->project->saveMap(newMap);
    editor->project->saveAllDataStructures();

    // Add new map to the map lists
    this->mapGroupModel->insertMapItem(newMap->name(), groupName);
    this->mapAreaModel->insertMapItem(newMap->name(), newMap->header()->location());
    this->layoutTreeModel->insertMapItem(newMap->name(), newMap->layout()->id);

    // Refresh any combo box that displays map names and persists between maps
    // (other combo boxes like for warp destinations are repopulated when the map changes).
    int mapIndex = this->editor->project->mapNames.indexOf(newMap->name());
    if (mapIndex >= 0) {
        const QSignalBlocker b_DiveMap(ui->comboBox_DiveMap);
        const QSignalBlocker b_EmergeMap(ui->comboBox_EmergeMap);
        ui->comboBox_DiveMap->insertItem(mapIndex, newMap->name());
        ui->comboBox_EmergeMap->insertItem(mapIndex, newMap->name());
    }

    if (newMap->needsHealLocation()) {
        addNewEvent(Event::Type::HealLocation);
        editor->project->saveHealLocations(newMap);
        editor->save();
    }

    userSetMap(newMap->name());
}

// Called any time a new layout is created (including as a byproduct of creating a new map)
void MainWindow::onNewLayoutCreated(Layout *layout) {
    logInfo(QString("Created a new layout named %1.").arg(layout->name));

    // Refresh layout combo box
    int layoutIndex = this->editor->project->layoutIds.indexOf(layout->id);
    if (layoutIndex >= 0) {
        const QSignalBlocker b(ui->comboBox_LayoutSelector);
        ui->comboBox_LayoutSelector->insertItem(layoutIndex, layout->id);
    }

    // Add new layout to the Layouts map list view
    this->layoutTreeModel->insertMapFolderItem(layout->id);
}

void MainWindow::onNewMapGroupCreated(const QString &groupName) {
    // Add new map group to the Groups map list view
    this->mapGroupModel->insertMapFolderItem(groupName);
}

void MainWindow::onNewMapSectionCreated(const QString &idName) {
    // Add new map section to the Areas map list view
    this->mapAreaModel->insertMapFolderItem(idName);

    // TODO: Refresh Region Map Editor's map section dropdown, if it's open
}

void MainWindow::openNewMapDialog() {
    auto dialog = new NewMapDialog(this->editor->project, this);
    dialog->open();
}

void MainWindow::openNewLayoutDialog() {
    auto dialog = new NewLayoutDialog(this->editor->project, this);
    connect(dialog, &NewLayoutDialog::applied, this, &MainWindow::userSetLayout);
    dialog->open();
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
            newSet.addMetatile(mt);
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
        editor->project->tilesetLabelsOrdered.append(createTilesetDialog->fullSymbolName);

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
            this->editor->layout,
            editor->ui->comboBox_PrimaryTileset->currentText(),
            editor->ui->comboBox_SecondaryTileset->currentText()
        );
    }
}

double MainWindow::getMetatilesZoomScale() {
    return pow(3.0, static_cast<double>(porymapConfig.metatilesZoom - 30) / 30.0);
}

void MainWindow::redrawMetatileSelection() {
    QSize size(editor->current_metatile_selection_item->pixmap().width(), editor->current_metatile_selection_item->pixmap().height());
    ui->graphicsView_currentMetatileSelection->setSceneRect(0, 0, size.width(), size.height());

    auto scale = getMetatilesZoomScale();
    QTransform transform;
    transform.scale(scale, scale);
    size *= scale;

    ui->graphicsView_currentMetatileSelection->setTransform(transform);
    ui->graphicsView_currentMetatileSelection->setFixedSize(size.width() + 2, size.height() + 2);
    ui->scrollAreaWidgetContents_SelectedMetatiles->adjustSize();
}

void MainWindow::scrollMetatileSelectorToSelection() {
    // Internal selections or 1x1 external selections can be scrolled to
    if (!editor->metatile_selector_item->isInternalSelection() && editor->metatile_selector_item->getSelectionDimensions() != QPoint(1, 1))
        return;

    MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
    if (selection.metatileItems.isEmpty())
        return;

    QPoint pos = editor->metatile_selector_item->getMetatileIdCoordsOnWidget(selection.metatileItems.first().metatileId);
    QPoint size = editor->metatile_selector_item->getSelectionDimensions();
    pos += QPoint(size.x() - 1, size.y() - 1) * 16 / 2; // We want to focus on the center of the whole selection
    pos *= getMetatilesZoomScale();

    auto viewport = ui->scrollArea_MetatileSelector->viewport();
    ui->scrollArea_MetatileSelector->ensureVisible(pos.x(), pos.y(), viewport->width() / 2, viewport->height() / 2);
}

void MainWindow::currentMetatilesSelectionChanged() {
    redrawMetatileSelection();
    if (this->tilesetEditor) {
        MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
        this->tilesetEditor->selectMetatile(selection.metatileItems.first().metatileId);
    }

    // Don't scroll to internal selections here, it will disrupt the user while they make their selection.
    if (!editor->metatile_selector_item->isInternalSelection())
        scrollMetatileSelectorToSelection();
}

void MainWindow::saveMapListTab(int index) {
    porymapConfig.mapListTab = index;
}

void MainWindow::openMapListItem(const QModelIndex &index) {
    if (!index.isValid())
        return;

    QVariant data = index.data(MapListUserRoles::NameRole);
    if (data.isNull())
        return;
    const QString name = data.toString();

    // Normally when a new map/layout is opened the search filters are cleared and the lists will scroll to display that map/layout in the list.
    // We don't want to do this when the user interacts with a list directly, so we temporarily prevent changes to the search filter.
    auto toolbar = getCurrentMapListToolBar();
    if (toolbar) toolbar->setFilterLocked(true);

    QString type = index.data(MapListUserRoles::TypeRole).toString();
    if (type == "map_name") {
        userSetMap(name);
    } else if (type == "map_layout") {
        userSetLayout(name);
    }

    if (toolbar) toolbar->setFilterLocked(false);
}

void MainWindow::updateMapList() {
    // Get the name of the open map/layout (or clear the relevant selection if there is none).
    QString activeItemName; 
    if (this->editor->map) {
        activeItemName = this->editor->map->name();
    } else {
        ui->mapList->clearSelection();
        ui->areaList->clearSelection();

        if (this->editor->layout) {
            activeItemName = this->editor->layout->id;
        } else {
            ui->layoutList->clearSelection();
        }
    }

    this->mapGroupModel->setActiveItem(activeItemName);
    this->mapAreaModel->setActiveItem(activeItemName);
    this->layoutTreeModel->setActiveItem(activeItemName);

    this->groupListProxyModel->layoutChanged();
    this->areaListProxyModel->layoutChanged();
    this->layoutListProxyModel->layoutChanged();
}

void MainWindow::on_action_Save_Project_triggered() {
    editor->saveProject();
    updateWindowTitle();
    updateMapList();
    saveGlobalConfigs();
}

void MainWindow::on_action_Save_triggered() {
    editor->save();
    updateWindowTitle();
    updateMapList();
    saveGlobalConfigs();
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
                    collision["collision"] = projectConfig.defaultCollision;
                    collision["elevation"] = projectConfig.defaultElevation;
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
            case MainTab::Map:
            {
                // copy the map image
                QPixmap pixmap = editor->layout ? editor->layout->render(true) : QPixmap();
                setClipboardData(pixmap.toImage());
                logInfo("Copied current map image to clipboard");
                break;
            }
            case MainTab::Events:
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
        else if (this->ui->mainTabBar->currentIndex() == MainTab::WildPokemon) {
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

void MainWindow::setClipboardData(const QString &text) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

void MainWindow::setClipboardData(QImage image) {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
}

void MainWindow::paste() {
    if (!editor || !editor->project || !(editor->map || editor->layout)) return;

    QClipboard *clipboard = QGuiApplication::clipboard();
    QString clipboardText(clipboard->text());

    if (ui->mainTabBar->currentIndex() == MainTab::WildPokemon) {
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
            case MainTab::Map:
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
            case MainTab::Events:
            {
                // can only paste events to this tab
                if (pasteObject["object"].toString() != "events") {
                    return;
                }

                QList<Event *> newEvents;

                QJsonArray events = pasteObject["events"].toArray();
                for (QJsonValue event : events) {
                    // paste the event to the map
                    const QString typeString = event["event_type"].toString();
                    Event::Type type = Event::eventTypeFromString(typeString);

                    if (this->editor->eventLimitReached(type)) {
                        logWarn(QString("Cannot paste event, the limit for type '%1' has been reached.").arg(typeString));
                        continue;
                    }
                    if (type == Event::Type::HealLocation) {
                        logWarn(QString("Cannot paste events of type '%1'").arg(typeString));
                        continue;
                    }

                    Event *pasteEvent = Event::create(type);
                    if (!pasteEvent)
                        continue;

                    pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                    pasteEvent->setMap(this->editor->map);
                    newEvents.append(pasteEvent);
                }

                if (!newEvents.empty()) {
                    editor->map->commit(new EventPaste(this->editor, editor->map, newEvents));
                    updateObjects();
                }

                break;
            }
        }
    }
}

void MainWindow::on_mapViewTab_tabBarClicked(int index)
{
    int oldIndex = ui->mapViewTab->currentIndex();
    ui->mapViewTab->setCurrentIndex(index);
    if (index != oldIndex)
        Scripting::cb_MapViewTabChanged(oldIndex, index);

    if (index == MapViewTab::Metatiles) {
        editor->setEditingMetatiles();
    } else if (index == MapViewTab::Collision) {
        editor->setEditingCollision();
    } else if (index == MapViewTab::Prefabs) {
        editor->setEditingMetatiles();
        if (projectConfig.prefabFilepath.isEmpty() && !projectConfig.prefabImportPrompted) {
            // User hasn't set up prefabs and hasn't been prompted before.
            // Ask if they'd like to import the default prefabs file.
            if (prefab.tryImportDefaultPrefabs(this, projectConfig.baseGameVersion))
                prefab.updatePrefabUi(this->editor->layout);
        }
    }
    editor->setCursorRectVisible(false);
}

void MainWindow::on_mainTabBar_tabBarClicked(int index)
{
    int oldIndex = ui->mainTabBar->currentIndex();
    ui->mainTabBar->setCurrentIndex(index);
    if (index != oldIndex)
        Scripting::cb_MainTabChanged(oldIndex, index);

    static const QMap<int, int> tabIndexToStackIndex = {
        {MainTab::Map, 0},
        {MainTab::Events, 0},
        {MainTab::Header, 1},
        {MainTab::Connections, 2},
        {MainTab::WildPokemon, 3},
    };
    ui->mainStackedWidget->setCurrentIndex(tabIndexToStackIndex.value(index));

    if (index == MainTab::Map) {
        ui->stackedWidget_MapEvents->setCurrentIndex(0);
        on_mapViewTab_tabBarClicked(ui->mapViewTab->currentIndex());
        clickToolButtonFromEditAction(editor->mapEditAction);
    } else if (index == MainTab::Events) {
        ui->stackedWidget_MapEvents->setCurrentIndex(1);
        editor->setEditingObjects();
        clickToolButtonFromEditAction(editor->objectEditAction);
    } else if (index == MainTab::Connections) {
        editor->setEditingConnections();
    } else if (index == MainTab::WildPokemon) {
        editor->setEditingEncounters();
    }

    if (!editor->map) return;
    if (index != MainTab::WildPokemon) {
        if (editor->project && editor->project->wildEncountersLoaded)
            editor->saveEncounterTabData();
    }
    if (index != MainTab::Events) {
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
    porymapConfig.prettyCursors = ui->actionBetter_Cursors->isChecked();
    this->editor->settings->betterCursors = ui->actionBetter_Cursors->isChecked();
}

void MainWindow::on_actionPlayer_View_Rectangle_triggered()
{
    bool enabled = ui->actionPlayer_View_Rectangle->isChecked();
    porymapConfig.showPlayerView = enabled;
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
    porymapConfig.showCursorTile = enabled;
    this->editor->settings->cursorTileRectEnabled = enabled;
    if ((this->editor->map_item && this->editor->map_item->has_mouse)
     || (this->editor->collision_item && this->editor->collision_item->has_mouse)) {
        this->editor->cursorMapTileRect->setVisible(enabled && this->editor->cursorMapTileRect->getActive());
        ui->graphicsView_Map->scene()->update();
    }
}

void MainWindow::on_actionShow_Grid_triggered() {
    this->editor->toggleGrid(ui->actionShow_Grid->isChecked());
}

void MainWindow::on_actionGrid_Settings_triggered() {
    if (!this->gridSettingsDialog) {
        this->gridSettingsDialog = new GridSettingsDialog(&this->editor->gridSettings, this);
        connect(this->gridSettingsDialog, &GridSettingsDialog::changedGridSettings, this->editor, &Editor::updateMapGrid);
    }
    openSubWindow(this->gridSettingsDialog);
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
            editor->selectMapEvent(object);
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

void MainWindow::tryAddEventTab(QWidget * tab) {
    auto group = getEventGroupFromTabWidget(tab);
    if (editor->map->getNumEvents(group))
        ui->tabWidget_EventType->addTab(tab, QString("%1s").arg(Event::eventGroupToString(group)));
}

void MainWindow::displayEventTabs() {
    const QSignalBlocker blocker(ui->tabWidget_EventType);

    ui->tabWidget_EventType->clear();
    tryAddEventTab(ui->tab_Objects);
    tryAddEventTab(ui->tab_Warps);
    tryAddEventTab(ui->tab_Triggers);
    tryAddEventTab(ui->tab_BGs);
    tryAddEventTab(ui->tab_Healspots);
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
    QList<DraggablePixmapItem *> events;

    if (editor->selected_events && editor->selected_events->length()) {
        events = *editor->selected_events;
    }
    else {
        QList<Event *> all_events;
        if (editor->map) {
            all_events = editor->map->getEvents();
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
            this->ui->spinner_ObjectID->setMaximum(current->getMap()->getNumEvents(eventGroup) + event_offs - 1);
            this->ui->spinner_ObjectID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Warp: {
            scrollTarget = ui->scrollArea_Warps;
            target = ui->scrollAreaWidgetContents_Warps;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Warps);

            QSignalBlocker b(this->ui->spinner_WarpID);
            this->ui->spinner_WarpID->setMinimum(event_offs);
            this->ui->spinner_WarpID->setMaximum(current->getMap()->getNumEvents(eventGroup) + event_offs - 1);
            this->ui->spinner_WarpID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Coord: {
            scrollTarget = ui->scrollArea_Triggers;
            target = ui->scrollAreaWidgetContents_Triggers;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Triggers);

            QSignalBlocker b(this->ui->spinner_TriggerID);
            this->ui->spinner_TriggerID->setMinimum(event_offs);
            this->ui->spinner_TriggerID->setMaximum(current->getMap()->getNumEvents(eventGroup) + event_offs - 1);
            this->ui->spinner_TriggerID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Bg: {
            scrollTarget = ui->scrollArea_BGs;
            target = ui->scrollAreaWidgetContents_BGs;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_BGs);

            QSignalBlocker b(this->ui->spinner_BgID);
            this->ui->spinner_BgID->setMinimum(event_offs);
            this->ui->spinner_BgID->setMaximum(current->getMap()->getNumEvents(eventGroup) + event_offs - 1);
            this->ui->spinner_BgID->setValue(current->getEventIndex() + event_offs);
            break;
        }
        case Event::Group::Heal: {
            scrollTarget = ui->scrollArea_Healspots;
            target = ui->scrollAreaWidgetContents_Healspots;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_Healspots);

            QSignalBlocker b(this->ui->spinner_HealID);
            this->ui->spinner_HealID->setMinimum(event_offs);
            this->ui->spinner_HealID->setMaximum(current->getMap()->getNumEvents(eventGroup) + event_offs - 1);
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

Event::Group MainWindow::getEventGroupFromTabWidget(QWidget *tab) {
    static const QMap<QWidget*,Event::Group> tabToGroup = {
        {ui->tab_Objects,   Event::Group::Object},
        {ui->tab_Warps,     Event::Group::Warp},
        {ui->tab_Triggers,  Event::Group::Coord},
        {ui->tab_BGs,       Event::Group::Bg},
        {ui->tab_Healspots, Event::Group::Heal},
    };
    return tabToGroup.value(tab, Event::Group::None);
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
            if (!selectedEvent && editor->map->getNumEvents(group)) {
                Event *event = editor->map->getEvent(group, 0);
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

void MainWindow::on_actionDive_Emerge_Map_triggered() {
    setDivingMapsVisible(ui->actionDive_Emerge_Map->isChecked());
}

void MainWindow::on_groupBox_DiveMapOpacity_toggled(bool on) {
    setDivingMapsVisible(on);
}

void MainWindow::setDivingMapsVisible(bool visible) {
    // Qt doesn't change the style of disabled sliders, so we do it ourselves
    QString stylesheet = visible ? "" : "QSlider::groove:horizontal {border: 1px solid #999999; border-radius: 3px; height: 2px; background: #B1B1B1;}"
                                        "QSlider::handle:horizontal {border: 1px solid #444444; border-radius: 3px; width: 10px; height: 9px; margin: -5px -1px; background: #5C5C5C; }";
    ui->slider_DiveEmergeMapOpacity->setStyleSheet(stylesheet);
    ui->slider_DiveMapOpacity->setStyleSheet(stylesheet);
    ui->slider_EmergeMapOpacity->setStyleSheet(stylesheet);

    // Sync UI toggle elements
    const QSignalBlocker blocker1(ui->groupBox_DiveMapOpacity);
    const QSignalBlocker blocker2(ui->actionDive_Emerge_Map);
    ui->groupBox_DiveMapOpacity->setChecked(visible);
    ui->actionDive_Emerge_Map->setChecked(visible);

    porymapConfig.showDiveEmergeMaps = visible;

    if (visible) {
        // We skip rendering diving maps if this setting is not enabled,
        // so when we enable it we need to make sure they've rendered.
        this->editor->renderDivingConnections();
    }
    this->editor->updateDivingMapsVisibility();
}

// Normally a map only has either a Dive map connection or an Emerge map connection,
// in which case we only show a single opacity slider to modify the one in use.
// If a user has both connections we show two separate opacity sliders so they can
// modify them independently.
void MainWindow::on_slider_DiveEmergeMapOpacity_valueChanged(int value) {
    porymapConfig.diveEmergeMapOpacity = value;
    this->editor->updateDivingMapsVisibility();
}

void MainWindow::on_slider_DiveMapOpacity_valueChanged(int value) {
    porymapConfig.diveMapOpacity = value;
    this->editor->updateDivingMapsVisibility();
}

void MainWindow::on_slider_EmergeMapOpacity_valueChanged(int value) {
    porymapConfig.emergeMapOpacity = value;
    this->editor->updateDivingMapsVisibility();
}

void MainWindow::on_horizontalSlider_CollisionTransparency_valueChanged(int value) {
    this->editor->collisionOpacity = static_cast<qreal>(value) / 100;
    porymapConfig.collisionOpacity = value;
    this->editor->collision_item->draw(true);
}

void MainWindow::on_toolButton_Paint_clicked()
{
    if (ui->mainTabBar->currentIndex() == MainTab::Map)
        editor->mapEditAction = Editor::EditAction::Paint;
    else
        editor->objectEditAction = Editor::EditAction::Paint;

    editor->settings->mapCursor = QCursor(QPixmap(":/icons/pencil_cursor.ico"), 10, 10);

    if (ui->mapViewTab->currentIndex() != MapViewTab::Collision)
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
    if (ui->mainTabBar->currentIndex() == MainTab::Map)
        editor->mapEditAction = Editor::EditAction::Select;
    else
        editor->objectEditAction = Editor::EditAction::Select;

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
    if (ui->mainTabBar->currentIndex() == MainTab::Map)
        editor->mapEditAction = Editor::EditAction::Fill;
    else
        editor->objectEditAction = Editor::EditAction::Fill;

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
    if (ui->mainTabBar->currentIndex() == MainTab::Map)
        editor->mapEditAction = Editor::EditAction::Pick;
    else
        editor->objectEditAction = Editor::EditAction::Pick;

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
    if (ui->mainTabBar->currentIndex() == MainTab::Map)
        editor->mapEditAction = Editor::EditAction::Move;
    else
        editor->objectEditAction = Editor::EditAction::Move;

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
    if (ui->mainTabBar->currentIndex() == MainTab::Map)
        editor->mapEditAction = Editor::EditAction::Shift;
    else
        editor->objectEditAction = Editor::EditAction::Shift;

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
    Editor::EditAction editAction;
    if (ui->mainTabBar->currentIndex() == MainTab::Map) {
        editAction = editor->mapEditAction;
    } else {
        editAction = editor->objectEditAction;
        if (editAction == Editor::EditAction::Select && editor->map_ruler)
            editor->map_ruler->setEnabled(true);
        else if (editor->map_ruler)
            editor->map_ruler->setEnabled(false);
    }

    ui->toolButton_Paint->setChecked(editAction == Editor::EditAction::Paint);
    ui->toolButton_Select->setChecked(editAction == Editor::EditAction::Select);
    ui->toolButton_Fill->setChecked(editAction == Editor::EditAction::Fill);
    ui->toolButton_Dropper->setChecked(editAction == Editor::EditAction::Pick);
    ui->toolButton_Move->setChecked(editAction == Editor::EditAction::Move);
    ui->toolButton_Shift->setChecked(editAction == Editor::EditAction::Shift);
}

void MainWindow::clickToolButtonFromEditAction(Editor::EditAction editAction) {
    if (editAction == Editor::EditAction::Paint) {
        on_toolButton_Paint_clicked();
    } else if (editAction == Editor::EditAction::Select) {
        on_toolButton_Select_clicked();
    } else if (editAction == Editor::EditAction::Fill) {
        on_toolButton_Fill_clicked();
    } else if (editAction == Editor::EditAction::Pick) {
        on_toolButton_Dropper_clicked();
    } else if (editAction == Editor::EditAction::Move) {
        on_toolButton_Move_clicked();
    } else if (editAction == Editor::EditAction::Shift) {
        on_toolButton_Shift_clicked();
    }
}

void MainWindow::onOpenConnectedMap(MapConnection *connection) {
    if (!connection)
        return;
    if (userSetMap(connection->targetMapName()))
        editor->setSelectedConnection(connection->findMirror());
}

void MainWindow::onLayoutChanged(Layout *) {
    updateMapList();
}

void MainWindow::onMapLoaded(Map *map) {
    connect(map, &Map::modified, [this, map] { this->markSpecificMapEdited(map); });
}

void MainWindow::onTilesetsSaved(QString primaryTilesetLabel, QString secondaryTilesetLabel) {
    // If saved tilesets are currently in-use, update them and redraw
    // Otherwise overwrite the cache for the saved tileset
    bool updated = false;
    if (primaryTilesetLabel == this->editor->layout->tileset_primary_label) {
        this->editor->updatePrimaryTileset(primaryTilesetLabel, true);
        Scripting::cb_TilesetUpdated(primaryTilesetLabel);
        updated = true;
    } else {
        this->editor->project->getTileset(primaryTilesetLabel, true);
    }
    if (secondaryTilesetLabel == this->editor->layout->tileset_secondary_label)  {
        this->editor->updateSecondaryTileset(secondaryTilesetLabel, true);
        Scripting::cb_TilesetUpdated(secondaryTilesetLabel);
        updated = true;
    } else {
        this->editor->project->getTileset(secondaryTilesetLabel, true);
    }
    if (updated)
        redrawMapScene();
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
    if (!this->editor->map) {
        QMessageBox warning(this);
        warning.setText("Notice");
        warning.setInformativeText("Map stitch images are not possible without a map selected.");
        warning.setStandardButtons(QMessageBox::Ok);
        warning.setDefaultButton(QMessageBox::Cancel);
        warning.setIcon(QMessageBox::Warning);

        warning.exec();
        return;
    }
    showExportMapImageWindow(ImageExporterMode::Stitch);
}

void MainWindow::on_actionExport_Map_Timelapse_Image_triggered() {
    showExportMapImageWindow(ImageExporterMode::Timelapse);
}

void MainWindow::on_actionImport_Map_from_Advance_Map_1_92_triggered() {
    QString filepath = FileDialog::getOpenFileName(this, "Import Map from Advance Map 1.92", "", "Advance Map 1.92 Map Files (*.map)");
    if (filepath.isEmpty()) {
        return;
    }

    bool error = false;
    Layout *mapLayout = AdvanceMapParser::parseLayout(filepath, &error, editor->project);
    if (error) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import map from Advance Map 1.92 .map file.");
        QString message = QString("The .map file could not be processed. View porymap.log for specific errors.");
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        delete mapLayout;
        return;
    }

    auto dialog = new NewLayoutDialog(this->editor->project, mapLayout, this);
    connect(dialog, &NewLayoutDialog::applied, this, &MainWindow::userSetLayout);
    connect(dialog, &NewLayoutDialog::finished, [mapLayout] { mapLayout->deleteLater(); });
    dialog->open();
}

void MainWindow::showExportMapImageWindow(ImageExporterMode mode) {
    if (!editor->project) return;

    // If the user is requesting this window again we assume it's for a new
    // window (the map/mode may have changed), so delete the old window.
    if (this->mapImageExporter)
        delete this->mapImageExporter;

    this->mapImageExporter = new MapImageExporter(this, this->editor, mode);

    openSubWindow(this->mapImageExporter);
}

void MainWindow::on_pushButton_AddConnection_clicked() {
    if (!this->editor || !this->editor->map || !this->editor->project)
        return;

    auto dialog = new NewMapConnectionDialog(this, this->editor->map, this->editor->project->mapNames);
    connect(dialog, &NewMapConnectionDialog::accepted, this->editor, &Editor::addConnection);
    dialog->open();
}

void MainWindow::on_pushButton_NewWildMonGroup_clicked() {
    editor->addNewWildMonGroup(this);
}

void MainWindow::on_pushButton_DeleteWildMonGroup_clicked() {
    editor->deleteWildMonGroup();
}

void MainWindow::on_pushButton_SummaryChart_clicked() {
    if (!this->wildMonChart) {
        this->wildMonChart = new WildMonChart(this, this->editor->getCurrentWildMonTable());
        connect(this->editor, &Editor::wildMonTableOpened, this->wildMonChart, &WildMonChart::setTable);
        connect(this->editor, &Editor::wildMonTableClosed, this->wildMonChart, &WildMonChart::clearTable);
        connect(this->editor, &Editor::wildMonTableEdited, this->wildMonChart, &WildMonChart::refresh);
    }
    openSubWindow(this->wildMonChart);
}

void MainWindow::on_pushButton_ConfigureEncountersJSON_clicked() {
    editor->configureEncounterJSON(this);
}

void MainWindow::on_button_OpenDiveMap_clicked() {
    userSetMap(ui->comboBox_DiveMap->currentText());
}

void MainWindow::on_button_OpenEmergeMap_clicked() {
    userSetMap(ui->comboBox_EmergeMap->currentText());
}

void MainWindow::on_comboBox_DiveMap_currentTextChanged(const QString &mapName) {
    // Include empty names as an update (user is deleting the connection)
    if (mapName.isEmpty() || editor->project->mapNames.contains(mapName))
        editor->updateDiveMap(mapName);
}

void MainWindow::on_comboBox_EmergeMap_currentTextChanged(const QString &mapName) {
    if (mapName.isEmpty() || editor->project->mapNames.contains(mapName))
        editor->updateEmergeMap(mapName);
}

void MainWindow::on_comboBox_PrimaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->primaryTilesetLabels.contains(tilesetLabel) && editor->layout) {
        editor->updatePrimaryTileset(tilesetLabel);
        redrawMapScene();
        on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->layout);
        markMapEdited();
    }
}

void MainWindow::on_comboBox_SecondaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->secondaryTilesetLabels.contains(tilesetLabel) && editor->layout) {
        editor->updateSecondaryTileset(tilesetLabel);
        redrawMapScene();
        on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->layout);
        markMapEdited();
    }
}

void MainWindow::on_pushButton_ChangeDimensions_clicked() {
    if (!editor || !editor->layout) return;

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
    widthSpinBox->setValue(editor->layout->getWidth());
    heightSpinBox->setValue(editor->layout->getHeight());
    bwidthSpinBox->setValue(editor->layout->getBorderWidth());
    bheightSpinBox->setValue(editor->layout->getBorderHeight());
    if (projectConfig.useCustomBorderSize) {
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
                    "The maximum layout width and height is the following: (width + 15) * (height + 14) <= %1\n"
                    "The specified layout width and height was: (%2 + 15) * (%3 + 14) = %4")
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
        Layout *layout = editor->layout;
        Blockdata oldMetatiles = layout->blockdata;
        Blockdata oldBorder = layout->border;
        QSize oldMapDimensions(layout->getWidth(), layout->getHeight());
        QSize oldBorderDimensions(layout->getBorderWidth(), layout->getBorderHeight());
        QSize newMapDimensions(widthSpinBox->value(), heightSpinBox->value());
        QSize newBorderDimensions(bwidthSpinBox->value(), bheightSpinBox->value());
        if (oldMapDimensions != newMapDimensions || oldBorderDimensions != newBorderDimensions) {
            layout->setDimensions(newMapDimensions.width(), newMapDimensions.height(), true, true);
            layout->setBorderDimensions(newBorderDimensions.width(), newBorderDimensions.height(), true, true);
            editor->layout->editHistory.push(new ResizeLayout(layout,
                oldMapDimensions, newMapDimensions,
                oldMetatiles, layout->blockdata,
                oldBorderDimensions, newBorderDimensions,
                oldBorder, layout->border
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
    editor->toggleBorderVisibility(selected != 0);
}

void MainWindow::on_checkBox_MirrorConnections_stateChanged(int selected)
{
    porymapConfig.mirrorConnectingMaps = (selected == Qt::Checked);
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
    this->tilesetEditor = new TilesetEditor(this->editor->project, this->editor->layout, this);
    connect(this->tilesetEditor, &TilesetEditor::tilesetsSaved, this, &MainWindow::onTilesetsSaved);
}

MapListToolBar* MainWindow::getCurrentMapListToolBar() {
    switch (ui->mapListContainer->currentIndex()) {
    case MapListTab::Groups:  return ui->mapListToolBar_Groups;
    case MapListTab::Areas:   return ui->mapListToolBar_Areas;
    case MapListTab::Layouts: return ui->mapListToolBar_Layouts;
    default: return nullptr;
    }
}

MapTree* MainWindow::getCurrentMapList() {
    auto toolbar = getCurrentMapListToolBar();
    if (toolbar)
        return toolbar->list();
    return nullptr;
}

// Clear the search filters on all the map lists.
// When the search filter is cleared the map lists will (if possible) display the currently-selected map/layout.
void MainWindow::resetMapListFilters() {
    ui->mapListToolBar_Groups->clearFilter();
    ui->mapListToolBar_Areas->clearFilter();
    ui->mapListToolBar_Layouts->clearFilter();
}

void MainWindow::mapListShortcut_ExpandAll() {
    auto toolbar = getCurrentMapListToolBar();
    if (toolbar) toolbar->expandList();
}

void MainWindow::mapListShortcut_CollapseAll() {
    auto toolbar = getCurrentMapListToolBar();
    if (toolbar) toolbar->collapseList();
}

void MainWindow::mapListShortcut_ToggleEmptyFolders() {
    auto toolbar = getCurrentMapListToolBar();
    if (toolbar) toolbar->toggleEmptyFolders();
}

void MainWindow::on_actionAbout_Porymap_triggered()
{
    if (!this->aboutWindow)
        this->aboutWindow = new AboutPorymap(this);
    openSubWindow(this->aboutWindow);
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
    if (porymapConfig.textEditorOpenFolder.isEmpty())
        ui->actionOpen_Project_in_Text_Editor->setEnabled(false);
    else
        ui->actionOpen_Project_in_Text_Editor->setEnabled(true);

    if (this->updatePromoter)
        this->updatePromoter->updatePreferences();
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
    this->openProjectSettingsEditor(porymapConfig.projectSettingsTab);
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
    Scripting::populateGlobalObject(this);
    // Lying to the scripts here, simulating a project reload
    Scripting::cb_ProjectOpened(projectConfig.projectDir);
    if (editor && editor->map)
        Scripting::cb_MapOpened(editor->map->name()); // TODO: API should have equivalent for layout
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
    porymapConfig.metatilesZoom = value;
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

    ui->scrollAreaWidgetContents_MetatileSelector->adjustSize();
    ui->scrollAreaWidgetContents_BorderMetatiles->adjustSize();

    redrawMetatileSelection();
    scrollMetatileSelectorToSelection();
}

void MainWindow::on_horizontalSlider_CollisionZoom_valueChanged(int value) {
    porymapConfig.collisionZoom = value;
    double scale = pow(3.0, static_cast<double>(value - 30) / 30.0);

    QTransform transform;
    transform.scale(scale, scale);
    QSize size(editor->movement_permissions_selector_item->pixmap().width(),
               editor->movement_permissions_selector_item->pixmap().height());
    size *= scale;

    ui->graphicsView_Collision->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Collision->setTransform(transform);
    ui->graphicsView_Collision->setFixedSize(size.width() + 2, size.height() + 2);
    ui->scrollAreaWidgetContents_Collision->adjustSize();
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
    PrefabCreationDialog dialog(this, this->editor->metatile_selector_item, this->editor->layout);
    dialog.setWindowTitle("Create Prefab");
    dialog.setWindowModality(Qt::NonModal);
    if (dialog.exec() == QDialog::Accepted) {
        dialog.savePrefab();
    }
}

bool MainWindow::initRegionMapEditor(bool silent) {
    this->regionMapEditor = new RegionMapEditor(this, this->editor->project);
    if (!this->regionMapEditor->load(silent)) {
        // The region map editor either failed to load,
        // or the user declined configuring their settings.
        if (!silent && this->regionMapEditor->setupErrored()) {
            if (this->askToFixRegionMapEditor())
                return true;
        }
        delete this->regionMapEditor;
        return false;
    }

    return true;
}

bool MainWindow::askToFixRegionMapEditor() {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(QString("There was an error opening the region map data. Please see %1 for full error details.").arg(getLogPath()));
    msgBox.setDetailedText(getMostRecentError());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    auto reconfigButton = msgBox.addButton("Reconfigure", QMessageBox::ActionRole);
    msgBox.exec();
    if (msgBox.clickedButton() == reconfigButton) {
        if (this->regionMapEditor->reconfigure()) {
            // User fixed error
            return true;
        }
        if (this->regionMapEditor->setupErrored()) {
            // User's new settings still fail, show error and ask again
            return this->askToFixRegionMapEditor();
        }
    }
    // User accepted error
    return false;
}

void MainWindow::clearOverlay() {
    if (ui->graphicsView_Map)
        ui->graphicsView_Map->clearOverlayMap();
}

// Attempt to close any open sub-windows of the main window, giving each a chance to abort the process.
// Each of these windows is a widget with WA_DeleteOnClose set, so manually deleting them isn't necessary.
// Because they're tracked with QPointers nullifying them shouldn't be necessary either, but it seems the
// delete is happening too late and some of the pointers haven't been cleared by the time we need them to,
// so we nullify them all here anyway.
bool MainWindow::closeSupplementaryWindows() {
    if (this->tilesetEditor && !this->tilesetEditor->close())
        return false;
    this->tilesetEditor = nullptr;

    if (this->regionMapEditor && !this->regionMapEditor->close())
        return false;
    this->regionMapEditor = nullptr;

    if (this->mapImageExporter && !this->mapImageExporter->close())
        return false;
    this->mapImageExporter = nullptr;

    if (this->shortcutsEditor && !this->shortcutsEditor->close())
        return false;
    this->shortcutsEditor = nullptr;

    if (this->preferenceEditor && !this->preferenceEditor->close())
        return false;
    this->preferenceEditor = nullptr;

    if (this->customScriptsEditor && !this->customScriptsEditor->close())
        return false;
    this->customScriptsEditor = nullptr;

    if (this->wildMonChart && !this->wildMonChart->close())
        return false;
    this->wildMonChart = nullptr;

    if (this->projectSettingsEditor) this->projectSettingsEditor->closeQuietly();
    this->projectSettingsEditor = nullptr;

    return true;
}

bool MainWindow::closeProject() {
    if (!closeSupplementaryWindows())
        return false;

    if (!isProjectOpen())
        return true;

    if (this->editor->project->hasUnsavedChanges()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this, "porymap", "The project has been modified, save changes?",
            QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

        if (result == QMessageBox::Yes) {
            editor->saveProject();
        } else if (result == QMessageBox::No) {
            logWarn("Closing project with unsaved changes.");
        } else if (result == QMessageBox::Cancel) {
            return false;
        }
    }
    editor->closeProject();
    clearProjectUI();
    setWindowDisabled(true);
    updateWindowTitle();

    return true;
}

void MainWindow::on_action_Exit_triggered() {
    if (!closeProject())
        return;
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!closeProject()) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
