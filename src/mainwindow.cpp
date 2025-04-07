#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "project.h"
#include "log.h"
#include "editor.h"
#include "prefabcreationdialog.h"
#include "eventframes.h"
#include "bordermetatilespixmapitem.h"
#include "currentselectedmetatilespixmapitem.h"
#include "customattributesframe.h"
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
#include "resizelayoutpopup.h"
#include "newmapdialog.h"
#include "newtilesetdialog.h"
#include "newmapgroupdialog.h"
#include "newlocationdialog.h"
#include "message.h"

#include <QClipboard>
#include <QDirIterator>
#include <QStandardItemModel>
#include <QSpinBox>
#include <QTextEdit>
#include <QSpacerItem>
#include <QFont>
#include <QScrollBar>
#include <QPushButton>
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



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isProgrammaticEventTabChange(false)
{
    QCoreApplication::setOrganizationName("pret");
    QCoreApplication::setApplicationName("porymap");
    QCoreApplication::setApplicationVersion(PORYMAP_VERSION);
    QApplication::setApplicationDisplayName(QApplication::applicationName());
    QApplication::setWindowIcon(QIcon(":/icons/porymap-icon-2.ico"));
    connect(qApp, &QApplication::applicationStateChanged, this, &MainWindow::showFileWatcherWarning);

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
    delete undoView;
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

#ifndef QT_CHARTS_LIB
    ui->pushButton_SummaryChart->setVisible(false);
#endif

    setWindowDisabled(true);
    show();
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
    connect(ui->actionDuplicate_Current_Map_Layout, &QAction::triggered, this, &MainWindow::openDuplicateMapOrLayoutDialog);
    connect(ui->comboBox_LayoutSelector->lineEdit(), &QLineEdit::editingFinished, this, &MainWindow::onLayoutSelectorEditingFinished);
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
    connect(this->editor, &Editor::eventsChanged, this, &MainWindow::updateEvents);
    connect(this->editor, &Editor::openConnectedMap, this, &MainWindow::onOpenConnectedMap);
    connect(this->editor, &Editor::openEventMap, this, &MainWindow::openEventMap);
    connect(this->editor, &Editor::currentMetatilesSelectionChanged, this, &MainWindow::currentMetatilesSelectionChanged);
    connect(this->editor, &Editor::wildMonTableEdited, this,  &MainWindow::markMapEdited);
    connect(this->editor, &Editor::mapRulerStatusChanged, this, &MainWindow::onMapRulerStatusChanged);
    connect(this->editor, &Editor::tilesetUpdated, this, &Scripting::cb_TilesetUpdated);
    connect(ui->newEventToolButton, &NewEventToolButton::newEventAdded, this->editor, &Editor::addNewEvent);
    connect(ui->toolButton_deleteEvent, &QAbstractButton::clicked, this->editor, &Editor::deleteSelectedEvents);

    this->loadUserSettings();

    undoAction = editor->editGroup.createUndoAction(this, tr("&Undo"));
    undoAction->setObjectName("action_Undo");
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));

    redoAction = editor->editGroup.createRedoAction(this, tr("&Redo"));
    redoAction->setObjectName("action_Redo");
    redoAction->setShortcuts({QKeySequence("Ctrl+Y"), QKeySequence("Ctrl+Shift+Z")});

    ui->menuEdit->addAction(undoAction);
    ui->menuEdit->addAction(redoAction);

    this->undoView = new QUndoView(&editor->editGroup);
    this->undoView->setWindowTitle(tr("Edit History"));
    this->undoView->setAttribute(Qt::WA_QuitOnClose, false);

    // Show the EditHistory dialog with Ctrl+E
    QAction *showHistory = new QAction("Show Edit History...", this);
    showHistory->setObjectName("action_ShowEditHistory");
    showHistory->setShortcut(QKeySequence("Ctrl+E"));
    connect(showHistory, &QAction::triggered, this, &MainWindow::openEditHistory);

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

void MainWindow::openEditHistory() {
    openSubWindow(this->undoView);
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
    QPushButton *buttonAdd = new QPushButton(QIcon(":/icons/add.ico"), "");
    buttonAdd->setToolTip("Create New Map");
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
    ui->mapListToolBar_Locations->setList(ui->locationList);
    ui->mapListToolBar_Layouts->setList(ui->layoutList);

    // Left-clicking on items in the map list opens the corresponding map/layout.
    connect(ui->mapList,      &QAbstractItemView::activated, this, &MainWindow::openMapListItem);
    connect(ui->locationList, &QAbstractItemView::activated, this, &MainWindow::openMapListItem);
    connect(ui->layoutList,   &QAbstractItemView::activated, this, &MainWindow::openMapListItem);

    // Right-clicking on items in the map list brings up a context menu.
    connect(ui->mapList,      &QTreeView::customContextMenuRequested, this, &MainWindow::onOpenMapListContextMenu);
    connect(ui->locationList, &QTreeView::customContextMenuRequested, this, &MainWindow::onOpenMapListContextMenu);
    connect(ui->layoutList,   &QTreeView::customContextMenuRequested, this, &MainWindow::onOpenMapListContextMenu);

    // Only the groups list allows reorganizing folder contents, editing folder names, etc.
    ui->mapListToolBar_Locations->setEditsAllowedButtonVisible(false);
    ui->mapListToolBar_Layouts->setEditsAllowedButtonVisible(false);

    // Initialize settings from config
    ui->mapListToolBar_Groups->setEditsAllowed(porymapConfig.mapListEditGroupsEnabled);
    for (auto i = porymapConfig.mapListHideEmptyEnabled.constBegin(); i != porymapConfig.mapListHideEmptyEnabled.constEnd(); i++) {
        auto toolbar = getMapListToolBar(i.key());
        if (toolbar) toolbar->setEmptyFoldersVisible(!i.value());
    }

    // Update config if map list settings change
    connect(ui->mapListToolBar_Groups, &MapListToolBar::editsAllowedChanged, [](bool allowed) {
        porymapConfig.mapListEditGroupsEnabled = allowed;
    });
    connect(ui->mapListToolBar_Groups, &MapListToolBar::emptyFoldersVisibleChanged, [](bool visible) {
        porymapConfig.mapListHideEmptyEnabled[MapListTab::Groups] = !visible;
    });
    connect(ui->mapListToolBar_Locations, &MapListToolBar::emptyFoldersVisibleChanged, [](bool visible) {
        porymapConfig.mapListHideEmptyEnabled[MapListTab::Locations] = !visible;
    });
    connect(ui->mapListToolBar_Layouts, &MapListToolBar::emptyFoldersVisibleChanged, [](bool visible) {
        porymapConfig.mapListHideEmptyEnabled[MapListTab::Layouts] = !visible;
    });

    // When map list search filter is cleared we want the current map/layout in the editor to be visible in the list.
    connect(ui->mapListToolBar_Groups,    &MapListToolBar::filterCleared, this, &MainWindow::scrollMapListToCurrentMap);
    connect(ui->mapListToolBar_Locations, &MapListToolBar::filterCleared, this, &MainWindow::scrollMapListToCurrentMap);
    connect(ui->mapListToolBar_Layouts,   &MapListToolBar::filterCleared, this, &MainWindow::scrollMapListToCurrentLayout);

    // Connect the "add folder" button in each of the map lists
    connect(ui->mapListToolBar_Groups,    &MapListToolBar::addFolderClicked, this, &MainWindow::openNewMapGroupDialog);
    connect(ui->mapListToolBar_Locations, &MapListToolBar::addFolderClicked, this, &MainWindow::openNewLocationDialog);
    connect(ui->mapListToolBar_Layouts,   &MapListToolBar::addFolderClicked, this, &MainWindow::openNewLayoutDialog);

    connect(ui->mapListContainer, &QTabWidget::currentChanged, this, &MainWindow::onMapListTabChanged);
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

void MainWindow::markLayoutEdited() {
    if (!this->editor->layout)
        return;
    this->editor->layout->hasUnsavedDataChanges = true;

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

    // Grid
    const QSignalBlocker b_Grid(ui->checkBox_ToggleGrid);
    ui->actionShow_Grid->setChecked(porymapConfig.showGrid);
    ui->checkBox_ToggleGrid->setChecked(porymapConfig.showGrid);

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

    ui->checkBox_MirrorConnections->setChecked(porymapConfig.mirrorConnectingMaps);
    ui->checkBox_ToggleBorder->setChecked(porymapConfig.showBorder);
    ui->actionShow_Events_In_Map_View->setChecked(porymapConfig.eventOverlayEnabled);

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
    connect(project, &Project::tilesetCreated, this, &MainWindow::onNewTilesetCreated);
    connect(project, &Project::mapGroupAdded, this, &MainWindow::onNewMapGroupCreated);
    connect(project, &Project::mapSectionAdded, this, &MainWindow::onNewMapSectionCreated);
    connect(project, &Project::mapSectionDisplayNameChanged, this, &MainWindow::onMapSectionDisplayNameChanged);
    connect(project, &Project::mapsExcluded, this, &MainWindow::showMapsExcludedAlert);
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

    ErrorMessage msgBox(QStringLiteral("The selected directory appears to be invalid."), this);
    msgBox.setInformativeText(QString("The directory '%1' is missing key files.\n\n"
                                      "Make sure you selected the correct project directory "
                                      "(the one used to make your .gba file, e.g. 'pokeemerald').").arg(editor->project->root));
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
    RecentErrorMessage::show(QStringLiteral("There was an error opening the project."), this);
}

// Alert the user that one or more maps have been excluded while loading the project.
void MainWindow::showMapsExcludedAlert(const QStringList &excludedMapNames) {
    RecentErrorMessage msgBox("", this);
    if (excludedMapNames.length() == 1) {
        msgBox.setText(QString("Failed to load map '%1'. Saving will exclude this map from your project.").arg(excludedMapNames.first()));
    } else {
        msgBox.setText(QStringLiteral("Failed to load the maps listed below. Saving will exclude these maps from your project."));
        msgBox.setDetailedText(excludedMapNames.join("\n")); // Overwrites error details text, user will need to check the log.
    }
    msgBox.exec();
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

void MainWindow::showFileWatcherWarning() {
     if (!porymapConfig.monitorFiles || !isProjectOpen())
        return;

    // Only show the file watcher warning if Porymap is the currently active application.
    // This stops Porymap from bugging users when they switch to their projects to make edits;
    // we'll warn them about the need to reload when they return to Porymap.
    if (QGuiApplication::applicationState() != Qt::ApplicationActive)
        return;

    Project *project = this->editor->project;
    QStringList modifiedFiles(project->modifiedFiles.constBegin(),  project->modifiedFiles.constEnd());
    if (modifiedFiles.isEmpty())
        return;
    project->modifiedFiles.clear();

    // Only allow one of these warnings at a single time.
    // Additional file changes are ignored while the warning is already active. 
    static bool showing = false;
    if (showing)
        return;
    showing = true;

    // Strip project root from filepaths
    const QString root = project->root + "/";
    for (auto &path : modifiedFiles) {
        path.remove(root);
    }

    QuestionMessage msgBox("", this);
    if (modifiedFiles.count() == 1) {
        msgBox.setText(QString("The file %1 has changed on disk. Would you like to reload the project?").arg(modifiedFiles.first()));
    } else {
        msgBox.setText(QStringLiteral("Some project files have changed on disk. Would you like to reload the project?"));
        msgBox.setDetailedText(QStringLiteral("The following files have changed:\n") + modifiedFiles.join("\n"));
    }

    QCheckBox showAgainCheck("Do not ask again.");
    msgBox.setCheckBox(&showAgainCheck);

    auto reply = msgBox.exec();
    if (reply == QMessageBox::Yes) {
        on_action_Reload_Project_triggered();
    } else if (reply == QMessageBox::No) {
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

    if (map_name.isEmpty()) {
        WarningMessage::show(QStringLiteral("Cannot open map with empty name."), this);
        return false;
    }

    if (map_name == editor->project->getDynamicMapName()) {
        WarningMessage msgBox(QString("Cannot open map '%1'.").arg(map_name), this);
        msgBox.setInformativeText(QStringLiteral("This map name is a placeholder to indicate that the warp's map will be set programmatically."));
        msgBox.exec();
        return false;
    }

    if (!setMap(map_name)) {
        RecentErrorMessage::show(QString("There was an error opening map '%1'.").arg(map_name), this);
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

    // If the map's MAPSEC / layout changes, update the map's position in the map list.
    // These are doing more work than necessary, rather than rebuilding the entire list they should find and relocate the appropriate row.
    connect(editor->map, &Map::layoutChanged, this, &MainWindow::rebuildMapList_Layouts, Qt::UniqueConnection);
    connect(editor->map->header(), &MapHeader::locationChanged, this, &MainWindow::rebuildMapList_Locations, Qt::UniqueConnection);

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
    this->ui->mainTabBar->setTabEnabled(MainTab::WildPokemon, mapEditingEnabled && editor->project->wildEncountersLoaded);

    this->ui->comboBox_LayoutSelector->setEnabled(mapEditingEnabled);
}

// setLayout, but with a visible error message in case of failure.
// Use when the user is specifically requesting a layout to open.
bool MainWindow::userSetLayout(QString layoutId) {
    if (!setLayout(layoutId)) {
        RecentErrorMessage::show(QString("There was an error opening layout '%1'.").arg(layoutId), this);
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
    editor->displayLayout();
    editor->displayMap();
    refreshMapScene();
}

void MainWindow::refreshMapScene() {
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

    on_mainTabBar_tabBarClicked(ui->mainTabBar->currentIndex());
}

void MainWindow::refreshMetatileViews() {
    on_horizontalSlider_MetatileZoom_valueChanged(ui->horizontalSlider_MetatileZoom->value());
}

void MainWindow::refreshCollisionSelector() {
    on_horizontalSlider_CollisionZoom_valueChanged(ui->horizontalSlider_CollisionZoom->value());
}

// Some events (like warps) have data that refers to an event on a different map.
// This function opens that map, and selects the event it's referring to.
void MainWindow::openEventMap(Event *sourceEvent) {
    if (!sourceEvent || !this->editor->map) return;

    QString targetMapName;
    QString targetEventIdName;
    Event::Group targetEventGroup;

    Event::Type eventType = sourceEvent->getEventType();
    if (eventType == Event::Type::Warp) {
        // Warp events open to their destination warp event.
        WarpEvent *warp = dynamic_cast<WarpEvent *>(sourceEvent);
        targetMapName = warp->getDestinationMap();
        targetEventIdName = warp->getDestinationWarpID();
        targetEventGroup = Event::Group::Warp;
    } else if (eventType == Event::Type::CloneObject) {
        // Clone object events open to their target object event.
        CloneObjectEvent *clone = dynamic_cast<CloneObjectEvent *>(sourceEvent);
        targetMapName = clone->getTargetMap();
        targetEventIdName = clone->getTargetID();
        targetEventGroup = Event::Group::Object;
    } else if (eventType == Event::Type::SecretBase) {
        // Secret Bases open to their secret base entrance
        const QString mapPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_map_prefix);
        SecretBaseEvent *base = dynamic_cast<SecretBaseEvent *>(sourceEvent);
        QString baseId = base->getBaseID();
        targetMapName = this->editor->project->mapConstantsToMapNames.value(mapPrefix + baseId.left(baseId.lastIndexOf("_")));
        targetEventIdName = "0";
        targetEventGroup = Event::Group::Warp;
    } else if (eventType == Event::Type::HealLocation && projectConfig.healLocationRespawnDataEnabled) {
        // Heal location events open to their respawn NPC
        HealLocationEvent *heal = dynamic_cast<HealLocationEvent *>(sourceEvent);
        targetMapName = heal->getRespawnMapName();
        targetEventIdName = heal->getRespawnNPC();
        targetEventGroup = Event::Group::Object;
    } else {
        // Other event types have no target map to open.
        return;
    }
    if (!userSetMap(targetMapName))
        return;

    // Map opened successfully, now try to select the targeted event on that map.
    Event* targetEvent = this->editor->map->getEvent(targetEventGroup, targetEventIdName);
    if (targetEvent) {
        this->editor->selectMapEvent(targetEvent);
    } else {
        // Can still warp to this map, but can't select the specified event
        logWarn(QString("%1 '%2' doesn't exist on map '%3'").arg(Event::groupToString(targetEventGroup)).arg(targetEventIdName).arg(targetMapName));
    }
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

    ui->mapCustomAttributesFrame->table()->setAttributes(editor->map->customAttributes());
}

void MainWindow::on_comboBox_LayoutSelector_currentTextChanged(const QString &text) {
    if (!this->editor || !this->editor->project || !this->editor->map)
        return;

    if (!this->editor->project->mapLayouts.contains(text)) {
        // User may be in the middle of typing the name of a layout, don't bother trying to load it.
        return;
    }

    Layout* layout = this->editor->project->loadLayout(text);
    if (!layout) {
        RecentErrorMessage::show(QString("Unable to set layout '%1'.").arg(text), this);

        // New layout failed to load, restore previous layout
        const QSignalBlocker b(ui->comboBox_LayoutSelector);
        ui->comboBox_LayoutSelector->setCurrentText(this->editor->map->layout()->id);
        return;
    }
    this->editor->map->setLayout(layout);
    setMap(this->editor->map->name());
    markMapEdited();
}

void MainWindow::onLayoutSelectorEditingFinished() {
    if (!this->editor || !this->editor->project || !this->editor->layout)
        return;

    // If the user left the layout selector in an invalid state, restore it so that it displays the current layout.
    const QString text = ui->comboBox_LayoutSelector->currentText();
    if (!this->editor->project->mapLayouts.contains(text)) {
        const QSignalBlocker b(ui->comboBox_LayoutSelector);
        ui->comboBox_LayoutSelector->setCurrentText(this->editor->layout->id);
    }
}

// Update the UI using information we've read from the user's project files.
bool MainWindow::setProjectUI() {
    Project *project = editor->project;

    this->mapHeaderForm->setProject(project);

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

    ui->newEventToolButton->setEventTypeVisible(Event::Type::WeatherTrigger, projectConfig.eventWeatherTriggerEnabled);
    ui->newEventToolButton->setEventTypeVisible(Event::Type::SecretBase, projectConfig.eventSecretBaseEnabled);
    ui->newEventToolButton->setEventTypeVisible(Event::Type::CloneObject, projectConfig.eventCloneObjectEnabled);

    editor->setCollisionGraphics();
    ui->spinBox_SelectedElevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_SelectedCollision->setMaximum(Block::getMaxCollision());

    // map models
    this->mapGroupModel = new MapGroupModel(editor->project);
    this->groupListProxyModel = new FilterChildrenProxyModel();
    this->groupListProxyModel->setSourceModel(this->mapGroupModel);
    this->groupListProxyModel->setHideEmpty(porymapConfig.mapListHideEmptyEnabled[MapListTab::Groups]);
    ui->mapList->setModel(groupListProxyModel);

    this->ui->mapList->setItemDelegateForColumn(0, new GroupNameDelegate(this->editor->project, this));
    connect(this->mapGroupModel, &MapGroupModel::dragMoveCompleted, this->ui->mapList, &MapTree::removeSelected);

    this->mapLocationModel = new MapLocationModel(editor->project);
    this->locationListProxyModel = new FilterChildrenProxyModel();
    this->locationListProxyModel->setSourceModel(this->mapLocationModel);
    this->locationListProxyModel->setHideEmpty(porymapConfig.mapListHideEmptyEnabled[MapListTab::Locations]);

    ui->locationList->setModel(locationListProxyModel);
    ui->locationList->sortByColumn(0, Qt::SortOrder::AscendingOrder);

    this->layoutTreeModel = new LayoutTreeModel(editor->project);
    this->layoutListProxyModel = new FilterChildrenProxyModel();
    this->layoutListProxyModel->setSourceModel(this->layoutTreeModel);
    this->layoutListProxyModel->setHideEmpty(porymapConfig.mapListHideEmptyEnabled[MapListTab::Layouts]);
    ui->layoutList->setModel(layoutListProxyModel);
    ui->layoutList->sortByColumn(0, Qt::SortOrder::AscendingOrder);

    ui->mapCustomAttributesFrame->table()->setRestrictedKeys(project->getTopLevelMapFields());

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
    delete this->mapLocationModel;
    delete this->locationListProxyModel;
    delete this->layoutTreeModel;
    delete this->layoutListProxyModel;
    resetMapListFilters();
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
    QAction* copyListNameAction = nullptr;
    QAction* copyToolTipAction = nullptr;

    if (itemType == "map_name") {
        // Right-clicking on a map.
        openItemAction = menu.addAction("Open Map");
        menu.addSeparator();
        copyListNameAction = menu.addAction("Copy Map Name");
        copyToolTipAction = menu.addAction("Copy Map ID");
        menu.addSeparator();
        connect(menu.addAction("Duplicate Map"), &QAction::triggered, [this, itemName] {
            openDuplicateMapDialog(itemName);
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
        addToFolderAction = menu.addAction("Add New Map to Location");
        menu.addSeparator();
        copyListNameAction = menu.addAction("Copy Location ID Name");
        copyToolTipAction = menu.addAction("Copy Location In-Game Name");
        menu.addSeparator();
        deleteFolderAction = menu.addAction("Delete Location");
        if (itemName == this->editor->project->getEmptyMapsecName())
            deleteFolderAction->setEnabled(false); // Disallow deleting the default name
    } else if (itemType == "map_layout") {
        // Right-clicking on a map layout
        openItemAction = menu.addAction("Open Layout");
        menu.addSeparator();
        copyListNameAction = menu.addAction("Copy Layout Name");
        copyToolTipAction = menu.addAction("Copy Layout ID");
        menu.addSeparator();
        connect(menu.addAction("Duplicate Layout"), &QAction::triggered, [this, itemName] {
            openDuplicateLayoutDialog(itemName);
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
    if (copyListNameAction) {
        connect(copyListNameAction, &QAction::triggered, [this, selectedItem] {
            setClipboardData(selectedItem->text());
        });
    }
    if (copyToolTipAction) {
        connect(copyToolTipAction, &QAction::triggered, [this, selectedItem] {
            setClipboardData(selectedItem->toolTip());
        });
    }

    if (menu.actions().length() != 0)
        menu.exec(QCursor::pos());
}

void MainWindow::onNewMapCreated(Map *newMap, const QString &groupName) {
    logInfo(QString("Created a new map named %1.").arg(newMap->name()));

    if (newMap->needsHealLocation()) {
        this->editor->addNewEvent(Event::Type::HealLocation);
    }

    // Add new map to the map lists
    this->mapGroupModel->insertMapItem(newMap->name(), groupName);
    this->mapLocationModel->insertMapItem(newMap->name(), newMap->header()->location());
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
    // Add new map section to the Locations map list view
    this->mapLocationModel->insertMapFolderItem(idName);
}

void MainWindow::onMapSectionDisplayNameChanged(const QString &idName, const QString &displayName) {
    // Update the tool tip in the map list that shows the MAPSEC's in-game name.
    QStandardItem *item = this->mapLocationModel->itemAt(idName);
    if (item) item->setToolTip(displayName);
}

void MainWindow::onNewTilesetCreated(Tileset *tileset) {
    logInfo(QString("Created a new tileset named %1.").arg(tileset->name));

    // Refresh tileset combo boxes
    if (!tileset->is_secondary) {
        int index = this->editor->project->primaryTilesetLabels.indexOf(tileset->name);
        ui->comboBox_PrimaryTileset->insertItem(index, tileset->name);
    } else {
        int index = this->editor->project->secondaryTilesetLabels.indexOf(tileset->name);
        ui->comboBox_SecondaryTileset->insertItem(index, tileset->name);
    }
}

void MainWindow::openNewMapGroupDialog() {
    auto dialog = new NewMapGroupDialog(this->editor->project, this);
    dialog->open();
}

void MainWindow::openNewLocationDialog() {
    auto dialog = new NewLocationDialog(this->editor->project, this);
    dialog->open();
}

void MainWindow::openNewMapDialog() {
    auto dialog = new NewMapDialog(this->editor->project, this);
    dialog->open();
}

void MainWindow::openDuplicateMapDialog(const QString &mapName) {
    const Map *map = this->editor->project->loadMap(mapName);
    if (map) {
        auto dialog = new NewMapDialog(this->editor->project, map, this);
        dialog->open();
    } else {
        RecentErrorMessage::show(QString("Unable to duplicate '%1'.").arg(mapName), this);
    }
}

NewLayoutDialog* MainWindow::createNewLayoutDialog(const Layout *layoutToCopy) {
    auto dialog = new NewLayoutDialog(this->editor->project, layoutToCopy, this);
    connect(dialog, &NewLayoutDialog::applied, this, &MainWindow::userSetLayout);
    return dialog;
}

void MainWindow::openNewLayoutDialog() {
    auto dialog = createNewLayoutDialog();
    dialog->open();
}

void MainWindow::openDuplicateLayoutDialog(const QString &layoutId) {
    auto layout = this->editor->project->loadLayout(layoutId);
    if (layout) {
        auto dialog = createNewLayoutDialog(layout);
        dialog->open();
    } else {
        RecentErrorMessage::show(QString("Unable to duplicate '%1'.").arg(layoutId), this);
    }
}

void MainWindow::openDuplicateMapOrLayoutDialog() {
    if (this->editor->map) {
        openDuplicateMapDialog(this->editor->map->name());
    } else if (this->editor->layout) {
        openDuplicateLayoutDialog(this->editor->layout->id);
    }
}

void MainWindow::on_actionNew_Tileset_triggered() {
    auto dialog = new NewTilesetDialog(editor->project, this);
    connect(dialog, &NewTilesetDialog::applied, [this](Tileset *tileset) {
        // Unlike creating a new map or layout (which immediately opens the new item)
        // creating a new tileset has no visual feedback that it succeeded, so we show a message.
        // It's important that we do this after the dialog has closed (sheet modal dialogs on macOS don't seem to play nice together).
        InfoMessage::show(QString("New tileset created at '%1'!").arg(tileset->getExpectedDir()), this);
    });
    dialog->open();
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

void MainWindow::onMapListTabChanged(int index) {
    // Save current tab for future sessions.
    porymapConfig.mapListTab = index;

    // After changing a map list tab the old tab's search widget can keep focus, which isn't helpful
    // (and might be a little confusing to the user, because they don't know that each search bar is secretly a separate object).
    // When we change tabs we'll automatically focus in on the search bar. This should also make finding maps a little quicker.
    auto toolbar = getCurrentMapListToolBar();
    if (toolbar) toolbar->setSearchFocus();
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

void MainWindow::rebuildMapList_Locations() {
    this->mapLocationModel->deleteLater();
    this->mapLocationModel = new MapLocationModel(this->editor->project);
    this->locationListProxyModel->setSourceModel(this->mapLocationModel);
    resetMapListFilters();
}
void MainWindow::rebuildMapList_Layouts() {
    this->layoutTreeModel->deleteLater();
    this->layoutTreeModel = new LayoutTreeModel(this->editor->project);
    this->layoutListProxyModel->setSourceModel(this->layoutTreeModel);
    resetMapListFilters();
}

void MainWindow::updateMapList() {
    // Get the name of the open map/layout (or clear the relevant selection if there is none).
    QString activeItemName; 
    if (this->editor->map) {
        activeItemName = this->editor->map->name();
    } else {
        ui->mapList->clearSelection();
        ui->locationList->clearSelection();

        if (this->editor->layout) {
            activeItemName = this->editor->layout->id;
        } else {
            ui->layoutList->clearSelection();
        }
    }

    this->mapGroupModel->setActiveItem(activeItemName);
    this->mapLocationModel->setActiveItem(activeItemName);
    this->layoutTreeModel->setActiveItem(activeItemName);

    emit this->groupListProxyModel->layoutChanged();
    emit this->locationListProxyModel->layoutChanged();
    emit this->layoutListProxyModel->layoutChanged();
}

void MainWindow::on_action_Save_Project_triggered() {
    save(false);
}

void MainWindow::on_action_Save_triggered() {
    save(true);
}

void MainWindow::save(bool currentOnly) {
    if (currentOnly) {
        this->editor->saveCurrent();
    } else {
        this->editor->saveAll();
    }
    updateWindowTitle();
    updateMapList();

    if (!porymapConfig.shownInGameReloadMessage) {
        // Show a one-time warning that the user may need to reload their map to see their new changes.
        InfoMessage::show(QStringLiteral("Reload your map in-game!\n\nIf your game is currently saved on a map you have edited, "
                                         "the changes may not appear until you leave the map and return."),
                          this);
        porymapConfig.shownInGameReloadMessage = true;
    }

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

                OrderedJson::array eventsArray;
                for (const auto &event : this->editor->selectedEvents) {
                    OrderedJson::object eventContainer;
                    eventContainer["event_type"] = Event::typeToJsonKey(event->getEventType());
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
    object["application"] = QApplication::applicationName();
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
        if (pasteObject["application"].toString() != QApplication::applicationName()) {
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
                    Event::Type type = Event::typeFromJsonKey(event["event_type"].toString());
                    Event *pasteEvent = Event::create(type);
                    if (!pasteEvent)
                        continue;

                    pasteEvent->loadFromJson(event["event"].toObject(), this->editor->project);
                    pasteEvent->setMap(this->editor->map);
                    newEvents.append(pasteEvent);
                }
                if (newEvents.empty())
                    return;

                if (!this->editor->canAddEvents(newEvents)) {
                    WarningMessage::show(QStringLiteral("Unable to paste, the maximum number of events would be exceeded."), this);
                    qDeleteAll(newEvents);
                    return;
                }
                this->editor->map->commit(new EventPaste(this->editor, this->editor->map, newEvents));
                updateEvents();
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

    static const QMap<int, Editor::EditMode> tabIndexToEditMode = {
        {MapViewTab::Metatiles, Editor::EditMode::Metatiles},
        {MapViewTab::Collision, Editor::EditMode::Collision},
        {MapViewTab::Prefabs,   Editor::EditMode::Metatiles},
    };
    if (tabIndexToEditMode.contains(index)) {
        editor->setEditMode(tabIndexToEditMode.value(index));
    }

    if (index == MapViewTab::Metatiles) {
        refreshMetatileViews();
    } else if (index == MapViewTab::Collision) {
        refreshCollisionSelector();
    } else if (index == MapViewTab::Prefabs) {
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

    static const QMap<int, Editor::EditMode> tabIndexToEditMode = {
        // MainTab::Map itself has no edit mode, depends on mapViewTab.
        {MainTab::Events,      Editor::EditMode::Events},
        {MainTab::Header,      Editor::EditMode::Header},
        {MainTab::Connections, Editor::EditMode::Connections},
        {MainTab::WildPokemon, Editor::EditMode::Encounters},
    };
    if (tabIndexToEditMode.contains(index)) {
        editor->setEditMode(tabIndexToEditMode.value(index));
    }

    if (index == MainTab::Map) {
        ui->stackedWidget_MapEvents->setCurrentIndex(0);
        on_mapViewTab_tabBarClicked(ui->mapViewTab->currentIndex());
        clickToolButtonFromEditAction(editor->mapEditAction);
    } else if (index == MainTab::Events) {
        ui->stackedWidget_MapEvents->setCurrentIndex(1);
        clickToolButtonFromEditAction(editor->eventEditAction);
    } else if (index == MainTab::Connections) {
        ui->graphicsView_Connections->setFocus(); // Avoid opening tab with focus on something editable
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

void MainWindow::on_actionShow_Events_In_Map_View_triggered() {
    porymapConfig.eventOverlayEnabled = ui->actionShow_Events_In_Map_View->isChecked();
    this->editor->redrawAllEvents();
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

void MainWindow::tryAddEventTab(QWidget * tab) {
    auto group = getEventGroupFromTabWidget(tab);
    if (this->editor->map && this->editor->map->getNumEvents(group))
        ui->tabWidget_EventType->addTab(tab, QString("%1s").arg(Event::groupToString(group)));
}

void MainWindow::displayEventTabs() {
    const QSignalBlocker blocker(ui->tabWidget_EventType);

    ui->tabWidget_EventType->clear();
    tryAddEventTab(ui->tab_Objects);
    tryAddEventTab(ui->tab_Warps);
    tryAddEventTab(ui->tab_Triggers);
    tryAddEventTab(ui->tab_BGs);
    tryAddEventTab(ui->tab_HealLocations);
}

void MainWindow::updateEvents() {
    if (this->editor->map) {
        for (auto i = this->lastSelectedEvent.begin(); i != this->lastSelectedEvent.end(); i++) {
            if (i.value() && !this->editor->map->hasEvent(i.value()))
                this->lastSelectedEvent.insert(i.key(), nullptr);
        }
    }
    displayEventTabs();
    updateSelectedEvents();
}

void MainWindow::updateSelectedEvents() {
    QList<Event*> events;

    if (!this->editor->selectedEvents.isEmpty()) {
        events = this->editor->selectedEvents;
    }
    else {
        QList<Event *> allEvents;
        if (this->editor->map) {
            allEvents = this->editor->map->getEvents();
        }
        if (!allEvents.isEmpty()) {
            Event *selectedEvent = allEvents.first();
            if (selectedEvent) {
                this->editor->selectedEvents.append(selectedEvent);
                this->editor->redrawEventPixmapItem(selectedEvent->getPixmapItem());
                events.append(selectedEvent);
            }
        }
    }

    QScrollArea *scrollTarget = ui->scrollArea_Multiple;
    QWidget *target = ui->scrollAreaWidgetContents_Multiple;

    this->isProgrammaticEventTabChange = true;

    if (events.length() == 1) {
        // single selected event case
        Event *current = events.constFirst();
        Event::Group eventGroup = current->getEventGroup();
        int event_offs = Event::getIndexOffset(eventGroup);

        if (eventGroup != Event::Group::None) {
            this->lastSelectedEvent.insert(eventGroup, current);
        }

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
            scrollTarget = ui->scrollArea_HealLocations;
            target = ui->scrollAreaWidgetContents_HealLocations;
            ui->tabWidget_EventType->setCurrentWidget(ui->tab_HealLocations);

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

    if (!events.isEmpty()) {
        // Set the 'New Event' button to be the type of the most recently-selected event
        ui->newEventToolButton->selectEventType(events.constLast()->getEventType());
    }

    this->isProgrammaticEventTabChange = false;

    QList<QFrame *> frames;
    for (auto event : events) {
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
        {ui->tab_Objects,       Event::Group::Object},
        {ui->tab_Warps,         Event::Group::Warp},
        {ui->tab_Triggers,      Event::Group::Coord},
        {ui->tab_BGs,           Event::Group::Bg},
        {ui->tab_HealLocations, Event::Group::Heal},
    };
    return tabToGroup.value(tab, Event::Group::None);
}

void MainWindow::eventTabChanged(int index) {
    if (editor->map) {
        Event::Group group = getEventGroupFromTabWidget(ui->tabWidget_EventType->widget(index));
        Event *selectedEvent = this->lastSelectedEvent.value(group, nullptr);
        if (!isProgrammaticEventTabChange) {
            if (!selectedEvent) selectedEvent = this->editor->map->getEvent(group, 0);
            this->editor->selectMapEvent(selectedEvent);
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
        editor->eventEditAction = Editor::EditAction::Paint;

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
        editor->eventEditAction = Editor::EditAction::Select;

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
        editor->eventEditAction = Editor::EditAction::Fill;

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
        editor->eventEditAction = Editor::EditAction::Pick;

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
        editor->eventEditAction = Editor::EditAction::Move;

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
        editor->eventEditAction = Editor::EditAction::Shift;

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
        editAction = editor->eventEditAction;
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
        WarningMessage::show(QStringLiteral("Map stitch images are not possible without a map selected."), this);
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
        RecentErrorMessage::show(QStringLiteral("Failed to import map from Advance Map 1.92 .map file."), this);
        delete mapLayout;
        return;
    }

    auto dialog = createNewLayoutDialog(mapLayout);
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

void MainWindow::on_toolButton_WildMonSearch_clicked() {
    if (!this->wildMonSearch) {
        this->wildMonSearch = new WildMonSearch(this->editor->project, this);
        connect(this->wildMonSearch, &WildMonSearch::openWildMonTableRequested, this, &MainWindow::openWildMonTable);
        connect(this->editor, &Editor::wildMonTableEdited, this->wildMonSearch, &WildMonSearch::refresh);
    }
    openSubWindow(this->wildMonSearch);
}

void MainWindow::openWildMonTable(const QString &mapName, const QString &groupName, const QString &fieldName) {
    if (userSetMap(mapName)) {
        // Switch to the correct main tab, wild encounter group, and wild encounter type tab.
        on_mainTabBar_tabBarClicked(MainTab::WildPokemon);
        ui->comboBox_EncounterGroupLabel->setCurrentText(groupName);
        QWidget *w = ui->stackedWidget_WildMons->currentWidget();
        if (w) static_cast<MonTabWidget *>(w)->setCurrentField(fieldName);
    }
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
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->layout);
        markLayoutEdited();
    }
}

void MainWindow::on_comboBox_SecondaryTileset_currentTextChanged(const QString &tilesetLabel)
{
    if (editor->project->secondaryTilesetLabels.contains(tilesetLabel) && editor->layout) {
        editor->updateSecondaryTileset(tilesetLabel);
        redrawMapScene();
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->layout);
        markLayoutEdited();
    }
}

void MainWindow::on_pushButton_ChangeDimensions_clicked() {
    if (!editor || !editor->layout) return;

    ResizeLayoutPopup popup(this->ui->graphicsView_Map, this->editor);
    popup.show();
    popup.setupLayoutView();
    if (popup.exec() == QDialog::Accepted) {
        Layout *layout = this->editor->layout;
        Map *map = this->editor->map;

        QMargins result = popup.getResult();
        QSize borderResult = popup.getBorderResult();
        QSize oldLayoutDimensions(layout->getWidth(), layout->getHeight());
        QSize oldBorderDimensions(layout->getBorderWidth(), layout->getBorderHeight());
        if (!result.isNull() || (borderResult != oldBorderDimensions)) {
            Blockdata oldMetatiles = layout->blockdata;
            Blockdata oldBorder = layout->border;

            layout->adjustDimensions(result);
            layout->setBorderDimensions(borderResult.width(), borderResult.height(), true, true);
            layout->editHistory.push(new ResizeLayout(layout,
                oldLayoutDimensions, result,
                oldMetatiles, layout->blockdata,
                oldBorderDimensions, borderResult,
                oldBorder, layout->border
            ));
        }
        // If we're in map-editing mode, adjust the events' position by the same amount.
        if (map) {
            auto events = map->getEvents();
            int deltaX = result.left();
            int deltaY = result.top();
            if ((deltaX || deltaY) && !events.isEmpty()) {
                map->commit(new EventShift(events, deltaX, deltaY, this->editor->eventShiftActionId++));
            }
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

MapListToolBar* MainWindow::getMapListToolBar(int tab) {
    switch (tab) {
    case MapListTab::Groups:    return ui->mapListToolBar_Groups;
    case MapListTab::Locations: return ui->mapListToolBar_Locations;
    case MapListTab::Layouts:   return ui->mapListToolBar_Layouts;
    default: return nullptr;
    }
}

MapListToolBar* MainWindow::getCurrentMapListToolBar() {
    return getMapListToolBar(ui->mapListContainer->currentIndex());
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
    ui->mapListToolBar_Locations->clearFilter();
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
        connect(preferenceEditor, &PreferenceEditor::themeChanged, this, &MainWindow::setTheme);
        connect(preferenceEditor, &PreferenceEditor::themeChanged, editor, &Editor::maskNonVisibleConnectionTiles);
        connect(preferenceEditor, &PreferenceEditor::preferencesSaved, this, &MainWindow::togglePreferenceSpecificUi);
        // Changes to porymapConfig.loadAllEventScripts or porymapConfig.eventSelectionShapeMode
        // require us to repopulate the EventFrames and redraw event pixmaps, respectively.
        connect(preferenceEditor, &PreferenceEditor::preferencesSaved, editor, &Editor::updateEvents);
        connect(preferenceEditor, &PreferenceEditor::scriptSettingsChanged, editor->project, &Project::readEventScriptLabels);
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
    static const QString informative = QStringLiteral(
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

    InfoMessage msgBox(QStringLiteral("Warp Events only function as exits on certain metatiles"), this);
    auto settingsButton = msgBox.addButton("Open Settings...", QMessageBox::ActionRole);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setInformativeText(informative);
    msgBox.exec();
    if (msgBox.clickedButton() == settingsButton)
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
    if (this->editor) {
        if (this->editor->layout)
            Scripting::cb_LayoutOpened(this->editor->layout->name);
        if (this->editor->map)
            Scripting::cb_MapOpened(this->editor->map->name());
    }
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
    RecentErrorMessage msgBox(QStringLiteral("There was an error opening the region map data."), this);
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
    #define SAFE_CLOSE(window) \
        do { \
            if ((window) && !(window)->close()) \
                return false; \
            window = nullptr; \
        } while (0);

    SAFE_CLOSE(this->tilesetEditor);
    SAFE_CLOSE(this->regionMapEditor);
    SAFE_CLOSE(this->mapImageExporter);
    SAFE_CLOSE(this->shortcutsEditor);
    SAFE_CLOSE(this->preferenceEditor);
    SAFE_CLOSE(this->customScriptsEditor);
    SAFE_CLOSE(this->wildMonChart);
    SAFE_CLOSE(this->wildMonSearch);

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
        QuestionMessage msgBox(QStringLiteral("The project has been modified, save changes?"), this);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);

        auto reply = msgBox.exec();
        if (reply == QMessageBox::Yes) {
            save();
        } else if (reply == QMessageBox::No) {
            logWarn("Closing project with unsaved changes.");
        } else if (reply == QMessageBox::Cancel) {
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
