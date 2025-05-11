#include "eventframes.h"
#include "customattributesframe.h"
#include "editcommands.h"
#include "eventpixmapitem.h"

#include <limits>
using std::numeric_limits;



static inline QSpacerItem *createSpacerH() {
    return new QSpacerItem(2, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
}


void EventFrame::setup() {
    // delete default layout due to lack of parent at initialization
    if (this->layout()) delete this->layout();

    // create layout
    this->layout_main = new QVBoxLayout(this);

    // set frame style
    this->setFrameStyle(QFrame::Box | QFrame::Raised);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    ///
    /// init widgets
    ///
    QHBoxLayout *l_layout_xyz = new QHBoxLayout();

    // x spinner & label
    this->spinner_x = new NoScrollSpinBox(this);
    this->spinner_x->setMinimum(numeric_limits<int16_t>::min());
    this->spinner_x->setMaximum(numeric_limits<int16_t>::max());

    QLabel *l_label_x = new QLabel("x");

    QHBoxLayout *l_layout_x = new QHBoxLayout();
    l_layout_x->addWidget(l_label_x);
    l_layout_x->addWidget(this->spinner_x);

    l_layout_xyz->addLayout(l_layout_x);

    // y spinner & label
    this->spinner_y = new NoScrollSpinBox(this);
    this->spinner_y->setMinimum(numeric_limits<int16_t>::min());
    this->spinner_y->setMaximum(numeric_limits<int16_t>::max());

    QLabel *l_label_y = new QLabel("y");

    QHBoxLayout *l_layout_y = new QHBoxLayout();
    l_layout_y->addWidget(l_label_y);
    l_layout_y->addWidget(this->spinner_y);

    l_layout_xyz->addLayout(l_layout_y);

    // z spinner & label
    this->spinner_z = new NoScrollSpinBox(this);
    this->spinner_z->setMinimum(numeric_limits<int16_t>::min());
    this->spinner_z->setMaximum(numeric_limits<int16_t>::max());

    this->hideable_label_z = new QLabel("z");

    QHBoxLayout *l_layout_z = new QHBoxLayout();
    l_layout_z->addWidget(hideable_label_z);
    l_layout_z->addWidget(this->spinner_z);

    l_layout_xyz->addLayout(l_layout_z);
    l_layout_xyz->addItem(createSpacerH());

    QVBoxLayout *l_vbox_1 = new QVBoxLayout();

    this->label_id = new QLabel("event_type");
    l_vbox_1->addWidget(this->label_id);
    l_vbox_1->addLayout(l_layout_xyz);
    this->label_id->setText(Event::typeToString(this->event->getEventType()));

    // icon / pixmap label
    this->label_icon = new QLabel(this);
    this->label_icon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->label_icon->setMinimumSize(QSize(64, 64));
    this->label_icon->setScaledContents(false);
    this->label_icon->setAlignment(Qt::AlignCenter);
    this->label_icon->setFrameStyle(QFrame::Box | QFrame::Sunken);

    QHBoxLayout *l_hbox_1 = new QHBoxLayout();
    l_hbox_1->addWidget(this->label_icon);
    l_hbox_1->addLayout(l_vbox_1);

    this->layout_main->addLayout(l_hbox_1);

    // for derived classes to add their own ui elements
    this->layout_contents = new QVBoxLayout();
    this->layout_contents->setContentsMargins(0, 0, 0, 0);

    this->layout_main->addLayout(this->layout_contents);
}

void EventFrame::initCustomAttributesTable() {
    this->custom_attributes = new CustomAttributesFrame(this);
    this->custom_attributes->table()->setRestrictedKeys(this->event->getExpectedFields());
    this->layout_contents->addWidget(this->custom_attributes);
}

void EventFrame::connectSignals(MainWindow *) {
    this->connected = true;

    this->spinner_x->disconnect();
    connect(this->spinner_x, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        int delta = value - event->getX();
        if (delta) {
            this->event->getMap()->commit(new EventMove(QList<Event *>() << this->event, delta, 0, this->spinner_x->getActionId()));
        }
    });

    connect(this->event->getPixmapItem(), &EventPixmapItem::xChanged, this->spinner_x, &NoScrollSpinBox::setValue);
    
    this->spinner_y->disconnect();
    connect(this->spinner_y, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        int delta = value - event->getY();
        if (delta) {
            this->event->getMap()->commit(new EventMove(QList<Event *>() << this->event, 0, delta, this->spinner_y->getActionId()));
        }
    });
    connect(this->event->getPixmapItem(), &EventPixmapItem::yChanged, this->spinner_y, &NoScrollSpinBox::setValue);
    
    this->spinner_z->disconnect();
    connect(this->spinner_z, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->event->setZ(value);
        this->event->modify();
    });

    this->custom_attributes->disconnect();
    connect(this->custom_attributes->table(), &CustomAttributesTable::edited, [this]() {
        this->event->setCustomAttributes(this->custom_attributes->table()->getAttributes());
        this->event->modify();
    });
}

void EventFrame::initialize() {
    this->initialized = true;

    const QSignalBlocker blocker(this);
    
    this->spinner_x->setValue(this->event->getX());
    this->spinner_y->setValue(this->event->getY());
    this->spinner_z->setValue(this->event->getZ());

    this->custom_attributes->table()->setAttributes(this->event->getCustomAttributes());

    this->label_icon->setPixmap(this->event->getPixmap());
}

void EventFrame::populate(Project *project) {
    this->populated = true;
    if (this->project && this->project != project) {
        this->project->disconnect(this);
    }
    this->project = project;
}

void EventFrame::invalidateConnections() {
    this->connected = false;
}

void EventFrame::invalidateUi() {
    this->initialized = false;
}

void EventFrame::invalidateValues() {
    this->populated = false;
    if (this->isVisible()) {
        // Repopulate immediately
        this->populate(this->project);
    }
}

void EventFrame::setActive(bool active) {
    this->setEnabled(active);
    this->blockSignals(!active);
}

void EventFrame::populateDropdown(NoScrollComboBox * combo, const QStringList &items) {
    // Set the items in the combo box. This may be called after the frame is initialized
    // if the frame needs to be repopulated, so ensure the text in the combo is preserved
    // and that we don't accidentally fire 'currentTextChanged'.
    const QSignalBlocker b(combo);
    const QString savedText = combo->currentText();
    combo->clear();
    combo->addItems(items);
    combo->setTextItem(savedText);
}

void EventFrame::populateScriptDropdown(NoScrollComboBox * combo, Project * project) {
    // The script dropdown and autocomplete are populated with scripts used by the map's events and from its scripts file.
    Map *map = this->event ? this->event->getMap() : nullptr;
    if (!map)
        return;

    QStringList scripts = map->getScriptLabels(this->event->getEventGroup());
    populateDropdown(combo, scripts);

    // Depending on the settings, the autocomplete may also contain all global scripts.
    if (project && porymapConfig.loadAllEventScripts) {
        project->insertGlobalScriptLabels(scripts);
    }

    // Note: Because 'combo' is the parent, the old QCompleter will be deleted when a new one is set.
    auto completer = new QCompleter(scripts, combo);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setFilterMode(Qt::MatchContains);

    // Improve display speed for the autocomplete popup
    auto popup = (QListView *)completer->popup();
    if (popup) popup->setUniformItemSizes(true);

    combo->setCompleter(completer);

    // If the script labels change then we need to update the EventFrame.
    if (project) connect(project, &Project::eventScriptLabelsRead, this, &EventFrame::invalidateValues, Qt::UniqueConnection);
    connect(map, &Map::scriptsModified, this, &EventFrame::invalidateValues, Qt::UniqueConnection);
}

void EventFrame::populateMapNameDropdown(NoScrollComboBox * combo, Project * project) {
    if (!project)
        return;

    populateDropdown(combo, project->mapNames());

    // This frame type displays map names, so when a new map is created we need to repopulate it.
    connect(project, &Project::mapCreated, this, &EventFrame::invalidateValues, Qt::UniqueConnection);
}

void EventFrame::populateIdNameDropdown(NoScrollComboBox * combo, Project * project, const QString &mapName, Event::Group group) {
    if (!project || !project->isKnownMap(mapName))
        return;

    Map *map = project->loadMap(mapName);
    if (map) populateDropdown(combo, map->getEventIdNames(group));
}


void ObjectFrame::setup() {
    EventFrame::setup();

    // local id
    QFormLayout *l_form_local_id = new QFormLayout();
    this->line_edit_local_id = new QLineEdit(this);
    static const QString line_edit_local_id_toolTip = Util::toHtmlParagraph("An optional, unique name to use to refer to this object in scripts. "
                                                                            "If no name is given you can refer to this object using its 'object id' number.");
    this->line_edit_local_id->setToolTip(line_edit_local_id_toolTip);
    this->line_edit_local_id->setPlaceholderText("LOCALID_MY_NPC");
    l_form_local_id->addRow("Local ID", this->line_edit_local_id);
    this->layout_contents->addLayout(l_form_local_id);

    // sprite combo
    QFormLayout *l_form_sprite = new QFormLayout();
    this->combo_sprite = new NoScrollComboBox(this);
    static const QString combo_sprite_toolTip = Util::toHtmlParagraph("The sprite graphics to use for this object.");
    this->combo_sprite->setToolTip(combo_sprite_toolTip);
    l_form_sprite->addRow("Sprite", this->combo_sprite);
    this->layout_contents->addLayout(l_form_sprite);

    // movement
    QFormLayout *l_form_movement = new QFormLayout();
    this->combo_movement = new NoScrollComboBox(this);
    static const QString combo_movement_toolTip = Util::toHtmlParagraph("The object's natural movement behavior when the player is not interacting with it.");
    this->combo_movement->setToolTip(combo_movement_toolTip);
    l_form_movement->addRow("Movement", this->combo_movement);
    this->layout_contents->addLayout(l_form_movement);
    
    // movement radii
    QFormLayout *l_form_radii_xy = new QFormLayout();
    this->spinner_radius_x = new NoScrollSpinBox(this);
    this->spinner_radius_x->setMinimum(0);
    this->spinner_radius_x->setMaximum(255);
    static const QString spinner_radius_x_toolTip = Util::toHtmlParagraph("The maximum number of metatiles this object is allowed to move left "
                                                                          "or right during its normal movement behavior actions.");
    this->spinner_radius_x->setToolTip(spinner_radius_x_toolTip);
    this->spinner_radius_y = new NoScrollSpinBox(this);
    this->spinner_radius_y->setMinimum(0);
    this->spinner_radius_y->setMaximum(255);
    static const QString spinner_radius_y_toolTip = Util::toHtmlParagraph("The maximum number of metatiles this object is allowed to move up "
                                                                          "or down during its normal movement behavior actions.");
    this->spinner_radius_y->setToolTip(spinner_radius_y_toolTip);
    l_form_radii_xy->addRow("Movement Radius X", this->spinner_radius_x);
    l_form_radii_xy->addRow("Movement Radius Y", this->spinner_radius_y);
    this->layout_contents->addLayout(l_form_radii_xy);

    // script
    QFormLayout *l_form_script = new QFormLayout();
    this->combo_script = new NoScrollComboBox(this);
    static const QString combo_script_toolTip = Util::toHtmlParagraph("The script that is executed with this event.");
    this->combo_script->setToolTip(combo_script_toolTip);

    // Add button next to combo which opens combo's current script.
    this->button_script = new QToolButton(this);
    static const QString button_script_toolTip = Util::toHtmlParagraph("Go to this script definition in text editor.");
    this->button_script->setToolTip(button_script_toolTip);
    this->button_script->setFixedSize(this->combo_script->height(), this->combo_script->height());
    this->button_script->setIcon(QFileIconProvider().icon(QFileIconProvider::File));

    QHBoxLayout *l_hbox_scr = new QHBoxLayout();
    l_hbox_scr->setSpacing(3);
    l_hbox_scr->addWidget(this->combo_script);
    l_hbox_scr->addWidget(this->button_script);

    l_form_script->addRow("Script", l_hbox_scr);
    this->layout_contents->addLayout(l_form_script);

    // event flag
    QFormLayout *l_form_flag = new QFormLayout();
    this->combo_flag = new NoScrollComboBox(this);
    static const QString combo_flag_toolTip = Util::toHtmlParagraph("The flag that hides the object when set.");
    this->combo_flag->setToolTip(combo_flag_toolTip);
    l_form_flag->addRow("Event Flag", this->combo_flag);
    this->layout_contents->addLayout(l_form_flag);

    // trainer type
    QFormLayout *l_form_trainer = new QFormLayout();
    this->combo_trainer_type = new NoScrollComboBox(this);
    static const QString combo_trainer_type_toolTip = Util::toHtmlParagraph("The trainer type of this object event. If it is not a trainer, use NONE. "
                                                                            "SEE ALL DIRECTIONS should only be used with a sight radius of 1.");
    this->combo_trainer_type->setToolTip(combo_trainer_type_toolTip);
    l_form_trainer->addRow("Trainer Type", this->combo_trainer_type);
    this->layout_contents->addLayout(l_form_trainer);

    // sight radius / berry tree id
    QFormLayout *l_form_radius_treeid = new QFormLayout();
    this->combo_radius_treeid = new NoScrollComboBox(this);
    static const QString combo_radius_treeid_toolTip = Util::toHtmlParagraph("The maximum sight range of a trainer, OR the unique id of the berry tree.");
    this->combo_radius_treeid->setToolTip(combo_radius_treeid_toolTip);
    l_form_radius_treeid->addRow("Sight Radius / Berry Tree ID", this->combo_radius_treeid);
    this->layout_contents->addLayout(l_form_radius_treeid);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void ObjectFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // local id
    this->line_edit_local_id->disconnect();
    connect(this->line_edit_local_id, &QLineEdit::textChanged, [this](const QString &text) {
        this->object->setIdName(text);
        this->object->modify();
    });

    // sprite update
    this->combo_sprite->disconnect();
    connect(this->combo_sprite, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setGfx(text);
        this->object->getPixmapItem()->render(this->project);
        this->object->modify();
    });
    connect(this->object->getPixmapItem(), &EventPixmapItem::rendered, this->label_icon, &QLabel::setPixmap);

    // movement
    this->combo_movement->disconnect();
    connect(this->combo_movement, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setMovement(text);
        this->object->getPixmapItem()->render(this->project);
        this->object->modify();
    });

    // radii
    this->spinner_radius_x->disconnect();
    connect(this->spinner_radius_x, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->object->setRadiusX(value);
        this->object->modify();
    });

    this->spinner_radius_y->disconnect();
    connect(this->spinner_radius_y, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->object->setRadiusY(value);
        this->object->modify();
    });

    // script
    this->combo_script->disconnect();
    connect(this->combo_script, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setScript(text);
        this->object->modify();
    });

    this->button_script->disconnect();
    connect(this->button_script, &QToolButton::clicked, [this]() {
        this->object->getMap()->openScript(this->combo_script->currentText());
        this->object->modify();
    });

    // flag
    this->combo_flag->disconnect();
    connect(this->combo_flag, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setFlag(text);
        this->object->modify();
    });

    // trainer type
    this->combo_trainer_type->disconnect();
    connect(this->combo_trainer_type, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setTrainerType(text);
        this->object->modify();
    });

    // sight berry
    this->combo_radius_treeid->disconnect();
    connect(this->combo_radius_treeid, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setSightRadiusBerryTreeID(text);
        this->object->modify();
    });
}

void ObjectFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // local id
    this->line_edit_local_id->setText(this->object->getIdName());

    // sprite
    this->combo_sprite->setTextItem(this->object->getGfx());

    // movement
    this->combo_movement->setTextItem(this->object->getMovement());

    // radius
    this->spinner_radius_x->setValue(this->object->getRadiusX());
    this->spinner_radius_y->setValue(this->object->getRadiusY());

    // script
    this->combo_script->setTextItem(this->object->getScript());
    if (porymapConfig.textEditorGotoLine.isEmpty())
        this->button_script->hide();

    // flag
    this->combo_flag->setTextItem(this->object->getFlag());

    // trainer type
    this->combo_trainer_type->setTextItem(this->object->getTrainerType());

    // sight berry
    this->combo_radius_treeid->setTextItem(this->object->getSightRadiusBerryTreeID());
}

void ObjectFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateDropdown(this->combo_sprite, project->gfxDefines.keys());
    populateDropdown(this->combo_movement, project->movementTypes);
    populateDropdown(this->combo_flag, project->flagNames);
    populateDropdown(this->combo_trainer_type, project->trainerTypes);
    populateScriptDropdown(this->combo_script, project);
}



void CloneObjectFrame::setup() {
    EventFrame::setup();

    this->spinner_z->setEnabled(false);

    // local id
    QFormLayout *l_form_local_id = new QFormLayout();
    this->line_edit_local_id = new QLineEdit(this);
    static const QString line_edit_local_id_toolTip = Util::toHtmlParagraph("An optional, unique name to use to refer to this object in scripts. "
                                                                            "If no name is given you can refer to this object using its 'object id' number.");
    this->line_edit_local_id->setToolTip(line_edit_local_id_toolTip);
    this->line_edit_local_id->setPlaceholderText("LOCALID_MY_CLONE_NPC");
    l_form_local_id->addRow("Local ID", this->line_edit_local_id);
    this->layout_contents->addLayout(l_form_local_id);

    // sprite combo (edits disabled)
    QFormLayout *l_form_sprite = new QFormLayout();
    this->combo_sprite = new NoScrollComboBox(this);
    static const QString combo_sprite_toolTip = Util::toHtmlParagraph("The sprite graphics to use for this object. This is updated automatically "
                                                                      "to match the target object, and so can't be edited. By default the games "
                                                                      "will get the graphics directly from the target object, so this field is ignored.");
    this->combo_sprite->setToolTip(combo_sprite_toolTip);
    l_form_sprite->addRow("Sprite", this->combo_sprite);
    this->combo_sprite->setEnabled(false);
    this->layout_contents->addLayout(l_form_sprite);

    // clone map id combo
    QFormLayout *l_form_dest_map = new QFormLayout();
    this->combo_target_map = new NoScrollComboBox(this);
    static const QString combo_target_map_toolTip = Util::toHtmlParagraph("The name of the map that the object being cloned is on.");
    this->combo_target_map->setToolTip(combo_target_map_toolTip);
    l_form_dest_map->addRow("Target Map", this->combo_target_map);
    this->layout_contents->addLayout(l_form_dest_map);

    // clone local id combo
    QFormLayout *l_form_dest_id = new QFormLayout();
    this->combo_target_id = new NoScrollComboBox(this);
    static const QString combo_target_id_toolTip = Util::toHtmlParagraph("The Local ID name or number of the object being cloned.");
    this->combo_target_id->setToolTip(combo_target_id_toolTip);
    l_form_dest_id->addRow("Target Local ID", this->combo_target_id);
    this->layout_contents->addLayout(l_form_dest_id);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void CloneObjectFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // local id
    this->line_edit_local_id->disconnect();
    connect(this->line_edit_local_id, &QLineEdit::textChanged, [this](const QString &text) {
        this->clone->setIdName(text);
        this->clone->modify();
    });

    // update icon displayed in frame with target
    connect(this->clone->getPixmapItem(), &EventPixmapItem::rendered, this->label_icon, &QLabel::setPixmap);

    // target map
    this->combo_target_map->disconnect();
    connect(this->combo_target_map, &QComboBox::currentTextChanged, [this](const QString &mapName) {
        this->clone->setTargetMap(mapName);
        this->clone->getPixmapItem()->render(this->project);
        this->combo_sprite->setTextItem(this->clone->getGfx());
        this->clone->modify();
        populateIdNameDropdown(this->combo_target_id, this->project, mapName, Event::Group::Object);
    });
    connect(window, &MainWindow::mapOpened, this, &CloneObjectFrame::tryInvalidateIdDropdown, Qt::UniqueConnection);

    // target id
    this->combo_target_id->disconnect();
    connect(this->combo_target_id, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->clone->setTargetID(text);
        this->clone->getPixmapItem()->render(this->project);
        this->combo_sprite->setTextItem(this->clone->getGfx());
        this->clone->modify();
    });
}

void CloneObjectFrame::tryInvalidateIdDropdown(Map *map) {
    // If the clone's target map is opened then the names in this frame's ID dropdown may be changed.
    // Make sure we update the frame next time it's opened.
    if (map && this->clone && map->name() == this->clone->getTargetMap()) {
        invalidateValues();
    }
}

void CloneObjectFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // local id
    this->line_edit_local_id->setText(this->clone->getIdName());

    // sprite
    this->combo_sprite->setTextItem(this->clone->getGfx());

    // target id
    this->combo_target_id->setTextItem(this->clone->getTargetID());

    // target map
    this->combo_target_map->setTextItem(this->clone->getTargetMap());
}

void CloneObjectFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateMapNameDropdown(this->combo_target_map, project);
    populateIdNameDropdown(this->combo_target_id, project, this->clone->getTargetMap(), Event::Group::Object);
}

void WarpFrame::setup() {
    EventFrame::setup();

    // ID
    QFormLayout *l_form_id = new QFormLayout();
    this->line_edit_id = new QLineEdit(this);
    static const QString line_edit_id_toolTip = Util::toHtmlParagraph("An optional, unique name to use to refer to this warp from other warps. "
                                                                      "If no name is given you can refer to this warp using its 'warp id' number.");
    this->line_edit_id->setToolTip(line_edit_id_toolTip);
    this->line_edit_id->setPlaceholderText("WARP_ID_MY_WARP");
    l_form_id->addRow("ID", this->line_edit_id);
    this->layout_contents->addLayout(l_form_id);

    // desination map combo
    QFormLayout *l_form_dest_map = new QFormLayout();
    this->combo_dest_map = new NoScrollComboBox(this);
    static const QString combo_dest_map_toolTip = Util::toHtmlParagraph("The destination map name of the warp.");
    this->combo_dest_map->setToolTip(combo_dest_map_toolTip);
    l_form_dest_map->addRow("Destination Map", this->combo_dest_map);
    this->layout_contents->addLayout(l_form_dest_map);

    // desination warp id
    QFormLayout *l_form_dest_warp = new QFormLayout();
    this->combo_dest_warp = new NoScrollComboBox(this);
    static const QString combo_dest_warp_toolTip = Util::toHtmlParagraph("The warp id on the destination map.");
    this->combo_dest_warp->setToolTip(combo_dest_warp_toolTip);
    l_form_dest_warp->addRow("Destination Warp", this->combo_dest_warp);
    this->layout_contents->addLayout(l_form_dest_warp);

    // warning
    auto warningText = QStringLiteral("Warning:\nThis warp event is not positioned on a metatile with a warp behavior.\nClick this warning for more details.");
    QVBoxLayout *l_vbox_warning = new QVBoxLayout();
    this->warning = new QPushButton(warningText, this);
    this->warning->setFlat(true);
    this->warning->setStyleSheet("color: red; text-align: left");
    this->warning->setVisible(this->warp->getWarningEnabled());
    l_vbox_warning->addWidget(this->warning);
    this->layout_contents->addLayout(l_vbox_warning);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void WarpFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // id
    this->line_edit_id->disconnect();
    connect(this->line_edit_id, &QLineEdit::textChanged, [this](const QString &text) {
        this->warp->setIdName(text);
        this->warp->modify();
    });

    // dest map
    this->combo_dest_map->disconnect();
    connect(this->combo_dest_map, &QComboBox::currentTextChanged, [this](const QString &mapName) {
        this->warp->setDestinationMap(mapName);
        this->warp->modify();
        populateIdNameDropdown(this->combo_dest_warp, this->project, mapName, Event::Group::Warp);
    });
    connect(window, &MainWindow::mapOpened, this, &WarpFrame::tryInvalidateIdDropdown, Qt::UniqueConnection);

    // dest id
    this->combo_dest_warp->disconnect();
    connect(this->combo_dest_warp, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->warp->setDestinationWarpID(text);
        this->warp->modify();
    });

    // warning
    this->warning->disconnect();
    connect(this->warning, &QPushButton::clicked, window, &MainWindow::onWarpBehaviorWarningClicked);
}

void WarpFrame::tryInvalidateIdDropdown(Map *map) {
    // If the warps's target map is opened then the names in this frame's ID dropdown may be changed.
    // Make sure we update the frame next time it's opened.
    if (map && this->warp && map->name() == this->warp->getDestinationMap()) {
        invalidateValues();
    }
}

void WarpFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // id
    this->line_edit_id->setText(this->warp->getIdName());

    // dest map
    this->combo_dest_map->setTextItem(this->warp->getDestinationMap());

    // dest id
    this->combo_dest_warp->setTextItem(this->warp->getDestinationWarpID());
}

void WarpFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateMapNameDropdown(this->combo_dest_map, project);
    populateIdNameDropdown(this->combo_dest_warp, project, this->warp->getDestinationMap(), Event::Group::Warp);
}



void TriggerFrame::setup() {
    EventFrame::setup();

    // script combo
    QFormLayout *l_form_script = new QFormLayout();
    this->combo_script = new NoScrollComboBox(this);
    static const QString combo_script_toolTip = Util::toHtmlParagraph("The script that is executed with this event.");
    this->combo_script->setToolTip(combo_script_toolTip);
    l_form_script->addRow("Script", this->combo_script);
    this->layout_contents->addLayout(l_form_script);

    // var combo
    QFormLayout *l_form_var = new QFormLayout();
    this->combo_var = new NoScrollComboBox(this);
    static const QString combo_var_toolTip = Util::toHtmlParagraph("The variable by which the script is triggered. "
                                                                    "The script is triggered when this variable's value matches 'Var Value'.");
    this->combo_var->setToolTip(combo_var_toolTip);
    l_form_var->addRow("Var", this->combo_var);
    this->layout_contents->addLayout(l_form_var);

    // var value combo
    QFormLayout *l_form_var_val = new QFormLayout();
    this->combo_var_value = new NoScrollComboBox(this);
    static const QString combo_var_value_toolTip = Util::toHtmlParagraph("The variable's value that triggers the script.");
    this->combo_var_value->setToolTip(combo_var_value_toolTip);
    l_form_var_val->addRow("Var Value", this->combo_var_value);
    this->layout_contents->addLayout(l_form_var_val);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void TriggerFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // label
    this->combo_script->disconnect();
    connect(this->combo_script, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->trigger->setScriptLabel(text);
        this->trigger->modify();
    });

    // var
    this->combo_var->disconnect();
    connect(this->combo_var, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->trigger->setScriptVar(text);
        this->trigger->modify();
    });

    // value
    this->combo_var_value->disconnect();
    connect(this->combo_var_value, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->trigger->setScriptVarValue(text);
        this->trigger->modify();
    });
}

void TriggerFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // script
    this->combo_script->setTextItem(this->trigger->getScriptLabel());

    // var
    this->combo_var->setTextItem(this->trigger->getScriptVar());

    // var value
    this->combo_var_value->setTextItem(this->trigger->getScriptVarValue());
}

void TriggerFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateDropdown(this->combo_var, project->varNames);
    populateScriptDropdown(this->combo_script, project);
}



void WeatherTriggerFrame::setup() {
    EventFrame::setup();

    // weather combo
    QFormLayout *l_form_weather = new QFormLayout();
    this->combo_weather = new NoScrollComboBox(this);
    static const QString combo_weather_toolTip = Util::toHtmlParagraph("The weather that starts when the player steps on this spot.");
    this->combo_weather->setToolTip(combo_weather_toolTip);
    l_form_weather->addRow("Weather", this->combo_weather);
    this->layout_contents->addLayout(l_form_weather);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void WeatherTriggerFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // weather
    this->combo_weather->disconnect();
    connect(this->combo_weather, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->weatherTrigger->setWeather(text);
        this->weatherTrigger->modify();
    });
}

void WeatherTriggerFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // weather
    this->combo_weather->setTextItem(this->weatherTrigger->getWeather());
}

void WeatherTriggerFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateDropdown(this->combo_weather, project->coordEventWeatherNames);
}



void SignFrame::setup() {
    EventFrame::setup();

    // facing dir combo
    QFormLayout *l_form_facing_dir = new QFormLayout();
    this->combo_facing_dir = new NoScrollComboBox(this);
    static const QString combo_facing_dir_toolTip = Util::toHtmlParagraph("The direction that the player must be facing to be able to interact with this event.");
    this->combo_facing_dir->setToolTip(combo_facing_dir_toolTip);
    l_form_facing_dir->addRow("Player Facing Direction", this->combo_facing_dir);
    this->layout_contents->addLayout(l_form_facing_dir);

    // script combo
    QFormLayout *l_form_script = new QFormLayout();
    this->combo_script = new NoScrollComboBox(this);
    static const QString combo_script_toolTip = Util::toHtmlParagraph("The script that is executed with this event.");
    this->combo_script->setToolTip(combo_script_toolTip);
    l_form_script->addRow("Script", this->combo_script);
    this->layout_contents->addLayout(l_form_script);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void SignFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // facing dir
    this->combo_facing_dir->disconnect();
    connect(this->combo_facing_dir, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->sign->setFacingDirection(text);
        this->sign->modify();
    });

    // script
    this->combo_script->disconnect();
    connect(this->combo_script, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->sign->setScriptLabel(text);
        this->sign->modify();
    });
}

void SignFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // facing dir
    this->combo_facing_dir->setTextItem(this->sign->getFacingDirection());

    // script
    this->combo_script->setTextItem(this->sign->getScriptLabel());
}

void SignFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateDropdown(this->combo_facing_dir, project->bgEventFacingDirections);
    populateScriptDropdown(this->combo_script, project);
}



void HiddenItemFrame::setup() {
    EventFrame::setup();

    // item combo
    QFormLayout *l_form_item = new QFormLayout();
    this->combo_item = new NoScrollComboBox(this);
    static const QString combo_item_toolTip = Util::toHtmlParagraph("The item to be given.");
    this->combo_item->setToolTip(combo_item_toolTip);
    l_form_item->addRow("Item", this->combo_item);
    this->layout_contents->addLayout(l_form_item);

    // flag combo
    QFormLayout *l_form_flag = new QFormLayout();
    this->combo_flag = new NoScrollComboBox(this);
    static const QString combo_flag_toolTip = Util::toHtmlParagraph("The flag that is set when the hidden item is picked up.");
    this->combo_flag->setToolTip(combo_flag_toolTip);
    l_form_flag->addRow("Flag", this->combo_flag);
    this->layout_contents->addLayout(l_form_flag);

    // quantity spinner
    this->hideable_quantity = new QFrame;
    QFormLayout *l_form_quantity = new QFormLayout(hideable_quantity);
    l_form_quantity->setContentsMargins(0, 0, 0, 0);
    this->spinner_quantity = new NoScrollSpinBox(hideable_quantity);
    static const QString spinner_quantity_toolTip = Util::toHtmlParagraph("The number of items received when the hidden item is picked up.");
    this->spinner_quantity->setToolTip(spinner_quantity_toolTip);
    this->spinner_quantity->setMinimum(0x01);
    this->spinner_quantity->setMaximum(0xFF);
    l_form_quantity->addRow("Quantity", this->spinner_quantity);
    this->layout_contents->addWidget(this->hideable_quantity);

    // itemfinder checkbox
    this->hideable_itemfinder = new QFrame;
    QFormLayout *l_form_itemfinder = new QFormLayout(hideable_itemfinder);
    l_form_itemfinder->setContentsMargins(0, 0, 0, 0);
    this->check_itemfinder = new QCheckBox(hideable_itemfinder);
    static const QString check_itemfinder_toolTip = Util::toHtmlParagraph("If checked, hidden item can only be picked up using the Itemfinder");
    this->check_itemfinder->setToolTip(check_itemfinder_toolTip);
    l_form_itemfinder->addRow("Requires Itemfinder", this->check_itemfinder);
    this->layout_contents->addWidget(hideable_itemfinder);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void HiddenItemFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // item
    this->combo_item->disconnect();
    connect(this->combo_item, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->hiddenItem->setItem(text);
        this->hiddenItem->modify();
    });

    // flag
    this->combo_flag->disconnect();
    connect(this->combo_flag, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->hiddenItem->setFlag(text);
        this->hiddenItem->modify();
    });

    // quantity
    this->spinner_quantity->disconnect();
    connect(this->spinner_quantity, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->hiddenItem->setQuantity(value);
        this->hiddenItem->modify();
    });

    // underfoot
    this->check_itemfinder->disconnect();
    connect(this->check_itemfinder, &QCheckBox::toggled, [=](bool checked) {
        this->hiddenItem->setUnderfoot(checked);
        this->hiddenItem->modify();
    });
}

void HiddenItemFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // item
    this->combo_item->setTextItem(this->hiddenItem->getItem());

    // flag
    this->combo_flag->setTextItem(this->hiddenItem->getFlag());

    // quantity
    if (projectConfig.hiddenItemQuantityEnabled) {
        this->spinner_quantity->setValue(this->hiddenItem->getQuantity());
    }
    this->hideable_quantity->setVisible(projectConfig.hiddenItemQuantityEnabled);

    // underfoot
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        this->check_itemfinder->setChecked(this->hiddenItem->getUnderfoot());
    }
    this->hideable_itemfinder->setVisible(projectConfig.hiddenItemRequiresItemfinderEnabled);
}

void HiddenItemFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateDropdown(this->combo_item, project->itemNames);
    populateDropdown(this->combo_flag, project->flagNames);
}



void SecretBaseFrame::setup() {
    EventFrame::setup();

    this->spinner_z->setEnabled(false);

    // item combo
    QFormLayout *l_form_base_id = new QFormLayout();
    this->combo_base_id = new NoScrollComboBox(this);
    static const QString combo_base_id_toolTip = Util::toHtmlParagraph("The secret base id that is inside this secret base entrance. "
                                                                       "Secret base ids are meant to be unique to each and every secret base entrance.");
    this->combo_base_id->setToolTip(combo_base_id_toolTip);
    l_form_base_id->addRow("Secret Base", this->combo_base_id);
    this->layout_contents->addLayout(l_form_base_id);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void SecretBaseFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    this->combo_base_id->disconnect();
    connect(this->combo_base_id, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->secretBase->setBaseID(text);
        this->secretBase->modify();
    });
}

void SecretBaseFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // id
    this->combo_base_id->setTextItem(this->secretBase->getBaseID());
}

void SecretBaseFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    populateDropdown(this->combo_base_id, project->secretBaseIds);
}



void HealLocationFrame::setup() {
    EventFrame::setup();

    this->hideable_label_z->setVisible(false);
    this->spinner_z->setVisible(false);

    // ID
    QFormLayout *l_form_id = new QFormLayout();
    this->line_edit_id = new QLineEdit(this);
    static const QString line_edit_id_toolTip = Util::toHtmlParagraph("The unique identifier for this heal location.");
    this->line_edit_id->setToolTip(line_edit_id_toolTip);
    this->line_edit_id->setPlaceholderText(projectConfig.getIdentifier(ProjectIdentifier::define_heal_locations_prefix) + "MY_MAP");
    l_form_id->addRow("ID", this->line_edit_id);
    this->layout_contents->addLayout(l_form_id);

    // respawn map combo
    this->hideable_respawn_map = new QFrame;
    QFormLayout *l_form_respawn_map = new QFormLayout(hideable_respawn_map);
    l_form_respawn_map->setContentsMargins(0, 0, 0, 0);
    this->combo_respawn_map = new NoScrollComboBox(hideable_respawn_map);
    static const QString combo_respawn_map_toolTip = Util::toHtmlParagraph("The map where the player will respawn after whiteout.");
    this->combo_respawn_map->setToolTip(combo_respawn_map_toolTip);
    l_form_respawn_map->addRow("Respawn Map", this->combo_respawn_map);
    this->layout_contents->addWidget(hideable_respawn_map);

    // npc spinner
    this->hideable_respawn_npc = new QFrame;
    QFormLayout *l_form_respawn_npc = new QFormLayout(hideable_respawn_npc);
    l_form_respawn_npc->setContentsMargins(0, 0, 0, 0);
    this->combo_respawn_npc = new NoScrollComboBox(hideable_respawn_npc);
    static const QString combo_respawn_npc_toolTip = Util::toHtmlParagraph("The Local ID name or number of the NPC the player "
                                                                           "interacts with upon respawning after whiteout.");
    this->combo_respawn_npc->setToolTip(combo_respawn_npc_toolTip);
    l_form_respawn_npc->addRow("Respawn NPC", this->combo_respawn_npc);
    this->layout_contents->addWidget(hideable_respawn_npc);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void HealLocationFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    this->line_edit_id->disconnect();
    connect(this->line_edit_id, &QLineEdit::textChanged, [this](const QString &text) {
        this->healLocation->setIdName(text);
        this->healLocation->modify();
    });

    this->combo_respawn_map->disconnect();
    connect(this->combo_respawn_map, &QComboBox::currentTextChanged, [this](const QString &mapName) {
        this->healLocation->setRespawnMapName(mapName);
        this->healLocation->modify();
        populateIdNameDropdown(this->combo_respawn_npc, this->project, mapName, Event::Group::Object);
    });
    connect(window, &MainWindow::mapOpened, this, &HealLocationFrame::tryInvalidateIdDropdown, Qt::UniqueConnection);

    this->combo_respawn_npc->disconnect();
    connect(this->combo_respawn_npc, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->healLocation->setRespawnNPC(text);
        this->healLocation->modify();
    });
}

void HealLocationFrame::tryInvalidateIdDropdown(Map *map) {
    // If the heal locations's target map is opened then the names in this frame's ID dropdown may be changed.
    // Make sure we update the frame next time it's opened.
    if (map && this->healLocation && map->name() == this->healLocation->getRespawnMapName()) {
        invalidateValues();
    }
}

void HealLocationFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    this->line_edit_id->setText(this->healLocation->getIdName());
    this->combo_respawn_map->setTextItem(this->healLocation->getRespawnMapName());
    this->combo_respawn_npc->setTextItem(this->healLocation->getRespawnNPC());

    bool respawnEnabled = projectConfig.healLocationRespawnDataEnabled;
    this->hideable_respawn_map->setVisible(respawnEnabled);
    this->hideable_respawn_npc->setVisible(respawnEnabled);
}

void HealLocationFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    if (projectConfig.healLocationRespawnDataEnabled) {
        populateMapNameDropdown(this->combo_respawn_map, project);
        populateIdNameDropdown(this->combo_respawn_npc, project, this->healLocation->getRespawnMapName(), Event::Group::Object);
    }
}
