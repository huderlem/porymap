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
#include "eventpixmapitem.h"
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
#include "loadingscreen.h"

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
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
#define RELEASE_PLATFORM
#endif
#if defined(QT_NETWORK_LIB) && defined(RELEASE_PLATFORM)
#define USE_UPDATE_PROMOTER
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

    logInit();
    logInfo(QString("Launching Porymap v%1 (%2)").arg(QCoreApplication::applicationVersion()).arg(QStringLiteral(PORYMAP_LATEST_COMMIT)));
    logInfo(QString("Using Qt v%2 (%3)").arg(QStringLiteral(QT_VERSION_STR)).arg(QSysInfo::buildCpuArchitecture()));
}

void MainWindow::initialize() {
    this->initWindow();
    if (porymapConfig.reopenOnLaunch && !porymapConfig.projectManuallyClosed && this->openProject(porymapConfig.getRecentProject(), true)) {
        on_toolButton_Paint_clicked();
    }

    // there is a bug affecting macOS users, where the trackpad deilveres a bad touch-release gesture
    // the warning is a bit annoying, so it is disabled here
    QLoggingCategory::setFilterRules(QStringLiteral("qt.pointer.dispatch=false"));

    if (porymapConfig.checkForUpdates)
        this->checkForUpdates(false);

    this->restoreWindowState();
    this->show();
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

    // Disabling the central widget above sets focus to the map list's search bar,
    // which prevents users from using keyboard shortcuts for menu actions.
    // Rather than explicitly set the tab order to give focus to something else
    // (which is a bit of a pain to set up and maintain) we just make sure focus
    // is on something that (mostly) ignores the keyboard.
    ui->graphicsView_Map->setFocus();
}

void MainWindow::initWindow() {
    porymapConfig.load();
    this->initLogStatusBar();
    this->initCustomUI();
    this->initExtraSignals();
    this->initEditor();
    this->initMiscHeapObjects();
    this->initMapList();
    this->initShortcuts();

    QStringList missingModules;

#ifndef USE_UPDATE_PROMOTER
    ui->actionCheck_for_Updates->setVisible(false);
#ifdef RELEASE_PLATFORM
    // Only report the network module missing if we would
    // have otherwise used it (we don't on non-release platforms).
    missingModules.append(" 'network'");
#endif
#endif

#ifndef QT_CHARTS_LIB
    ui->pushButton_SummaryChart->setVisible(false);
    missingModules.append(" 'charts'");
#endif

#ifndef QT_QML_LIB
    ui->actionCustom_Scripts->setVisible(false);
    missingModules.append(" 'qml'");
#endif

    if (!missingModules.isEmpty()) {
        logWarn(QString("Qt module%1%2 not found. Some features will be disabled.")
                            .arg(missingModules.length() > 1 ? "s" : "")
                            .arg(missingModules.join(",")));
    }

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

void MainWindow::initLogStatusBar() {
    removeLogStatusBar(this->statusBar());
    auto logTypes = QSet<LogType>(porymapConfig.statusBarLogTypes.begin(), porymapConfig.statusBarLogTypes.end());
    if (!logTypes.isEmpty()) {
        addLogStatusBar(this->statusBar(), logTypes);
    }
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
        {MainTab::Events, ProjectConfig::getPlayerIcon(BaseGameVersion::pokefirered, 0)}, // Arbitrary default
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

    // Center zooming on the mouse
    ui->graphicsView_Map->setTransformationAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);
    ui->graphicsView_Map->setResizeAnchor(QGraphicsView::ViewportAnchor::AnchorUnderMouse);
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
    connect(ui->comboBox_LayoutSelector, &NoScrollComboBox::editingFinished, this, &MainWindow::onLayoutSelectorEditingFinished);
    connect(ui->checkBox_smartPaths, &QCheckBox::toggled, this, &MainWindow::setSmartPathsEnabled);
    connect(ui->checkBox_ToggleBorder, &QCheckBox::toggled, this, &MainWindow::setBorderVisibility);
    connect(ui->checkBox_MirrorConnections, &QCheckBox::toggled, this, &MainWindow::setMirrorConnectionsEnabled);
    connect(ui->comboBox_PrimaryTileset, &NoScrollComboBox::editingFinished, [this] { setPrimaryTileset(ui->comboBox_PrimaryTileset->currentText()); });
    connect(ui->comboBox_SecondaryTileset, &NoScrollComboBox::editingFinished, [this] { setSecondaryTileset(ui->comboBox_SecondaryTileset->currentText()); });
}

void MainWindow::on_actionCheck_for_Updates_triggered() {
    checkForUpdates(true);
}

#ifdef USE_UPDATE_PROMOTER
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
        Util::show(this->updatePromoter);
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
    connect(this->editor, &Editor::wildMonTableEdited, [this] { markMapEdited(this->editor->map); });
    connect(this->editor, &Editor::mapRulerStatusChanged, this, &MainWindow::onMapRulerStatusChanged);
    connect(this->editor, &Editor::editActionSet, this, &MainWindow::setEditActionUi);
    connect(ui->newEventToolButton, &NewEventToolButton::newEventAdded, this->editor, &Editor::addNewEvent);
    connect(ui->toolButton_deleteEvent, &QAbstractButton::clicked, this->editor, &Editor::deleteSelectedEvents);
    connect(ui->graphicsView_Connections, &ConnectionsView::pressedDelete, this->editor, &Editor::removeSelectedConnection);

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
    Util::show(this->undoView);
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

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonFrame);
    buttonLayout->setSpacing(0);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    // Create add map/layout button
    QPushButton *buttonAdd = new QPushButton(QIcon(":/icons/add.ico"), "");
    buttonAdd->setToolTip("Create new map");
    connect(buttonAdd, &QPushButton::clicked, this, &MainWindow::openNewMapDialog);
    buttonLayout->addWidget(buttonAdd);

    /* TODO: Remove button disabled, no current support for deleting maps/layouts
    // Create remove map/layout button
    QPushButton *buttonRemove = new QPushButton(QIcon(":/icons/delete.ico"), "");
    connect(buttonRemove, &QPushButton::clicked, this, &MainWindow::deleteCurrentMapOrLayout);
    buttonLayout->addWidget(buttonRemove);
    */

    ui->mapListContainer->setCornerWidget(buttonFrame, Qt::TopRightCorner);

    // Navigation arrows
    auto navigationFrame = new QFrame(ui->mapListContainer);
    navigationFrame->setFrameShape(QFrame::NoFrame);

    auto navigationLayout = new QHBoxLayout(navigationFrame);
    navigationLayout->setSpacing(0);
    navigationLayout->setContentsMargins(0, 0, 0, 0);

    auto backArrow = new QToolButton(navigationFrame);
    backArrow->setArrowType(Qt::LeftArrow);
    backArrow->setToolTip("Open previous map");
    backArrow->setEnabled(false);
    connect(backArrow, &QToolButton::clicked, this, &MainWindow::openPreviousMap);
    connect(ui->actionBack, &QAction::triggered, this, &MainWindow::openPreviousMap);
    navigationLayout->addWidget(backArrow);
    this->backNavigation.button = backArrow;

    auto forwardArrow = new QToolButton(navigationFrame);
    forwardArrow->setArrowType(Qt::RightArrow);
    forwardArrow->setToolTip("Open next map");
    forwardArrow->setEnabled(false);
    connect(forwardArrow, &QToolButton::clicked, this, &MainWindow::openNextMap);
    connect(ui->actionForward, &QAction::triggered, this, &MainWindow::openNextMap);
    navigationLayout->addWidget(forwardArrow);
    this->forwardNavigation.button = forwardArrow;

    ui->mapListContainer->setCornerWidget(navigationFrame, Qt::TopLeftCorner);

    // Can't seem to get mapListContainer to update the size hint appropriately for
    // the new corner widgets. The default size hint is too small, and the tabs will elide.
    // For now we set an explicit minimum to fit both the tabs and buttons, and we make
    // the buttons as small as they can reasonably be to maximize space for the tabs.
    ui->mapListContainer->setMinimumWidth(285);
    buttonAdd->setFixedWidth(25);
    backArrow->setFixedWidth(15);
    forwardArrow->setFixedWidth(15);

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

    // When searching the Layouts list, don't expand to show all the maps. If the user is searching
    // this list then they're probably more interested in the layouts than the maps.
    ui->mapListToolBar_Layouts->setExpandListForSearch(false);

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

void MainWindow::markMapEdited(Map* map) {
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

    this->editor->gridSettings = porymapConfig.gridSettings;

    setTheme(porymapConfig.theme);
    setDivingMapsVisible(porymapConfig.showDiveEmergeMaps);
}

void MainWindow::restoreWindowState() {
    QMap<QString, QByteArray> geometry = porymapConfig.getMainGeometry();
    const QByteArray mainWindowGeometry = geometry.value("main_window_geometry");
    if (!mainWindowGeometry.isEmpty()) {
        logInfo("Restoring main window geometry from previous session.");
        restoreGeometry(mainWindowGeometry);
        restoreState(geometry.value("main_window_state"));
        ui->splitter_map->restoreState(geometry.value("map_splitter_state"));
        ui->splitter_main->restoreState(geometry.value("main_splitter_state"));
        ui->splitter_Metatiles->restoreState(geometry.value("metatiles_splitter_state"));
    }

    // Resize the window if it exceeds the available screen size.
    auto screen = windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    if (!screen) return;
    const QRect screenGeometry = screen->availableGeometry();
    if (this->width() > screenGeometry.width() || this->height() > screenGeometry.height()) {
        auto pixelRatio = screen->devicePixelRatio();
        logInfo(QString("Resizing main window. Dimensions of %1x%2 exceed available screen size of %3x%4")
                        .arg(qRound(this->width() * pixelRatio))
                        .arg(qRound(this->height() * pixelRatio))
                        .arg(qRound(screenGeometry.width() * pixelRatio))
                        .arg(qRound(screenGeometry.height() * pixelRatio)));
        resize(qMin(this->width(), screenGeometry.width()),
               qMin(this->height(), screenGeometry.height()));
        move(screenGeometry.center() - this->rect().center());
    }
}

void MainWindow::setTheme(QString theme) {
    QFile File(QString(":/themes/%1.qss").arg(theme));
    File.open(QFile::ReadOnly);
    QString stylesheet = QLatin1String(File.readAll());

    stylesheet.append(QString("QWidget { %1 } ").arg(Util::toStylesheetString(porymapConfig.applicationFont)));
    stylesheet.append(QString("MapTree { %1 } ").arg(Util::toStylesheetString(porymapConfig.mapListFont)));

    setStyleSheet(stylesheet);
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
    logInfo(openMessage);

    porysplash->start();

    porysplash->showLoadingMessage("config");
    if (!projectConfig.load(dir) || !userConfig.load(dir)) {
        showProjectOpenFailure();
        porysplash->stop();
        return false;
    }

    porysplash->showLoadingMessage("custom scripts");
    Scripting::init(this);

    // Create the project
    auto project = new Project(editor);
    project->setRoot(dir);
    connect(project, &Project::fileChanged, this, &MainWindow::showFileWatcherWarning);
    connect(project, &Project::mapLoaded, this, &MainWindow::onMapLoaded);
    connect(project, &Project::mapCreated, this, &MainWindow::onNewMapCreated);
    connect(project, &Project::layoutCreated, this, &MainWindow::onNewLayoutCreated);
    connect(project, &Project::tilesetCreated, this, &MainWindow::onNewTilesetCreated);
    connect(project, &Project::mapGroupAdded, this, &MainWindow::onNewMapGroupCreated);
    connect(project, &Project::mapSectionAdded, this, &MainWindow::onNewMapSectionCreated);
    connect(project, &Project::mapSectionDisplayNameChanged, this, &MainWindow::onMapSectionDisplayNameChanged);
    this->editor->setProject(project);

    // Make sure project looks reasonable before attempting to load it
    porysplash->showMessage("Verifying project");
    if (isInvalidProject(this->editor->project)) {
        delete this->editor->project;
        porysplash->stop();
        return false;
    }

    // Load the project
    if (!(loadProjectData() && setProjectUI() && setInitialMap())) {
        showProjectOpenFailure();
        delete this->editor->project;
        // TODO: Allow changing project settings at this point
        porysplash->stop();
        return false;
    }

    // Only create the config files once the project has opened successfully in case the user selected an invalid directory
    this->editor->project->saveConfig();
    
    updateWindowTitle();

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
    porysplash->stop();
    return true;
}

bool MainWindow::loadProjectData() {
    porysplash->showLoadingMessage("project");
    bool success = editor->project->load();
    Scripting::populateGlobalObject(this);
    return success;
}

bool MainWindow::isInvalidProject(Project *project) {
    return !(checkProjectSanity(project) && checkProjectVersion(project));
}

bool MainWindow::checkProjectSanity(Project *project) {
    if (project->sanityCheck())
        return true;

    logWarn(QString("The directory '%1' failed the project sanity check.").arg(project->root));

    ErrorMessage msgBox(QStringLiteral("The selected directory appears to be invalid."), porysplash);
    msgBox.setInformativeText(QString("The directory '%1' is missing key files.\n\n"
                                      "Make sure you selected the correct project directory "
                                      "(the one used to make your .gba file, e.g. 'pokeemerald').").arg(project->root));
    auto tryAnyway = msgBox.addButton("Try Anyway", QMessageBox::ActionRole);
    msgBox.exec();
    if (msgBox.clickedButton() == tryAnyway) {
        // The user has chosen to try to load this project anyway.
        // This will almost certainly fail, but they'll get a more specific error message.
        return true;
    }
    return false;
}

bool MainWindow::checkProjectVersion(Project *project) {
    QString error;
    int projectVersion = project->getSupportedMajorVersion(&error);
    if (projectVersion < 0) {
        // Failed to identify a supported major version.
        // We can't draw any conclusions from this, so we don't consider the project to be invalid.
        QString msg = QStringLiteral("Failed to check project version");
        logWarn(error.isEmpty() ? msg : QString("%1: '%2'").arg(msg).arg(error));
    } else {
        QString msg = QStringLiteral("Successfully checked project version. ");
        logInfo(msg + ((projectVersion != 0) ? QString("Supports at least Porymap v%1").arg(projectVersion)
                                             : QStringLiteral("Too old for any Porymap version")));

        if (projectVersion < porymapVersion.majorVersion() && projectConfig.forcedMajorVersion < porymapVersion.majorVersion()) {
            // We were unable to find the necessary changes for Porymap's current major version.
            // Unless they have explicitly suppressed this message, warn the user that this might mean their project is missing breaking changes.
            // Note: Do not report 'projectVersion' to the user in this message. We've already logged it for troubleshooting.
            //       It is very plausible that the user may have reproduced the required changes in an
            //       unknown commit, rather than merging the required changes directly from the base repo.
            //       In this case the 'projectVersion' may actually be too old to use for their repo.
            ErrorMessage msgBox(QStringLiteral("Your project may be incompatible!"), porysplash);
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setInformativeText(QString("Make sure '%1' has all the required changes for Porymap version %2.<br>"
                                              "See <a href=\"https://huderlem.github.io/porymap/manual/breaking-changes.html\">Breaking Changes</a> in the manual for more details")
                                              .arg(project->getProjectTitle())
                                              .arg(porymapVersion.majorVersion()));
            auto tryAnyway = msgBox.addButton("Try Anyway", QMessageBox::ActionRole);
            msgBox.exec();
            if (msgBox.clickedButton() != tryAnyway){
                return false;
            }
            // User opted to try with this version anyway. Don't warn them about this version again.
            projectConfig.forcedMajorVersion = porymapVersion.majorVersion();
        }
    }
    return true;
}

void MainWindow::showProjectOpenFailure() {
    if (!this->isVisible()){
        // The main window is not visible during the initial project open; the splash screen is busy providing visual feedback.
        // If project opening fails we can immediately display the empty main window (which we need anyway to parent messages to).
        restoreWindowState();
        show();
    }
    RecentErrorMessage::show(QStringLiteral("There was an error opening the project."), this);
}

bool MainWindow::isProjectOpen() {
    return editor && editor->project;
}

bool MainWindow::setInitialMap() {
    porysplash->showMessage("Opening initial map");

    const QString recent = userConfig.recentMapOrLayout;
    if (editor->project->isKnownMap(recent)) {
        // User recently had a map open that still exists.
        if (setMap(recent))
            return true;
    } else if (editor->project->isKnownLayout(recent)) {
        // User recently had a layout open that still exists.
        if (setLayout(recent))
            return true;
    }

    // Failed to open recent map/layout, or no recent map/layout. Try opening maps then layouts sequentially.
    for (const auto &name : editor->project->getMapNamesByGroup()) {
        if (name != recent && setMap(name))
            return true;
    }
    for (const auto &id : editor->project->layoutIds()) {
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
    if (this->fileWatcherWarning)
        return;
    this->fileWatcherWarning = new QuestionMessage("", this);
    this->fileWatcherWarning->setAttribute(Qt::WA_DeleteOnClose);
    this->fileWatcherWarning->setWindowModality(Qt::ApplicationModal);

    // Strip project root from filepaths
    const QString root = project->root + "/";
    for (auto &path : modifiedFiles) {
        path.remove(root);
    }

    if (modifiedFiles.count() == 1) {
        this->fileWatcherWarning->setText(QString("The file %1 has changed on disk. Would you like to reload the project?").arg(modifiedFiles.first()));
    } else {
        this->fileWatcherWarning->setText(QStringLiteral("Some project files have changed on disk. Would you like to reload the project?"));
        this->fileWatcherWarning->setDetailedText(QStringLiteral("The following files have changed:\n") + modifiedFiles.join("\n"));
    }

    this->fileWatcherWarning->setCheckBox(new QCheckBox("Do not ask again."));
    connect(this->fileWatcherWarning->checkBox(), &QCheckBox::toggled, [this](bool checked) {
        porymapConfig.monitorFiles = !checked;
        if (this->preferenceEditor)
            this->preferenceEditor->updateFields();
    });
    connect(this->fileWatcherWarning, &QuestionMessage::accepted, this, &MainWindow::on_action_Reload_Project_triggered);
    this->fileWatcherWarning->exec();
}

void MainWindow::on_action_Open_Project_triggered() {
    QString dir = FileDialog::getExistingDirectory(this, QStringLiteral("Choose Project Folder"));
    if (!dir.isEmpty())
        openProject(dir);
}

void MainWindow::on_action_Reload_Project_triggered() {
    if (this->editor && this->editor->project)
        openProject(this->editor->project->root);
}

void MainWindow::on_action_Close_Project_triggered() {
    closeProject();
    porymapConfig.projectManuallyClosed = true;
}

void MainWindow::unsetMap() {
    this->editor->unsetMap();
    setLayoutOnlyMode(true);
}

void MainWindow::openPreviousMap() {
    openMapFromHistory(true);
}

void MainWindow::openNextMap() {
    openMapFromHistory(false);
}

// Either open a map/layout from the 'Back' list and put it in the 'Forward' list (i.e., previous == true) or vice versa.
void MainWindow::openMapFromHistory(bool previous) {
    if (!this->editor->project)
        return;

    MapNavigation* popNavigation = (previous) ? &this->backNavigation : &this->forwardNavigation;
    MapNavigation* pushNavigation = (previous) ? &this->forwardNavigation : &this->backNavigation;
    if (popNavigation->stack.isEmpty())
        return;

    QString incomingItem = popNavigation->stack.top();
    QString outgoingItem = getActiveItemName();

    this->ignoreNavigationRecords = true;

    bool success = false;
    if (this->editor->project->isKnownMap(incomingItem)) {
        success = userSetMap(incomingItem);
    } else if (this->editor->project->isKnownLayout(incomingItem)) {
        success = userSetLayout(incomingItem);
    }
    if (success) {
        // We were successful in opening the map/layout, so we can remove it from the history.
        popNavigation->stack.pop();
        if (popNavigation->stack.isEmpty()) {
            popNavigation->button->setEnabled(false);
        }

        // Save the map/layout that was previously open.
        pushNavigation->stack.push(outgoingItem);
        pushNavigation->button->setEnabled(true);
    }

    this->ignoreNavigationRecords = false;
}

void MainWindow::recordMapNavigation(const QString &itemName) {
    if (this->ignoreNavigationRecords)
        return;

    this->backNavigation.stack.push(itemName);
    this->backNavigation.button->setEnabled(true);

    this->forwardNavigation.stack.clear();
    this->forwardNavigation.button->setEnabled(false);
}

void MainWindow::resetMapNavigation() {
    this->backNavigation.stack.clear();
    this->backNavigation.button->setEnabled(false);
    this->forwardNavigation.stack.clear();
    this->forwardNavigation.button->setEnabled(false);
}

// setMap, but with a visible error message in case of failure.
// Use when the user is specifically requesting a map to open.
bool MainWindow::userSetMap(const QString &mapName) {
    if (mapName.isEmpty()) {
        WarningMessage::show(QStringLiteral("Cannot open map with empty name."), this);
        return false;
    }

    if (mapName == editor->project->getDynamicMapName()) {
        auto msgBox = new WarningMessage(QString("Cannot open map '%1'.").arg(mapName), this);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->setInformativeText(QStringLiteral("This map name is a placeholder to indicate that the warp's map will be set programmatically."));
        msgBox->open();
        return false;
    }

    QString prevItem = getActiveItemName();
    if (prevItem == mapName)
        return true; // Already set

    if (!setMap(mapName)) {
        RecentErrorMessage::show(QString("There was an error opening map '%1'.").arg(mapName), this);
        return false;
    }
    recordMapNavigation(prevItem);
    return true;
}

bool MainWindow::setMap(const QString &mapName) {
    if (!editor || !editor->project || mapName.isEmpty() || mapName == editor->project->getDynamicMapName()) {
        logWarn(QString("Ignored setting map to '%1'").arg(mapName));
        return false;
    }

    logInfo(QString("Setting map to '%1'").arg(mapName));
    if (!editor->setMap(mapName)) {
        logWarn(QString("Failed to set map to '%1'").arg(mapName));
        return false;
    }

    setLayoutOnlyMode(false);
    this->lastSelectedEvent.clear();

    refreshMapScene();
    displayMapProperties();
    updateWindowTitle();
    updateMapList();
    scrollCurrentMapListToItem(mapName);

    // If the map's MAPSEC / layout changes, update the map's position in the map list.
    // These are doing more work than necessary, rather than rebuilding the entire list they should find and relocate the appropriate row.
    connect(editor->map, &Map::layoutChanged, this, &MainWindow::rebuildMapList_Layouts, Qt::UniqueConnection);
    connect(editor->map->header(), &MapHeader::locationChanged, this, &MainWindow::rebuildMapList_Locations, Qt::UniqueConnection);

    connect(editor->layout, &Layout::needsRedrawing, this, &MainWindow::redrawMapScene, Qt::UniqueConnection);

    userConfig.recentMapOrLayout = mapName;

    Scripting::cb_MapOpened(mapName);
    prefab.updatePrefabUi(editor->layout);
    updateTilesetEditor();

    emit mapOpened(editor->map);

    return true;
}

// These parts of the UI only make sense when editing maps.
// When editing in layout-only mode they are disabled.
void MainWindow::setLayoutOnlyMode(bool layoutOnly) {
    bool mapEditingEnabled = !layoutOnly;
    ui->mainTabBar->setTabEnabled(MainTab::Events, mapEditingEnabled);
    ui->mainTabBar->setTabEnabled(MainTab::Header, mapEditingEnabled);
    ui->mainTabBar->setTabEnabled(MainTab::Connections, mapEditingEnabled);
    ui->mainTabBar->setTabEnabled(MainTab::WildPokemon, mapEditingEnabled && this->editor->project->wildEncountersLoaded);

    // Set a tool tip to explain why the tabs are disabled.
    static const QString disabledToolTip = Util::toHtmlParagraph("You are in layout-only mode. This tab is only enabled when a map is open.");
    QString toolTip = mapEditingEnabled ? QString() : disabledToolTip;
    ui->mainTabBar->setTabToolTip(MainTab::Events, toolTip);
    ui->mainTabBar->setTabToolTip(MainTab::Header, toolTip);
    ui->mainTabBar->setTabToolTip(MainTab::Connections, toolTip);
    ui->mainTabBar->setTabToolTip(MainTab::WildPokemon, this->editor->project->wildEncountersLoaded ? toolTip : QString());

    ui->comboBox_LayoutSelector->setEnabled(mapEditingEnabled);
}

// setLayout, but with a visible error message in case of failure.
// Use when the user is specifically requesting a layout to open.
bool MainWindow::userSetLayout(const QString &layoutId) {
    if (layoutId.isEmpty()) {
        WarningMessage::show(QStringLiteral("Cannot open layout with empty ID."), this);
        return false;
    }

    QString prevItem = getActiveItemName();
    if (prevItem == layoutId)
        return true; // Already set

    if (!setLayout(layoutId)) {
        RecentErrorMessage::show(QString("There was an error opening layout '%1'.").arg(layoutId), this);
        return false;
    }

    recordMapNavigation(prevItem);

    // Only the Layouts tab of the map list shows Layouts, so if we're not already on that tab we'll open it now.
    ui->mapListContainer->setCurrentIndex(MapListTab::Layouts);

    return true;
}

bool MainWindow::setLayout(const QString &layoutId) {
    // Prefer logging the name of the layout as displayed in the map list.
    QString layoutName = this->editor->project ? this->editor->project->getLayoutName(layoutId) : QString();
    logInfo(QString("Setting layout to '%1'").arg(layoutName.isEmpty() ? layoutId : layoutName));

    if (!this->editor->setLayout(layoutId)) {
        return false;
    }

    if (this->editor->map)
        logInfo("Switching to layout-only editing mode. Disabling map-related edits.");

    unsetMap();
    refreshMapScene();
    updateWindowTitle();
    updateMapList();
    scrollCurrentMapListToItem(layoutId);

    connect(editor->layout, &Layout::needsRedrawing, this, &MainWindow::redrawMapScene, Qt::UniqueConnection);

    updateTilesetEditor();

    userConfig.recentMapOrLayout = layoutId;

    emit layoutOpened(editor->layout);

    return true;
}

// TODO: This is being used to do more work than necessary (e.g. when a layout is resized,
//       we don't need to be recreating the metatile selector, all the events, etc.)
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
        auto warp = dynamic_cast<WarpEvent *>(sourceEvent);
        targetMapName = warp->getDestinationMap();
        targetEventIdName = warp->getDestinationWarpID();
        targetEventGroup = Event::Group::Warp;
    } else if (eventType == Event::Type::CloneObject) {
        // Clone object events open to their target object event.
        auto clone = dynamic_cast<CloneObjectEvent *>(sourceEvent);
        targetMapName = clone->getTargetMap();
        targetEventIdName = clone->getTargetID();
        targetEventGroup = Event::Group::Object;
    } else if (eventType == Event::Type::SecretBase) {
        // Secret Bases open to their secret base entrance
        auto base = dynamic_cast<SecretBaseEvent *>(sourceEvent);

        // Extract the map name from the secret base ID.
        QString baseId = base->getBaseID();
        if (baseId.isEmpty())
            return;
        targetMapName = this->editor->project->secretBaseIdToMapName(baseId);
        if (targetMapName.isEmpty()) {
            WarningMessage::show(QString("Failed to determine which map '%1' refers to.").arg(baseId), this);
            return;
        }

        // Just select the first warp. Normally the only warp event on every secret base map is the entrance/exit, so this is usually correct.
        // The warp IDs for secret bases are specified in the project's C code, not in the map data, so we don't have an easy way to read the actual IDs.
        targetEventIdName = "0";
        targetEventGroup = Event::Group::Warp;
    } else if (eventType == Event::Type::HealLocation && projectConfig.healLocationRespawnDataEnabled) {
        // Heal location events open to their respawn NPC
        auto heal = dynamic_cast<HealLocationEvent *>(sourceEvent);
        targetMapName = heal->getRespawnMapName();
        targetEventIdName = heal->getRespawnNPC();
        targetEventGroup = Event::Group::Object;
    } else {
        // Other event types have no target map to open.
        return;
    }
    if (targetMapName.isEmpty() || !userSetMap(targetMapName))
        return;

    // Map opened successfully, now try to select the targeted event on that map.
    Event *targetEvent = this->editor->map->getEvent(targetEventGroup, targetEventIdName);
    this->editor->selectMapEvent(targetEvent);
}

void MainWindow::displayMapProperties() {
    this->mapHeaderForm->clear();
    if (!editor || !editor->map || !editor->project) {
        ui->frame_HeaderData->setEnabled(false);
        return;
    }
    ui->frame_HeaderData->setEnabled(true);
    this->mapHeaderForm->setHeader(editor->map->header());

    ui->mapCustomAttributesFrame->table()->setAttributes(editor->map->customAttributes());
}

void MainWindow::on_comboBox_LayoutSelector_currentTextChanged(const QString &text) {
    if (!this->editor || !this->editor->project || !this->editor->map)
        return;

    if (!this->editor->project->isKnownLayout(text)) {
        // User may be in the middle of typing the name of a layout, don't bother trying to load it.
        return;
    }

    Layout* layout = this->editor->project->loadLayout(text);
    if (!layout) {
        RecentErrorMessage::show(QString("Unable to set layout '%1'.").arg(text), this);

        // New layout failed to load, restore previous layout
        const QSignalBlocker b(ui->comboBox_LayoutSelector);
        ui->comboBox_LayoutSelector->setTextItem(this->editor->map->layoutId());
        return;
    }
    this->editor->map->setLayout(layout);
    setMap(this->editor->map->name());
    markMapEdited(this->editor->map);
}

void MainWindow::onLayoutSelectorEditingFinished() {
    if (!this->editor || !this->editor->project || !this->editor->layout)
        return;

    // If the user left the layout selector in an invalid state, restore it so that it displays the current layout.
    const QString text = ui->comboBox_LayoutSelector->currentText();
    if (!this->editor->project->isKnownLayout(text)) {
        const QSignalBlocker b(ui->comboBox_LayoutSelector);
        ui->comboBox_LayoutSelector->setTextItem(this->editor->layout->id);
    }
}

// Update the UI using information we've read from the user's project files.
bool MainWindow::setProjectUI() {
    porysplash->showLoadingMessage("project UI");

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
    ui->comboBox_LayoutSelector->addItems(project->layoutIds());

    const QSignalBlocker b_DiveMap(ui->comboBox_DiveMap);
    ui->comboBox_DiveMap->clear();
    ui->comboBox_DiveMap->addItems(project->mapNames());
    ui->comboBox_DiveMap->setClearButtonEnabled(true);
    ui->comboBox_DiveMap->setFocusedScrollingEnabled(false);

    const QSignalBlocker b_EmergeMap(ui->comboBox_EmergeMap);
    ui->comboBox_EmergeMap->clear();
    ui->comboBox_EmergeMap->addItems(project->mapNames());
    ui->comboBox_EmergeMap->setClearButtonEnabled(true);
    ui->comboBox_EmergeMap->setFocusedScrollingEnabled(false);

    // Show/hide parts of the UI that are dependent on the user's project settings

    // Wild Encounters tab
    ui->mainTabBar->setTabEnabled(MainTab::WildPokemon, editor->project->wildEncountersLoaded);

    ui->newEventToolButton->setEventTypeVisible(Event::Type::WeatherTrigger, projectConfig.eventWeatherTriggerEnabled);
    ui->newEventToolButton->setEventTypeVisible(Event::Type::SecretBase, projectConfig.eventSecretBaseEnabled);
    ui->newEventToolButton->setEventTypeVisible(Event::Type::CloneObject, projectConfig.eventCloneObjectEnabled);

    this->editor->setPlayerViewRect(QRectF(QPoint(0,0), Metatile::pixelSize()).marginsAdded(projectConfig.playerViewDistance));

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
    setMapListSorted(ui->locationList, porymapConfig.mapListLocationsSorted);

    this->layoutTreeModel = new LayoutTreeModel(editor->project);
    this->layoutListProxyModel = new FilterChildrenProxyModel();
    this->layoutListProxyModel->setSourceModel(this->layoutTreeModel);
    this->layoutListProxyModel->setHideEmpty(porymapConfig.mapListHideEmptyEnabled[MapListTab::Layouts]);
    ui->layoutList->setModel(layoutListProxyModel);
    setMapListSorted(ui->layoutList, porymapConfig.mapListLayoutsSorted);

    ui->mapCustomAttributesFrame->table()->setRestrictedKeys(project->getTopLevelMapFields());

    // Set a version dependent player icon (or user-chosen icon) for the Events tab.
    QIcon eventTabIcon;
    if (!projectConfig.eventsTabIconPath.isEmpty()) {
        eventTabIcon = QIcon(project->getExistingFilepath(projectConfig.eventsTabIconPath));
        if (eventTabIcon.isNull()) {
            logWarn(QString("Failed to load custom Events tab icon '%1'.").arg(projectConfig.eventsTabIconPath));
        }
    }
    if (eventTabIcon.isNull()) {
        // We randomly choose between the available characters for ~flavor~.
        // For now, this correctly assumes all versions have 2 icons.
        eventTabIcon = ProjectConfig::getPlayerIcon(projectConfig.baseGameVersion, QRandomGenerator::global()->bounded(0, 2));
    }
    ui->mainTabBar->setTabIcon(MainTab::Events, eventTabIcon);

    return true;
}

void MainWindow::clearProjectUI() {
    // Clear project comboboxes
    const QSignalBlocker b_PrimaryTileset(ui->comboBox_PrimaryTileset);
    ui->comboBox_PrimaryTileset->clear();

    const QSignalBlocker b_SecondaryTileset(ui->comboBox_SecondaryTileset);
    ui->comboBox_SecondaryTileset->clear();

    ui->comboBox_DiveMap->clear();
    ui->comboBox_EmergeMap->clear();

    const QSignalBlocker b_LayoutSelector(ui->comboBox_LayoutSelector);
    ui->comboBox_LayoutSelector->clear();

    this->mapHeaderForm->clear();
    ui->label_NoEvents->setText("");

    prefab.clearPrefabUi();

    // Clear map models
    delete this->mapGroupModel;
    delete this->groupListProxyModel;
    delete this->mapLocationModel;
    delete this->locationListProxyModel;
    delete this->layoutTreeModel;
    delete this->layoutListProxyModel;
    ui->mapListToolBar_Groups->clearFilter();
    ui->mapListToolBar_Locations->clearFilter();
    ui->mapListToolBar_Layouts->clearFilter();
    resetMapNavigation();
}

void MainWindow::scrollMapList(MapTree *list, const QString &itemName, bool expandItem) {
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
    if (expandItem) list->setExpanded(index, true);
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

// Scroll the current map list to the specified map name / layout ID.
// No scrolling occurs if...
// - The map list was in the middle of a search
// - A map/layout is being opened by interacting with the list (in which case `lockMapListAutoScroll` is true)
// - The item is not in the list (e.g. a layout ID for the Groups list)
void MainWindow::scrollCurrentMapListToItem(const QString &itemName, bool expandItem) {
    if (this->lockMapListAutoScroll)
        return;

    auto toolbar = getCurrentMapListToolBar();
    if (toolbar && toolbar->filterText().isEmpty()) {
        scrollMapList(toolbar->list(), itemName, expandItem);
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
    QAction* sortFoldersAction = nullptr;

    if (itemType == "map_name") {
        // Right-clicking on a map.
        openItemAction = menu.addAction("Open Map");
        connect(menu.addAction("Open JSON file"), &QAction::triggered, [this, itemName] {
            this->editor->openMapJson(itemName);
        });
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
        menu.addSeparator();
        sortFoldersAction = menu.addAction(list->isSortingEnabled() ? "Sort List by Value" : "Sort List Alphabetically");
    } else if (itemType == "map_layout") {
        // Right-clicking on a map layout
        openItemAction = menu.addAction("Open Layout");
        connect(menu.addAction("Open JSON file"), &QAction::triggered, [this, itemName] {
            this->editor->openLayoutJson(itemName);
        });
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
        menu.addSeparator();
        sortFoldersAction = menu.addAction(list->isSortingEnabled() ? "Sort List by Value" : "Sort List Alphabetically");
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
    if (sortFoldersAction) {
        connect(sortFoldersAction, &QAction::triggered, [this, list, itemName] {
            setMapListSorted(list, !list->isSortingEnabled());
            scrollCurrentMapListToItem(itemName, false);
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
    this->layoutTreeModel->insertMapItem(newMap->name(), newMap->layoutId());

    // Refresh any combo box that displays map names and persists between maps
    // (other combo boxes like for warp destinations are repopulated when the map changes).
    int mapIndex = this->editor->project->mapNames().indexOf(newMap->name());
    if (mapIndex >= 0) {
        ui->comboBox_DiveMap->insertItem(mapIndex, newMap->name());
        ui->comboBox_EmergeMap->insertItem(mapIndex, newMap->name());
    }

    userSetMap(newMap->name());
}

// Called any time a new layout is created (including as a byproduct of creating a new map)
void MainWindow::onNewLayoutCreated(Layout *layout) {
    logInfo(QString("Created a new layout named %1.").arg(layout->name));

    // Refresh layout combo box
    int layoutIndex = this->editor->project->layoutIds().indexOf(layout->id);
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
    if (!editor->metatile_selector_item->isInternalSelection() && editor->metatile_selector_item->getSelectionDimensions() != QSize(1, 1))
        return;

    MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
    if (selection.metatileItems.isEmpty())
        return;

    QPoint pos = editor->metatile_selector_item->getMetatileIdCoordsOnWidget(selection.metatileItems.first().metatileId);
    QSize size = editor->metatile_selector_item->getSelectionDimensions();
    pos += QPoint((size.width() - 1) * Metatile::pixelWidth(), (size.height() - 1) * Metatile::pixelHeight()) / 2; // We want to focus on the center of the whole selection
    pos *= getMetatilesZoomScale();

    auto viewport = ui->scrollArea_MetatileSelector->viewport();
    ui->scrollArea_MetatileSelector->ensureVisible(pos.x(), pos.y(), viewport->width() / 2, viewport->height() / 2);
}

void MainWindow::currentMetatilesSelectionChanged() {
    redrawMetatileSelection();
    if (this->tilesetEditor) {
        MetatileSelection selection = editor->metatile_selector_item->getMetatileSelection();
        if (!selection.metatileItems.isEmpty()) {
            this->tilesetEditor->selectMetatile(selection.metatileItems.first().metatileId);
        }
    }

    // Don't scroll to internal selections here, it will disrupt the user while they make their selection.
    if (!editor->metatile_selector_item->isInternalSelection())
        scrollMetatileSelectorToSelection();
}

void MainWindow::onMapListTabChanged(int index) {
    auto newToolbar = getMapListToolBar(index);
    auto oldToolbar = getMapListToolBar(porymapConfig.mapListTab);
    if (newToolbar && oldToolbar && newToolbar != oldToolbar) {
        newToolbar->applyFilter(oldToolbar->filterText());
    }

    // Save current tab for future sessions.
    porymapConfig.mapListTab = index;

    // After changing a map list tab the old tab's search widget can keep focus, which isn't helpful
    // (and might be a little confusing to the user, because they don't know that each search bar is secretly a separate object).
    // When we change tabs we'll automatically focus in on the search bar. This should also make finding maps a little quicker.
    if (newToolbar) newToolbar->setSearchFocus();
}

void MainWindow::openMapListItem(const QModelIndex &index) {
    if (!index.isValid())
        return;

    QVariant data = index.data(MapListUserRoles::NameRole);
    if (data.isNull())
        return;
    const QString name = data.toString();

    // Normally when a new map/layout is opened the current map list will scroll to display that item in the list.
    // We don't want to do this when the user interacts with a list directly, so we temporarily prevent this.
    this->lockMapListAutoScroll = true;

    QString type = index.data(MapListUserRoles::TypeRole).toString();
    if (type == "map_name") {
        userSetMap(name);
    } else if (type == "map_layout") {
        userSetLayout(name);
    }

    this->lockMapListAutoScroll = false;
}

void MainWindow::rebuildMapList_Locations() {
    this->mapLocationModel->deleteLater();
    this->mapLocationModel = new MapLocationModel(this->editor->project);
    this->locationListProxyModel->setSourceModel(this->mapLocationModel);
    ui->mapListToolBar_Locations->refreshFilter();
}
void MainWindow::rebuildMapList_Layouts() {
    this->layoutTreeModel->deleteLater();
    this->layoutTreeModel = new LayoutTreeModel(this->editor->project);
    this->layoutListProxyModel->setSourceModel(this->layoutTreeModel);
    ui->mapListToolBar_Layouts->refreshFilter();
}

void MainWindow::setMapListSorted(MapTree *list, bool sort) {
    if (sort == list->isSortingEnabled())
        return;
    list->setSortingEnabled(sort);
    list->sortByColumn(sort ? 0 : -1, Qt::SortOrder::AscendingOrder);

    if (list == ui->locationList) {
        porymapConfig.mapListLocationsSorted = sort;
    } else if (list == ui->layoutList) {
        porymapConfig.mapListLayoutsSorted = sort;
    }
}

QString MainWindow::getActiveItemName() {
    if (this->editor->map) return this->editor->map->name();
    if (this->editor->layout) return this->editor->layout->id;
    return QString();
}

void MainWindow::updateMapList() {
    QString activeItemName = getActiveItemName();

    // Clear relevant selections
    if (!this->editor->map) {
        ui->mapList->clearSelection();
        ui->locationList->clearSelection();
    }
    if (!this->editor->layout) {
        ui->layoutList->clearSelection();
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

bool MainWindow::save(bool currentOnly) {
    bool success = currentOnly ? this->editor->saveCurrent() : this->editor->saveAll();
    if (!success) {
        RecentErrorMessage::show(QStringLiteral("Failed to save some project changes."), this);
    }
    updateWindowTitle();
    updateMapList();

    if (success && !porymapConfig.shownInGameReloadMessage) {
        // Show a one-time warning that the user may need to reload their map to see their new changes.
        InfoMessage::show(QStringLiteral("Reload your map in-game!"),
                          QStringLiteral("If your game is currently saved on a map you have edited, "
                                         "the changes may not appear until you leave the map and return."),
                          this);
        porymapConfig.shownInGameReloadMessage = true;
    }

    saveGlobalConfigs();
    return success;
}

void MainWindow::duplicate() {
    editor->duplicateSelectedEvents();
}

void MainWindow::copy() {
    auto focused = QApplication::focusWidget();
    if (focused) {
        // Allow copying text from selectable QLabels.
        auto label = dynamic_cast<QLabel*>(focused);
        if (label && !label->selectedText().isEmpty()) {
            setClipboardData(label->selectedText());
            return;
        }

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
            copyObject["width"] = editor->metatile_selector_item->getSelectionDimensions().width();
            copyObject["height"] = editor->metatile_selector_item->getSelectionDimensions().height();
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
    } else if (index == MainTab::Events) {
        ui->stackedWidget_MapEvents->setCurrentIndex(1);
    } else if (index == MainTab::Connections) {
        ui->graphicsView_Connections->setFocus(); // Avoid opening tab with focus on something editable
    }

    if (!editor->map) return;
    if (index != MainTab::WildPokemon) {
        if (editor->project && editor->project->wildEncountersLoaded)
            editor->saveEncounterTabData();
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
    this->editor->updateCursorRectVisibility();
}

void MainWindow::on_actionCursor_Tile_Outline_triggered()
{
    bool enabled = ui->actionCursor_Tile_Outline->isChecked();
    porymapConfig.showCursorTile = enabled;
    this->editor->settings->cursorTileRectEnabled = enabled;
    this->editor->updateCursorRectVisibility();
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
        connect(this->gridSettingsDialog, &GridSettingsDialog::accepted, [this] { porymapConfig.gridSettings = this->editor->gridSettings; });
    }
    Util::show(this->gridSettingsDialog);
}

void MainWindow::on_actionShortcuts_triggered()
{
    if (!shortcutsEditor)
        initShortcutsEditor();

    Util::show(shortcutsEditor);
}

void MainWindow::initShortcutsEditor() {
    shortcutsEditor = new ShortcutsEditor(this);
    connect(shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
            this, &MainWindow::applyUserShortcuts);

    connectSubEditorsToShortcutsEditor();

    auto objectList = shortcutableObjects();
    for (auto *menu : findChildren<QMenu *>()) {
        if (!menu->objectName().isEmpty())
            objectList.append(qobject_cast<QObject *>(menu));
    }
    shortcutsEditor->setShortcutableObjects(objectList);
}

void MainWindow::connectSubEditorsToShortcutsEditor() {
    /* Initialize sub-editors so that their children are added to MainWindow's object tree and will
     * be returned by shortcutableObjects() to be passed to ShortcutsEditor. */
    if (!this->tilesetEditor) {
        initTilesetEditor();
    }
    if (this->tilesetEditor) {
        connect(this->shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
                this->tilesetEditor, &TilesetEditor::applyUserShortcuts);
    }

    if (!this->regionMapEditor){
        initRegionMapEditor(true);
    }
    if (this->regionMapEditor) {
        connect(this->shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
                this->regionMapEditor, &RegionMapEditor::applyUserShortcuts);
    }

    if (!this->customScriptsEditor) {
        initCustomScriptsEditor();
    }
    if (this->customScriptsEditor) {
        connect(this->shortcutsEditor, &ShortcutsEditor::shortcutsSaved,
                this->customScriptsEditor, &CustomScriptsEditor::applyUserShortcuts);
    }
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
    if (!this->editor->map)
        return;

    for (auto i = this->lastSelectedEvent.begin(); i != this->lastSelectedEvent.end(); i++) {
        if (i.value() && !this->editor->map->hasEvent(i.value()))
            this->lastSelectedEvent.insert(i.key(), nullptr);
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

        if (this->editor->map && this->editor->map->isInheritingEvents()) {
            QString message = QString("<span style=\"color:red;\">NOTE:</span> This map inherits events from %1."
                                      "<br>Adding any events will separate it from that map.").arg(this->editor->map->sharedEventsMap());
            ui->label_NoEvents->setText(message);
        } else {
            ui->label_NoEvents->setText(QStringLiteral("There are no events on the current map."));
        }
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

void MainWindow::on_actionPencil_triggered()     { on_toolButton_Paint_clicked(); }
void MainWindow::on_actionPointer_triggered()    { on_toolButton_Select_clicked(); }
void MainWindow::on_actionFlood_Fill_triggered() { on_toolButton_Fill_clicked(); }
void MainWindow::on_actionEyedropper_triggered() { on_toolButton_Dropper_clicked(); }
void MainWindow::on_actionMove_triggered()       { on_toolButton_Move_clicked(); }
void MainWindow::on_actionMap_Shift_triggered()  { on_toolButton_Shift_clicked(); }

void MainWindow::on_toolButton_Paint_clicked()   { editor->setEditAction(Editor::EditAction::Paint); }
void MainWindow::on_toolButton_Select_clicked()  { editor->setEditAction(Editor::EditAction::Select); }
void MainWindow::on_toolButton_Fill_clicked()    { editor->setEditAction(Editor::EditAction::Fill); }
void MainWindow::on_toolButton_Dropper_clicked() { editor->setEditAction(Editor::EditAction::Pick); }
void MainWindow::on_toolButton_Move_clicked()    { editor->setEditAction(Editor::EditAction::Move); }
void MainWindow::on_toolButton_Shift_clicked()   { editor->setEditAction(Editor::EditAction::Shift); }

void MainWindow::setEditActionUi(Editor::EditAction editAction) {
    ui->toolButton_Paint->setChecked(editAction == Editor::EditAction::Paint);
    ui->toolButton_Select->setChecked(editAction == Editor::EditAction::Select);
    ui->toolButton_Fill->setChecked(editAction == Editor::EditAction::Fill);
    ui->toolButton_Dropper->setChecked(editAction == Editor::EditAction::Pick);
    ui->toolButton_Move->setChecked(editAction == Editor::EditAction::Move);
    ui->toolButton_Shift->setChecked(editAction == Editor::EditAction::Shift);
}

void MainWindow::onOpenConnectedMap(MapConnection *connection) {
    if (!connection)
        return;
    if (userSetMap(connection->targetMapName()))
        editor->setSelectedConnection(connection->findMirror());
}

void MainWindow::onMapLoaded(Map *map) {
    connect(map, &Map::modified, [this, map] { markMapEdited(map); });
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
    if (label_MapRulerStatus && label_MapRulerStatus->isVisible() && label_MapRulerStatus->parentWidget()) {
        label_MapRulerStatus->move(label_MapRulerStatus->parentWidget()->mapToGlobal(QPoint(6, 6)));
    }
    if (this->resizeLayoutPopup) {
        this->resizeLayoutPopup->resetPosition();
    }
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

    // If the user is requesting this window again with a different mode
    // then we'll recreate the window with the new mode.
    if (this->mapImageExporter && this->mapImageExporter->mode() != mode)
        delete this->mapImageExporter;

    if (!this->mapImageExporter) {
        // Open new image export window
        if (this->editor->map){
            this->mapImageExporter = new MapImageExporter(this, this->editor->project, this->editor->map, mode);
        } else if (this->editor->layout) {
            this->mapImageExporter = new MapImageExporter(this, this->editor->project, this->editor->layout, mode);
        }
        if (this->mapImageExporter) {
            connect(this, &MainWindow::mapOpened, this->mapImageExporter, &MapImageExporter::setMap);
            connect(this, &MainWindow::layoutOpened, this->mapImageExporter, &MapImageExporter::setLayout);
        }
    }

    Util::show(this->mapImageExporter);
}

void MainWindow::on_pushButton_AddConnection_clicked() {
    if (!this->editor || !this->editor->map || !this->editor->project)
        return;

    auto dialog = new NewMapConnectionDialog(this, this->editor->map, this->editor->project->mapNames());
    connect(dialog, &NewMapConnectionDialog::newConnectionedAdded, this->editor, &Editor::addNewConnection);
    connect(dialog, &NewMapConnectionDialog::connectionReplaced, this->editor, &Editor::replaceConnection);
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
    Util::show(this->wildMonChart);
}

void MainWindow::on_toolButton_WildMonSearch_clicked() {
    if (!this->wildMonSearch) {
        this->wildMonSearch = new WildMonSearch(this->editor->project, this);
        connect(this->wildMonSearch, &WildMonSearch::openWildMonTableRequested, this, &MainWindow::openWildMonTable);
        connect(this->editor, &Editor::wildMonTableEdited, this->wildMonSearch, &WildMonSearch::refresh);
    }
    Util::show(this->wildMonSearch);
}

void MainWindow::openWildMonTable(const QString &mapName, const QString &groupName, const QString &fieldName) {
    if (userSetMap(mapName)) {
        // Switch to the correct main tab, wild encounter group, and wild encounter type tab.
        on_mainTabBar_tabBarClicked(MainTab::WildPokemon);
        ui->comboBox_EncounterGroupLabel->setTextItem(groupName);
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

void MainWindow::setPrimaryTileset(const QString &tilesetLabel) {
    if (!this->editor->layout || this->editor->layout->tileset_primary_label == tilesetLabel)
        return;

    if (editor->project->primaryTilesetLabels.contains(tilesetLabel)) {
        editor->updatePrimaryTileset(tilesetLabel);
        redrawMapScene();
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->layout);
        markLayoutEdited();
    }

    // Restore valid text if input was invalid, or sync combo box with new valid setting.
    const QSignalBlocker b(ui->comboBox_PrimaryTileset);
    ui->comboBox_PrimaryTileset->setTextItem(this->editor->layout->tileset_primary_label);
}

void MainWindow::setSecondaryTileset(const QString &tilesetLabel) {
    if (!this->editor->layout || this->editor->layout->tileset_secondary_label == tilesetLabel)
        return;

    if (editor->project->secondaryTilesetLabels.contains(tilesetLabel)) {
        editor->updateSecondaryTileset(tilesetLabel);
        redrawMapScene();
        updateTilesetEditor();
        prefab.updatePrefabUi(editor->layout);
        markLayoutEdited();
    }

    // Restore valid text if input was invalid, or sync combo box with new valid setting.
    const QSignalBlocker b(ui->comboBox_SecondaryTileset);
    ui->comboBox_SecondaryTileset->setTextItem(this->editor->layout->tileset_secondary_label);
}

void MainWindow::on_pushButton_ChangeDimensions_clicked() {
    if (this->resizeLayoutPopup || !this->editor->layout || !this->editor->project) return;

    this->resizeLayoutPopup = new ResizeLayoutPopup(this->ui->graphicsView_Map, this->editor->layout, this->editor->project);
    this->resizeLayoutPopup->show();
    this->resizeLayoutPopup->setupLayoutView();
    if (this->resizeLayoutPopup->exec() == QDialog::Accepted) {
        Layout *layout = this->editor->layout;
        QMargins result = this->resizeLayoutPopup->getResult();
        QSize borderResult = this->resizeLayoutPopup->getBorderResult();
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
        Map *map = this->editor->map;
        if (map) {
            auto events = map->getEvents();
            int deltaX = result.left();
            int deltaY = result.top();
            if ((deltaX || deltaY) && !events.isEmpty()) {
                map->commit(new EventShift(events, deltaX, deltaY, this->editor->eventShiftActionId++));
            }
        }
    }
    this->resizeLayoutPopup->deleteLater();
}

void MainWindow::setSmartPathsEnabled(bool enabled)
{
    this->editor->settings->smartPathsEnabled = enabled;
    this->editor->cursorMapTileRect->setSmartPathMode(enabled);
}

void MainWindow::setBorderVisibility(bool visible)
{
    editor->toggleBorderVisibility(visible);
}

void MainWindow::setMirrorConnectionsEnabled(bool enabled)
{
    porymapConfig.mirrorConnectingMaps = enabled;
}

void MainWindow::on_actionTileset_Editor_triggered()
{
    if (!this->tilesetEditor) {
        initTilesetEditor();
    }

    Util::show(this->tilesetEditor);

    MetatileSelection selection = this->editor->metatile_selector_item->getMetatileSelection();
    if (!selection.metatileItems.isEmpty()) {
        this->tilesetEditor->selectMetatile(selection.metatileItems.first().metatileId);
    }
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
    Util::show(this->aboutWindow);
}

void MainWindow::on_actionOpen_Log_File_triggered() {
    const QString logPath = getLogPath();
    const int lineCount = ParseUtil::textFileLineCount(logPath);
    this->editor->openInTextEditor(logPath, lineCount);
}

void MainWindow::on_actionOpen_Config_Folder_triggered() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)));
}

void MainWindow::on_actionOpen_Manual_triggered() {
    static const QUrl url("https://huderlem.github.io/porymap/");
    QDesktopServices::openUrl(url);
}

void MainWindow::on_actionPreferences_triggered() {
    if (!preferenceEditor) {
        preferenceEditor = new PreferenceEditor(this);
        connect(preferenceEditor, &PreferenceEditor::themeChanged, this, &MainWindow::setTheme);
        connect(preferenceEditor, &PreferenceEditor::themeChanged, editor, &Editor::maskNonVisibleConnectionTiles);
        connect(preferenceEditor, &PreferenceEditor::preferencesSaved, this, &MainWindow::togglePreferenceSpecificUi);
        connect(preferenceEditor, &PreferenceEditor::preferencesSaved, this, &MainWindow::initLogStatusBar);
        // Changes to porymapConfig.loadAllEventScripts or porymapConfig.eventSelectionShapeMode
        // require us to repopulate the EventFrames and redraw event pixmaps, respectively.
        connect(preferenceEditor, &PreferenceEditor::preferencesSaved, editor, &Editor::updateEvents);
        connect(preferenceEditor, &PreferenceEditor::scriptSettingsChanged, editor->project, &Project::readEventScriptLabels);
        connect(preferenceEditor, &PreferenceEditor::reloadProjectRequested, this, &MainWindow::on_action_Reload_Project_triggered);
    }

    Util::show(preferenceEditor);
}

void MainWindow::togglePreferenceSpecificUi() {
    ui->actionOpen_Project_in_Text_Editor->setEnabled(!porymapConfig.textEditorOpenFolder.isEmpty());

#ifdef USE_UPDATE_PROMOTER
    if (this->updatePromoter)
        this->updatePromoter->updatePreferences();
#endif
}

void MainWindow::openProjectSettingsEditor(int tab) {
    if (!this->projectSettingsEditor) {
        this->projectSettingsEditor = new ProjectSettingsEditor(this, this->editor->project);
        connect(this->projectSettingsEditor, &ProjectSettingsEditor::reloadProject,
                this, &MainWindow::on_action_Reload_Project_triggered);
    }
    this->projectSettingsEditor->setTab(tab);
    Util::show(this->projectSettingsEditor);
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

    auto msgBox = new InfoMessage(QStringLiteral("Warp Events only function as exits on certain metatiles"), this);
    auto settingsButton = msgBox->addButton("Open Settings...", QMessageBox::ActionRole);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->setTextFormat(Qt::RichText);
    msgBox->setInformativeText(informative);
    connect(settingsButton, &QAbstractButton::clicked, [this] {
        openProjectSettingsEditor(ProjectSettingsEditor::eventsTab);
    });
    msgBox->open();
}

void MainWindow::on_actionCustom_Scripts_triggered() {
    if (!this->customScriptsEditor) {
        initCustomScriptsEditor();
    }
    if (this->customScriptsEditor) {
        Util::show(this->customScriptsEditor);
    }
}

void MainWindow::initCustomScriptsEditor() {
#ifdef QT_QML_LIB
    this->customScriptsEditor = new CustomScriptsEditor(this);
    connect(this->customScriptsEditor, &CustomScriptsEditor::reloadScriptEngine,
            this, &MainWindow::reloadScriptEngine);
#endif
}

void MainWindow::reloadScriptEngine() {
    Scripting::init(this);
    Scripting::populateGlobalObject(this);
    // Lying to the scripts here, simulating a project reload
    Scripting::cb_ProjectOpened(projectConfig.projectDir());
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

    Util::show(this->regionMapEditor);
}

void MainWindow::on_pushButton_CreatePrefab_clicked() {
    auto dialog = new PrefabCreationDialog(this, this->editor->metatile_selector_item, this->editor->layout);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
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

bool MainWindow::closeSupplementaryWindows() {
    if (this->projectSettingsEditor) {
        this->projectSettingsEditor->closeQuietly();
    }

    // Attempt to close any visible windows (excluding the main window), giving each a chance to abort the process.
    for (const auto &widget : QApplication::topLevelWidgets()) {
        if (widget != this && widget->isWindow()) {
            // Make sure the window is raised and activated before closing in case it has a confirmation prompt.
            if (widget->isVisible()) {
                Util::show(widget);
            }
            if (!widget->close()) {
                QString message = QStringLiteral("Aborted project close");
                if (widget && !widget->objectName().isEmpty()) {
                    message.append(QString(": stopped by '%1'").arg(widget->objectName()));
                }
                logInfo(message);
                return false;
            }
        }
    }
    // We have some QPointers to windows that may have been closed above.
    // Make sure we force them to update to nullptr now; they may be read
    // before the next event loop gets a chance to update them.
    QApplication::sendPostedEvents();
    return true;
}

bool MainWindow::closeProject() {
    if (!isProjectOpen())
        return true;

    if (!closeSupplementaryWindows())
        return false;

    if (this->editor->project->hasUnsavedChanges()) {
        auto reply = SaveChangesMessage::show(QStringLiteral("The project"), this);
        if (reply == QMessageBox::Yes) {
            if (!save())
                return false;
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
