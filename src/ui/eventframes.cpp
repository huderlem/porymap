#include "eventframes.h"
#include "customattributestable.h"
#include "editcommands.h"
#include "draggablepixmapitem.h"

#include <limits>
using std::numeric_limits;



static inline QSpacerItem *createSpacerH() {
    return new QSpacerItem(2, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
}

static inline QSpacerItem *createSpacerV() {
    return new QSpacerItem(1, 2, QSizePolicy::Minimum, QSizePolicy::Expanding);
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
    CustomAttributesTable *customAttributes = new CustomAttributesTable(this->event, this);
    this->layout_contents->addWidget(customAttributes);
}

void EventFrame::connectSignals(MainWindow *) {
    this->connected = true;

    this->spinner_x->disconnect();
    connect(this->spinner_x, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        int delta = value - event->getX();
        if (delta) {
            this->event->getMap()->editHistory.push(new EventMove(QList<Event *>() << this->event, delta, 0, this->spinner_x->getActionId()));
        }
    });

    connect(this->event->getPixmapItem(), &DraggablePixmapItem::xChanged, this->spinner_x, &NoScrollSpinBox::setValue);
    
    this->spinner_y->disconnect();
    connect(this->spinner_y, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        int delta = value - event->getY();
        if (delta) {
            this->event->getMap()->editHistory.push(new EventMove(QList<Event *>() << this->event, 0, delta, this->spinner_y->getActionId()));
        }
    });
    connect(this->event->getPixmapItem(), &DraggablePixmapItem::yChanged, this->spinner_y, &NoScrollSpinBox::setValue);
    
    this->spinner_z->disconnect();
    connect(this->spinner_z, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->event->setZ(value);
        this->event->modify();
    });
}

void EventFrame::initialize() {
    this->initialized = true;

    const QSignalBlocker blocker(this);
    
    this->spinner_x->setValue(this->event->getX());
    this->spinner_y->setValue(this->event->getY());
    this->spinner_z->setValue(this->event->getZ());

    this->label_icon->setPixmap(this->event->getPixmap());
}

void EventFrame::populate(Project *) {
    this->populated = true;
}

void EventFrame::invalidateConnections() {
    this->connected = false;
}

void EventFrame::invalidateUi() {
    this->initialized = false;
}

void EventFrame::invalidateValues() {
    this->populated = false;
}

void EventFrame::setActive(bool active) {
    this->setEnabled(active);
    this->blockSignals(!active);
}



void ObjectFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Object");

    // sprite combo
    QFormLayout *l_form_sprite = new QFormLayout();
    this->combo_sprite = new NoScrollComboBox(this);
    this->combo_sprite->setToolTip("The sprite graphics to use for this object.");
    l_form_sprite->addRow("Sprite", this->combo_sprite);
    this->layout_contents->addLayout(l_form_sprite);

    // movement
    QFormLayout *l_form_movement = new QFormLayout();
    this->combo_movement = new NoScrollComboBox(this);
    this->combo_movement->setToolTip("The object's natural movement behavior when\n"
                                     "the player is not interacting with it.");
    l_form_movement->addRow("Movement", this->combo_movement);
    this->layout_contents->addLayout(l_form_movement);
    
    // movement radii
    QFormLayout *l_form_radii_xy = new QFormLayout();
    this->spinner_radius_x = new NoScrollSpinBox(this);
    this->spinner_radius_x->setMinimum(0);
    this->spinner_radius_x->setMaximum(255);
    this->spinner_radius_x->setToolTip("The maximum number of metatiles this object\n"
                                       "is allowed to move left or right during its\n"
                                       "normal movement behavior actions.");
    this->spinner_radius_y = new NoScrollSpinBox(this);
    this->spinner_radius_y->setMinimum(0);
    this->spinner_radius_y->setMaximum(255);
    this->spinner_radius_y->setToolTip("The maximum number of metatiles this object\n"
                                       "is allowed to move up or down during its\n"
                                       "normal movement behavior actions.");
    l_form_radii_xy->addRow("Movement Radius X", this->spinner_radius_x);
    l_form_radii_xy->addRow("Movement Radius Y", this->spinner_radius_y);
    this->layout_contents->addLayout(l_form_radii_xy);

    // script
    QFormLayout *l_form_script = new QFormLayout();
    this->combo_script = new NoScrollComboBox(this);
    this->combo_script->setToolTip("The script which is executed with this event.");

    // Add button next to combo which opens combo's current script.
    this->button_script = new QToolButton(this);
    this->button_script->setToolTip("Go to this script definition in text editor.");
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
    this->combo_flag->setToolTip("The flag which hides the object when set.");
    l_form_flag->addRow("Event Flag", this->combo_flag);
    this->layout_contents->addLayout(l_form_flag);

    // trainer type
    QFormLayout *l_form_trainer = new QFormLayout();
    this->combo_trainer_type = new NoScrollComboBox(this);
    this->combo_trainer_type->setToolTip("The trainer type of this object event.\n"
                                         "If it is not a trainer, use NONE. SEE ALL DIRECTIONS\n"
                                         "should only be used with a sight radius of 1.");
    l_form_trainer->addRow("Trainer Type", this->combo_trainer_type);
    this->layout_contents->addLayout(l_form_trainer);

    // sight radius / berry tree id
    QFormLayout *l_form_radius_treeid = new QFormLayout();
    this->combo_radius_treeid = new NoScrollComboBox(this);
    this->combo_radius_treeid->setToolTip("The maximum sight range of a trainer,\n"
                                            "OR the unique id of the berry tree.");
    l_form_radius_treeid->addRow("Sight Radius / Berry Tree ID", this->combo_radius_treeid);
    this->layout_contents->addLayout(l_form_radius_treeid);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void ObjectFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // sprite update
    this->combo_sprite->disconnect();
    connect(this->combo_sprite, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setGfx(text);
        this->object->getPixmapItem()->updatePixmap();
        this->object->modify();
    });
    connect(this->object->getPixmapItem(), &DraggablePixmapItem::spriteChanged, this->label_icon, &QLabel::setPixmap);

    // movement
    this->combo_movement->disconnect();
    connect(this->combo_movement, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->object->setMovement(text);
        this->object->getPixmapItem()->updatePixmap();
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

    // sprite
    this->combo_sprite->setTextItem(this->object->getGfx());

    // movement
    this->combo_movement->setTextItem(this->object->getMovement());

    // radius
    this->spinner_radius_x->setValue(this->object->getRadiusX());
    this->spinner_radius_y->setValue(this->object->getRadiusY());

    // script
    this->combo_script->setCurrentText(this->object->getScript());
    if (porymapConfig.getTextEditorGotoLine().isEmpty())
        this->button_script->hide();

    // flag
    this->combo_flag->setTextItem(this->object->getFlag());

    // trainer type
    this->combo_trainer_type->setTextItem(this->object->getTrainerType());

    // sight berry
    this->combo_radius_treeid->setCurrentText(this->object->getSightRadiusBerryTreeID());
}

void ObjectFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    this->combo_sprite->addItems(project->gfxDefines.keys());
    this->combo_movement->addItems(project->movementTypes);
    this->combo_flag->addItems(project->flagNames);
    this->combo_trainer_type->addItems(project->trainerTypes);

    // The script dropdown is populated with scripts used by the map's events and from its scripts file.
    QStringList scriptLabels;
    if (this->object->getMap()) {
        scriptLabels.append(this->object->getMap()->getScriptLabels());
        this->combo_script->addItems(scriptLabels);
    }

    // The dropdown's autocomplete has all script labels across the full project.
    scriptLabels.append(project->getGlobalScriptLabels());
    scriptLabels.removeDuplicates();
    this->scriptCompleter = new QCompleter(scriptLabels, this);
    this->scriptCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    this->scriptCompleter->setFilterMode(Qt::MatchContains);
    this->combo_script->setCompleter(this->scriptCompleter);
}



void CloneObjectFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Clone Object");

    this->spinner_z->setEnabled(false);

    // sprite combo (edits disabled)
    QFormLayout *l_form_sprite = new QFormLayout();
    this->combo_sprite = new NoScrollComboBox(this);
    l_form_sprite->addRow("Sprite", this->combo_sprite);
    this->combo_sprite->setEnabled(false);
    this->layout_contents->addLayout(l_form_sprite);

    // clone map id combo
    QFormLayout *l_form_dest_map = new QFormLayout();
    this->combo_target_map = new NoScrollComboBox(this);
    this->combo_target_map->setToolTip("The name of the map that the object being cloned is on.");
    l_form_dest_map->addRow("Target Map", this->combo_target_map);
    this->layout_contents->addLayout(l_form_dest_map);

    // clone local id spinbox
    QFormLayout *l_form_dest_id = new QFormLayout();
    this->spinner_target_id = new NoScrollSpinBox(this);
    this->spinner_target_id->setToolTip("event_object ID of the object being cloned.");
    l_form_dest_id->addRow("Target Local ID", this->spinner_target_id);
    this->layout_contents->addLayout(l_form_dest_id);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void CloneObjectFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // update icon displayed in frame with target
    connect(this->clone->getPixmapItem(), &DraggablePixmapItem::spriteChanged, this->label_icon, &QLabel::setPixmap);

    // target map
    this->combo_target_map->disconnect();
    connect(this->combo_target_map, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->clone->setTargetMap(text);
        this->clone->getPixmapItem()->updatePixmap();
        this->combo_sprite->setCurrentText(this->clone->getGfx());
        this->clone->modify();
    });

    // target id
    this->spinner_target_id->disconnect();
    connect(this->spinner_target_id, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
        this->clone->setTargetID(value);
        this->clone->getPixmapItem()->updatePixmap();
        this->combo_sprite->setCurrentText(this->clone->getGfx());
        this->clone->modify();
    });
}

void CloneObjectFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // sprite
    this->combo_sprite->setCurrentText(this->clone->getGfx());

    // target id
    this->spinner_target_id->setMinimum(1);
    this->spinner_target_id->setMaximum(126);
    this->spinner_target_id->setValue(this->clone->getTargetID());

    // target map
    this->combo_target_map->setTextItem(this->clone->getTargetMap());
}

void CloneObjectFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    this->combo_target_map->addItems(project->mapNames);
}

void WarpFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Warp");

    // desination map combo
    QFormLayout *l_form_dest_map = new QFormLayout();
    this->combo_dest_map = new NoScrollComboBox(this);
    this->combo_dest_map->setToolTip("The destination map name of the warp.");
    l_form_dest_map->addRow("Destination Map", this->combo_dest_map);
    this->layout_contents->addLayout(l_form_dest_map);

    // desination warp id
    QFormLayout *l_form_dest_warp = new QFormLayout();
    this->combo_dest_warp = new NoScrollComboBox(this);
    this->combo_dest_warp->setToolTip("The warp id on the destination map.");
    l_form_dest_warp->addRow("Destination Warp", this->combo_dest_warp);
    this->layout_contents->addLayout(l_form_dest_warp);

    // warning
    static const QString warningText = "Warning:\n"
                                       "This warp event is not positioned on a metatile with a warp behavior.\n"
                                       "Click this warning for more details.";
    QVBoxLayout *l_vbox_warning = new QVBoxLayout();
    this->warning = new QPushButton(warningText, this);
    this->warning->setFlat(true);
    this->warning->setStyleSheet("color: red; text-align: left");
    this->warning->setVisible(false);
    l_vbox_warning->addWidget(this->warning);
    this->layout_contents->addLayout(l_vbox_warning);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void WarpFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    // dest map
    this->combo_dest_map->disconnect();
    connect(this->combo_dest_map, &QComboBox::currentTextChanged, [this](const QString &text) {
        this->warp->setDestinationMap(text);
        this->warp->modify();
    });

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

void WarpFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    // dest map
    this->combo_dest_map->setTextItem(this->warp->getDestinationMap());

    // dest id
    this->combo_dest_warp->setCurrentText(this->warp->getDestinationWarpID());
}

void WarpFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    this->combo_dest_map->addItems(project->mapNames);
}



void TriggerFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Trigger");

    // script combo
    QFormLayout *l_form_script = new QFormLayout();
    this->combo_script = new NoScrollComboBox(this);
    this->combo_script->setToolTip("The script which is executed with this event.");
    l_form_script->addRow("Script", this->combo_script);
    this->layout_contents->addLayout(l_form_script);

    // var combo
    QFormLayout *l_form_var = new QFormLayout();
    this->combo_var = new NoScrollComboBox(this);
    this->combo_var->setToolTip("The variable by which the script is triggered.\n"
                                "The script is triggered when this variable's value matches 'Var Value'.");
    l_form_var->addRow("Var", this->combo_var);
    this->layout_contents->addLayout(l_form_var);

    // var value combo
    QFormLayout *l_form_var_val = new QFormLayout();
    this->combo_var_value = new NoScrollComboBox(this);
    this->combo_var_value->setToolTip("The variable's value which triggers the script.");
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
    this->combo_script->setCurrentText(this->trigger->getScriptLabel());

    // var
    this->combo_var->setTextItem(this->trigger->getScriptVar());

    // var value
    this->combo_var_value->setCurrentText(this->trigger->getScriptVarValue());
}

void TriggerFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    // var combo
    this->combo_var->addItems(project->varNames);

    // The script dropdown is populated with scripts used by the map's events and from its scripts file.
    QStringList scriptLabels;
    if (this->trigger->getMap()) {
        scriptLabels.append(this->trigger->getMap()->getScriptLabels());
        this->combo_script->addItems(scriptLabels);
    }

    // The dropdown's autocomplete has all script labels across the full project.
    scriptLabels.append(project->getGlobalScriptLabels());
    scriptLabels.removeDuplicates();
    this->scriptCompleter = new QCompleter(scriptLabels, this);
    this->scriptCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    this->scriptCompleter->setFilterMode(Qt::MatchContains);
    this->combo_script->setCompleter(this->scriptCompleter);
}



void WeatherTriggerFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Weather Trigger");

    // weather combo
    QFormLayout *l_form_weather = new QFormLayout();
    this->combo_weather = new NoScrollComboBox(this);
    this->combo_weather->setToolTip("The weather that starts when the player steps on this spot.");
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

    // weather
    this->combo_weather->addItems(project->coordEventWeatherNames);
}



void SignFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Sign");

    // facing dir combo
    QFormLayout *l_form_facing_dir = new QFormLayout();
    this->combo_facing_dir = new NoScrollComboBox(this);
    this->combo_facing_dir->setToolTip("The direction which the player must be facing\n"
                                       "to be able to interact with this event.");
    l_form_facing_dir->addRow("Player Facing Direction", this->combo_facing_dir);
    this->layout_contents->addLayout(l_form_facing_dir);

    // script combo
    QFormLayout *l_form_script = new QFormLayout();
    this->combo_script = new NoScrollComboBox(this);
    this->combo_script->setToolTip("The script which is executed with this event.");
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
    this->combo_script->setCurrentText(this->sign->getScriptLabel());
}

void SignFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    // facing dir
    this->combo_facing_dir->addItems(project->bgEventFacingDirections);

    // The script dropdown is populated with scripts used by the map's events and from its scripts file.
    QStringList scriptLabels;
    if (this->sign->getMap()) {
        scriptLabels.append(this->sign->getMap()->getScriptLabels());
        this->combo_script->addItems(scriptLabels);
    }

    // The dropdown's autocomplete has all script labels across the full project.
    scriptLabels.append(project->getGlobalScriptLabels());
    scriptLabels.removeDuplicates();
    this->scriptCompleter = new QCompleter(scriptLabels, this);
    this->scriptCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    this->scriptCompleter->setFilterMode(Qt::MatchContains);
    this->combo_script->setCompleter(this->scriptCompleter);
}



void HiddenItemFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Hidden Item");

    // item combo
    QFormLayout *l_form_item = new QFormLayout();
    this->combo_item = new NoScrollComboBox(this);
    this->combo_item->setToolTip("The item to be given.");
    l_form_item->addRow("Item", this->combo_item);
    this->layout_contents->addLayout(l_form_item);

    // flag combo
    QFormLayout *l_form_flag = new QFormLayout();
    this->combo_flag = new NoScrollComboBox(this);
    this->combo_flag->setToolTip("The flag which is set when the hidden item is picked up.");
    l_form_flag->addRow("Flag", this->combo_flag);
    this->layout_contents->addLayout(l_form_flag);

    // quantity spinner
    this->hideable_quantity = new QFrame;
    QFormLayout *l_form_quantity = new QFormLayout(hideable_quantity);
    l_form_quantity->setContentsMargins(0, 0, 0, 0);
    this->spinner_quantity = new NoScrollSpinBox(hideable_quantity);
    this->spinner_quantity->setToolTip("The number of items received when the hidden item is picked up.");
    this->spinner_quantity->setMinimum(0x01);
    this->spinner_quantity->setMaximum(0xFF);
    l_form_quantity->addRow("Quantity", this->spinner_quantity);
    this->layout_contents->addWidget(this->hideable_quantity);

    // itemfinder checkbox
    this->hideable_itemfinder = new QFrame;
    QFormLayout *l_form_itemfinder = new QFormLayout(hideable_itemfinder);
    l_form_itemfinder->setContentsMargins(0, 0, 0, 0);
    this->check_itemfinder = new QCheckBox(hideable_itemfinder);
    this->check_itemfinder->setToolTip("If checked, hidden item can only be picked up using the Itemfinder");
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
    connect(this->check_itemfinder, &QCheckBox::stateChanged, [=](int state) {
        this->hiddenItem->setUnderfoot(state == Qt::Checked);
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
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        this->spinner_quantity->setValue(this->hiddenItem->getQuantity());
    }
    this->hideable_quantity->setVisible(projectConfig.getHiddenItemQuantityEnabled());

    // underfoot
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
        this->check_itemfinder->setChecked(this->hiddenItem->getUnderfoot());
    }
    this->hideable_itemfinder->setVisible(projectConfig.getHiddenItemRequiresItemfinderEnabled());
}

void HiddenItemFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    this->combo_item->addItems(project->itemNames);
    this->combo_flag->addItems(project->flagNames);
}



void SecretBaseFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Secret Base");

    this->spinner_z->setEnabled(false);

    // item combo
    QFormLayout *l_form_base_id = new QFormLayout();
    this->combo_base_id = new NoScrollComboBox(this);
    this->combo_base_id->setToolTip("The secret base id which is inside this secret\n"
                                    "base entrance. Secret base ids are meant to be\n"
                                    "unique to each and every secret base entrance.");
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

    this->combo_base_id->addItems(project->secretBaseIds);
}



void HealLocationFrame::setup() {
    EventFrame::setup();

    this->label_id->setText("Heal Location");

    this->hideable_label_z->setVisible(false);
    this->spinner_z->setVisible(false);

    // respawn map combo
    this->hideable_respawn_map = new QFrame;
    QFormLayout *l_form_respawn_map = new QFormLayout(hideable_respawn_map);
    l_form_respawn_map->setContentsMargins(0, 0, 0, 0);
    this->combo_respawn_map = new NoScrollComboBox(hideable_respawn_map);
    this->combo_respawn_map->setToolTip("The map where the player will respawn after whiteout.");
    l_form_respawn_map->addRow("Respawn Map", this->combo_respawn_map);
    this->layout_contents->addWidget(hideable_respawn_map);

    // npc spinner
    this->hideable_respawn_npc = new QFrame;
    QFormLayout *l_form_respawn_npc = new QFormLayout(hideable_respawn_npc);
    l_form_respawn_npc->setContentsMargins(0, 0, 0, 0);
    this->spinner_respawn_npc = new NoScrollSpinBox(hideable_respawn_npc);
    this->spinner_respawn_npc->setToolTip("event_object ID of the NPC the player interacts with\n" 
                                          "upon respawning after whiteout.");
    l_form_respawn_npc->addRow("Respawn NPC", this->spinner_respawn_npc);
    this->layout_contents->addWidget(hideable_respawn_npc);

    // custom attributes
    EventFrame::initCustomAttributesTable();
}

void HealLocationFrame::connectSignals(MainWindow *window) {
    if (this->connected) return;

    EventFrame::connectSignals(window);

    if (projectConfig.getHealLocationRespawnDataEnabled()) {
        this->combo_respawn_map->disconnect();
        connect(this->combo_respawn_map, &QComboBox::currentTextChanged, [this](const QString &text) {
            this->healLocation->setRespawnMap(text);
            this->healLocation->modify();
        });

        this->spinner_respawn_npc->disconnect();
        connect(this->spinner_respawn_npc, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
            this->healLocation->setRespawnNPC(value);
            this->healLocation->modify();
        });
    }
}

void HealLocationFrame::initialize() {
    if (this->initialized) return;

    const QSignalBlocker blocker(this);
    EventFrame::initialize();

    bool respawnEnabled = projectConfig.getHealLocationRespawnDataEnabled();
    if (respawnEnabled) {
        this->combo_respawn_map->setTextItem(this->healLocation->getRespawnMap());
        this->spinner_respawn_npc->setValue(this->healLocation->getRespawnNPC());
    }

    this->hideable_respawn_map->setVisible(respawnEnabled);
    this->hideable_respawn_npc->setVisible(respawnEnabled);
}

void HealLocationFrame::populate(Project *project) {
    if (this->populated) return;

    const QSignalBlocker blocker(this);
    EventFrame::populate(project);

    if (projectConfig.getHealLocationRespawnDataEnabled())
        this->combo_respawn_map->addItems(project->mapNames);
}
