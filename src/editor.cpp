#include "editor.h"
#include "eventpixmapitem.h"
#include "imageproviders.h"
#include "log.h"
#include "connectionslistitem.h"
#include "currentselectedmetatilespixmapitem.h"
#include "eventfilters.h"
#include "metatile.h"
#include "montabwidget.h"
#include "editcommands.h"
#include "config.h"
#include "scripting.h"
#include "customattributesframe.h"
#include "validator.h"
#include "message.h"
#include "eventframes.h"
#include <QCheckBox>
#include <QPainter>
#include <QMouseEvent>
#include <QDir>
#include <QProcess>
#include <math.h>

static bool selectNewEvents = false;

// 2D array mapping collision+elevation combos to an icon.
QList<QList<const QImage*>> Editor::collisionIcons;

Editor::Editor(Ui::MainWindow* ui)
{
    this->ui = ui;
    this->settings = new Settings();
    this->cursorMapTileRect = new CursorTileRect(&this->settings->cursorTileRectEnabled, qRgb(255, 255, 255));
    this->map_ruler = new MapRuler(4);
    connect(this->map_ruler, &MapRuler::statusChanged, this, &Editor::mapRulerStatusChanged);

    /// Instead of updating the selected events after every single undo action
    /// (eg when the user rolls back several at once), only reselect events when
    /// the index is changed.
    connect(&editGroup, &QUndoGroup::indexChanged, [this](int) {
        if (selectNewEvents) {
            updateEvents();
            selectNewEvents = false;
        }
    });

    // Send signals used for updating the wild pokemon summary chart
    connect(ui->stackedWidget_WildMons, &QStackedWidget::currentChanged, [this] {
        emit wildMonTableOpened(getCurrentWildMonTable());
    });

    connect(ui->toolButton_Open_Scripts, &QToolButton::pressed, this, &Editor::openMapScripts);
    connect(ui->actionOpen_Project_in_Text_Editor, &QAction::triggered, this, &Editor::openProjectInTextEditor);
    connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, this, &Editor::toggleGrid);
    connect(ui->mapCustomAttributesFrame->table(), &CustomAttributesTable::edited, this, &Editor::updateCustomMapAttributes);

    connect(ui->comboBox_DiveMap, &NoScrollComboBox::editingFinished, [this] {
        onDivingMapEditingFinished(this->ui->comboBox_DiveMap, "dive");
    });
    connect(ui->comboBox_EmergeMap, &NoScrollComboBox::editingFinished, [this] {
        onDivingMapEditingFinished(this->ui->comboBox_EmergeMap, "emerge");
    });
    connect(ui->comboBox_DiveMap, &NoScrollComboBox::currentTextChanged, [this] {
        updateDivingMapButton(this->ui->button_OpenDiveMap, this->ui->comboBox_DiveMap->currentText());
    });
    connect(ui->comboBox_EmergeMap, &NoScrollComboBox::currentTextChanged, [this] {
        updateDivingMapButton(this->ui->button_OpenEmergeMap, this->ui->comboBox_EmergeMap->currentText());
    });
}

Editor::~Editor()
{
    delete this->settings;
    delete this->playerViewRect;
    delete this->cursorMapTileRect;
    delete this->map_ruler;
    for (auto sublist : collisionIcons)
        qDeleteAll(sublist);

    closeProject();
}

bool Editor::saveCurrent() {
    return save(true);
}

bool Editor::saveAll() {
    return save(false);
}

bool Editor::save(bool currentOnly) {
    if (!this->project)
        return true;

    saveEncounterTabData();

    bool success = true;
    if (currentOnly) {
        if (this->map) {
            success = this->project->saveMap(this->map);
        } else if (this->layout) {
            success = this->project->saveLayout(this->layout);
        }
        if (!this->project->saveGlobalData())
            success = false;
    } else {
        success = this->project->saveAll();
    }
    return success;
}

void Editor::setProject(Project * project) {
    closeProject();
    this->project = project;
    MapConnection::project = project;
}

void Editor::closeProject() {
    if (!this->project)
        return;
    this->project->saveConfig();
    Scripting::cb_ProjectClosed(this->project->root);
    Scripting::stop();
    clearMap();
    delete this->project;
}

bool Editor::getEditingLayout() {
    return this->editMode == EditMode::Metatiles || this->editMode == EditMode::Collision;
}

void Editor::setEditMode(EditMode editMode) {
    // At the moment we can't early return if editMode == this->editMode, because this function also takes care of refreshing the map view.
    // The main window relies on this when switching projects (the edit mode will remain the same, but it needs a refresh).
    auto oldEditMode = this->editMode;
    this->editMode = editMode;

    if (!map_item || !collision_item) return;
    if (!this->layout) return;

    map_item->setVisible(true); // is map item ever not visible
    collision_item->setVisible(false);

    switch (this->editMode) {
    case EditMode::Metatiles:
    case EditMode::Connections:
    case EditMode::Events:
        current_view = map_item;
        break;
    case EditMode::Collision:
        current_view = collision_item;
        break;
    default:
        current_view = nullptr;
        break;
    }

    map_item->setEditsEnabled(this->editMode != EditMode::Connections);
    map_item->draw();
    collision_item->draw();

    if (current_view) current_view->setVisible(true);

    updateBorderVisibility();

    QUndoStack *editStack = this->map ? this->map->editHistory() : nullptr;
    bool editingLayout = getEditingLayout();
    if (editingLayout && this->layout) {
        editStack = &this->layout->editHistory;
    }

    this->cursorMapTileRect->setActive(editingLayout);
    this->playerViewRect->setActive(editingLayout);
    this->editGroup.setActiveStack(editStack);
    this->ui->toolButton_Fill->setEnabled(editingLayout);
    this->ui->toolButton_Dropper->setEnabled(editingLayout);
    this->ui->pushButton_ChangeDimensions->setEnabled(editingLayout);
    this->ui->checkBox_smartPaths->setEnabled(editingLayout);

    if (this->editMode == EditMode::Events || oldEditMode == EditMode::Events) {
        // When switching to or from the Events tab the opacity of the events changes. Redraw the events to reflect that change.
       redrawAllEvents();
    }
    if (this->editMode == EditMode::Events){
        updateWarpEventWarnings();
    }

    if (current_view) {
        // Updating the edit action is only relevant for edit modes with a graphics view.
        setEditAction(getEditAction());
    }
}

Editor::EditAction Editor::getEditAction() const {
    return this->editMode == EditMode::Events ? getEventEditAction() : getMapEditAction();
}

void Editor::setEditAction(EditAction editAction) {
    if (this->editMode == EditMode::Events) {
        this->eventEditAction = editAction;
        this->map_ruler->setEnabled(editAction == EditAction::Select);
    } else {
        this->mapEditAction = editAction;
        this->map_ruler->setEnabled(false);
    }

    this->cursorMapTileRect->setSingleTileMode(!(editAction == EditAction::Paint && this->editMode == EditMode::Metatiles));

    // Update cursor
    static const QMap<EditAction, QCursor> cursors = {
        {EditAction::Paint,  QCursor(QPixmap(":/icons/pencil_cursor.ico"), 10, 10)},
        {EditAction::Select, QCursor()},
        {EditAction::Fill,   QCursor(QPixmap(":/icons/fill_color_cursor.ico"), 10, 10)},
        {EditAction::Pick,   QCursor(QPixmap(":/icons/pipette_cursor.ico"), 10, 10)},
        {EditAction::Move,   QCursor(QPixmap(":/icons/move.ico"), 7, 7)},
        {EditAction::Shift,  QCursor(QPixmap(":/icons/shift_cursor.ico"), 10, 10)},
    };
    this->settings->mapCursor = cursors.value(editAction);
    emit editActionSet(editAction);
}

void Editor::clearWildMonTables() {
    QStackedWidget *stack = ui->stackedWidget_WildMons;
    const QSignalBlocker blocker(stack);

    // delete widgets from previous map data if they exist
    while (stack->count()) {
        QWidget *oldWidget = stack->widget(0);
        stack->removeWidget(oldWidget);
        delete oldWidget;
    }

    ui->comboBox_EncounterGroupLabel->clear();
    emit wildMonTableClosed();
}

int Editor::getSortedItemIndex(QComboBox *combo, QString item) {
    int i = 0;
    for (; i < combo->count(); i++) {
        if (item < combo->itemText(i))
            break;
    }
    return i;
}

void Editor::displayWildMonTables() {
    clearWildMonTables();

    // Don't try to read encounter data if it doesn't exist on disk for this map.
    if (!project->wildMonData.contains(map->constantName())) {
        return;
    }

    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;
    QStringList labelComboStrings;
    for (auto groupPair : project->wildMonData[map->constantName()])
        labelComboStrings.append(groupPair.first);

    labelComboStrings.sort();
    labelCombo->addItems(labelComboStrings);
    labelCombo->setCurrentIndex(0);

    QStackedWidget *stack = ui->stackedWidget_WildMons;
    int labelIndex = 0;
    for (QString label : labelComboStrings) {
        WildPokemonHeader header = project->wildMonData[map->constantName()][label];

        MonTabWidget *tabWidget = new MonTabWidget(this);
        stack->insertWidget(labelIndex++, tabWidget);

        int tabIndex = 0;
        for (EncounterField monField : project->wildMonFields) {
            QString fieldName = monField.name;

            tabWidget->clearTableAt(tabIndex);

            if (project->wildMonData.contains(map->constantName()) && header.wildMons[fieldName].active) {
                tabWidget->populateTab(tabIndex, header.wildMons[fieldName]);
            } else {
                tabWidget->setTabActive(tabIndex, false);
            }
            tabIndex++;
        }
        connect(tabWidget, &MonTabWidget::currentChanged, [this] {
            emit wildMonTableOpened(getCurrentWildMonTable());
        });
    }
    stack->setCurrentIndex(0);
    emit wildMonTableOpened(getCurrentWildMonTable());
}

void Editor::addNewWildMonGroup(QWidget *window) {
    QStackedWidget *stack = ui->stackedWidget_WildMons;
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;

    int stackIndex = stack->currentIndex();

    QDialog dialog(window, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("New Wild Encounter Group Label");
    dialog.setWindowModality(Qt::NonModal);

    QFormLayout form(&dialog);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);

    QLineEdit *lineEdit = new QLineEdit();
    lineEdit->setClearButtonEnabled(true);
    form.addRow(new QLabel("Group Base Label:"), lineEdit);
    lineEdit->setValidator(new IdentifierValidator(lineEdit));
    connect(lineEdit, &QLineEdit::textChanged, [this, &lineEdit, &buttonBox](QString text){
        bool invalid = !this->project->isIdentifierUnique(text);
        Util::setErrorStylesheet(lineEdit, invalid);
        buttonBox.button(QDialogButtonBox::Ok)->setDisabled(invalid);
    });
    // Give a default value to the label.
    lineEdit->setText(this->project->toUniqueIdentifier("g" + map->name()));

    // Fields [x] copy from existing
    QLabel *fieldsLabel = new QLabel("Fields:");
    form.addRow(fieldsLabel);
    QCheckBox *copyCheckbox = new QCheckBox;
    copyCheckbox->setEnabled(stack->count());
    form.addRow(new QLabel("Copy from current group"), copyCheckbox);
    QVector<QCheckBox *> fieldCheckboxes;
    for (EncounterField monField : project->wildMonFields) {
        QCheckBox *fieldCheckbox = new QCheckBox;
        fieldCheckboxes.append(fieldCheckbox);
        form.addRow(new QLabel(monField.name), fieldCheckbox);
    }
    // Reading from ui here so not saving to disk before user.
    connect(copyCheckbox, &QCheckBox::toggled, [=](bool checked){
        if (checked) {
            int fieldIndex = 0;
            MonTabWidget *monWidget = static_cast<MonTabWidget *>(stack->widget(stack->currentIndex()));
            for (EncounterField monField : project->wildMonFields) {
                fieldCheckboxes[fieldIndex]->setChecked(monWidget->isTabEnabled(fieldIndex));
                fieldCheckboxes[fieldIndex]->setEnabled(false);
                fieldIndex++;
            }
        } else {
            int fieldIndex = 0;
            for (EncounterField monField : project->wildMonFields) {
                fieldCheckboxes[fieldIndex]->setEnabled(true);
                fieldIndex++;
            }
        }
    });

    connect(&buttonBox, &QDialogButtonBox::accepted, [&dialog, &lineEdit, this](){
        QString newLabel = lineEdit->text();
        if (!newLabel.isEmpty()) {
            this->project->encounterGroupLabels.append(newLabel);
            dialog.accept();
        }
    });
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        WildPokemonHeader header;
        for (EncounterField& monField : project->wildMonFields) {
            QString fieldName = monField.name;
            header.wildMons[fieldName].active = false;
            header.wildMons[fieldName].encounterRate = 0;
        }

        QString tempItemLabel = lineEdit->text();
        int newItemIndex = getSortedItemIndex(labelCombo, tempItemLabel);
        
        labelCombo->insertItem(newItemIndex, tempItemLabel);

        MonTabWidget *tabWidget = new MonTabWidget(this);

        int tabIndex = 0;
        for (EncounterField &monField : project->wildMonFields) {
            QString fieldName = monField.name;
            tabWidget->clearTableAt(tabIndex);
            if (fieldCheckboxes[tabIndex]->isChecked()) {
                if (copyCheckbox->isChecked()) {
                    MonTabWidget *copyFrom = static_cast<MonTabWidget *>(stack->widget(stackIndex));
                    if (copyFrom->isTabEnabled(tabIndex)) {
                        QTableView *monTable = copyFrom->tableAt(tabIndex);
                        EncounterTableModel *model = static_cast<EncounterTableModel *>(monTable->model());
                        header.wildMons[fieldName] = model->encounterData();
                    }
                    else {
                        header.wildMons[fieldName] = getDefaultMonInfo(monField);
                    }
                } else {
                    header.wildMons[fieldName] = getDefaultMonInfo(monField);
                }
                tabWidget->populateTab(tabIndex, header.wildMons[fieldName]);
            } else {
                tabWidget->setTabActive(tabIndex, false);
            }
            tabIndex++;
        }

        stack->insertWidget(newItemIndex, tabWidget);
        labelCombo->setCurrentIndex(newItemIndex);

        saveEncounterTabData();
        emit wildMonTableEdited();
    }
}

void Editor::deleteWildMonGroup() {
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;

    if (labelCombo->count() < 1) {
        return;
    }

    QMessageBox msgBox;
    msgBox.setText("Confirm Delete");
    msgBox.setInformativeText("Are you sure you want to delete " + labelCombo->currentText() + "?");

    QPushButton *deleteButton = msgBox.addButton("Delete", QMessageBox::DestructiveRole);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.exec();

    if (msgBox.clickedButton() == deleteButton) {
        auto it = project->wildMonData.find(map->constantName());
        if (it == project->wildMonData.end()) {
          logError(QString("Failed to find data for map %1. Unable to delete").arg(map->constantName()));
          return;
        }

        int i = project->encounterGroupLabels.indexOf(labelCombo->currentText());
        if (i < 0) {
          logError(QString("Failed to find selected wild mon group: %1. Unable to delete")
                   .arg(labelCombo->currentText()));
          return;
        }

        it.value().erase(labelCombo->currentText());
        project->encounterGroupLabels.remove(i);

        displayWildMonTables();
        saveEncounterTabData();
        emit wildMonTableEdited();
    }
}

void Editor::configureEncounterJSON(QWidget *window) {
    QVector<QWidget *> fieldSlots;

    EncounterFields tempFields = project->wildMonFields;

    QLabel *totalLabel = new QLabel;

    // lambda: Update the total displayed at the bottom of the Configure JSON
    //         window. Take groups into account when applicable.
    auto updateTotal = [&fieldSlots, totalLabel](EncounterField &currentField) {
        int total = 0, spinnerIndex = 0;
        QString groupTotalMessage;
        QMap<QString, int> groupTotals;
        for (auto keyPair : currentField.groups) {
            groupTotals.insert(keyPair.first, 0);// add to group map and initialize total to zero
        }
        for (auto slot : fieldSlots) {
            QSpinBox *spinner = slot->findChild<QSpinBox *>();
            int val = spinner->value();
            currentField.encounterRates[spinnerIndex] = val;
            if (!currentField.groups.empty()) {
                for (auto keyPair : currentField.groups) {
                    QString key = keyPair.first;
                    if (currentField.groups[key].contains(spinnerIndex)) {
                        groupTotals[key] += val;
                        break;
                    }
                }
            } else {
                total += val;
            }
            spinnerIndex++;
        }
        if (!currentField.groups.empty()) {
            groupTotalMessage += "Totals: ";
            for (auto keyPair : currentField.groups) {
                QString key = keyPair.first;
                groupTotalMessage += QString("%1 (%2),\t").arg(groupTotals[key]).arg(key);
            }
            groupTotalMessage.chop(2);
        } else {
            groupTotalMessage = QString("Total: %1").arg(QString::number(total));
        }
        if (total > 0xFF) {
            totalLabel->setTextFormat(Qt::RichText);
            groupTotalMessage += QString("<font color=\"red\">\tWARNING: value exceeds the limit for a u8 variable.</font>");
        }
        totalLabel->setText(groupTotalMessage);
    };

    // lambda: Create a new "slot", which is the widget containing a spinner and an index label. 
    //         Add the slot to a list of fieldSlots, which exists to keep track of them for memory management.
    auto createNewSlot = [&fieldSlots, &tempFields, &updateTotal](int index, EncounterField &currentField) {
        QLabel *indexLabel = new QLabel(QString("Index: %1").arg(QString::number(index)));
        QSpinBox *chanceSpinner = new QSpinBox;
        int chance = currentField.encounterRates.at(index);
        chanceSpinner->setMinimum(1);
        chanceSpinner->setMaximum(9999);
        chanceSpinner->setValue(chance);
        connect(chanceSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [&updateTotal, &currentField](int) {
            updateTotal(currentField);
        });

        bool useGroups = !currentField.groups.empty();

        QFrame *slotChoiceFrame = new QFrame;
        QVBoxLayout *slotChoiceLayout = new QVBoxLayout;
        if (useGroups) {
            auto groupCombo = new NoScrollComboBox;
            groupCombo->setEditable(false);
            groupCombo->setMinimumContentsLength(10);
            connect(groupCombo, QOverload<const QString &>::of(&QComboBox::textActivated), [&tempFields, &currentField, &updateTotal, index](QString newGroupName) {
                for (EncounterField &field : tempFields) {
                    if (field.name == currentField.name) {
                        for (auto groupNameIterator : field.groups) {
                            QString groupName = groupNameIterator.first;
                            if (field.groups[groupName].contains(index)) {
                                field.groups[groupName].removeAll(index);
                                break;
                            }
                        }
                        for (auto groupNameIterator : field.groups) {
                            QString groupName = groupNameIterator.first;
                            if (groupName == newGroupName) field.groups[newGroupName].append(index);
                        }
                        break;
                    }
                }
                updateTotal(currentField);
            });
            for (auto groupNameIterator : currentField.groups) {
                groupCombo->addItem(groupNameIterator.first);
            }
            QString currentGroupName;
            for (auto groupNameIterator : currentField.groups) {
                QString groupName = groupNameIterator.first;
                if (currentField.groups[groupName].contains(index)) {
                    currentGroupName = groupName;
                    break;
                }
            }
            groupCombo->setTextItem(currentGroupName);
            slotChoiceLayout->addWidget(groupCombo);
        }
        slotChoiceLayout->addWidget(chanceSpinner);
        slotChoiceFrame->setLayout(slotChoiceLayout);

        QFrame *slot = new QFrame;
        QHBoxLayout *slotLayout = new QHBoxLayout;
        slotLayout->addWidget(indexLabel);
        slotLayout->addWidget(slotChoiceFrame);
        slot->setLayout(slotLayout);

        fieldSlots.append(slot);

        return slot;
    };

    QDialog dialog(window, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle("Configure Wild Encounter Fields");
    dialog.setWindowModality(Qt::NonModal);

    enum GridRow {
        NoteLabel,
        Header,
        TableStart
    };
    QGridLayout grid;

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // lambda: Get a QStringList of the existing field names.
    auto getFieldNames = [&tempFields]() {
        QStringList fieldNames;
        for (EncounterField field : tempFields)
            fieldNames.append(field.name);
        return fieldNames;
    };

    // lambda: Draws the slot widgets onto a grid (4 wide) on the dialog window.
    auto drawSlotWidgets = [&dialog, &grid, &createNewSlot, &fieldSlots, &updateTotal, &tempFields](int index) {
        // Clear them first.
        while (!fieldSlots.isEmpty()) {
            auto slot = fieldSlots.takeFirst();
            grid.removeWidget(slot);
            delete slot;
        }

        if (!tempFields.size()) {
            return;
        }
        if (index >= tempFields.size()) {
            index = tempFields.size() - 1;
        }
        EncounterField &currentField = tempFields[index];
        const int numSlotColumns = 4;
        for (int i = 0; i < currentField.encounterRates.size(); i++) {
            grid.addWidget(createNewSlot(i, currentField), (i / numSlotColumns) + GridRow::TableStart, i % numSlotColumns);
        }

        updateTotal(currentField);

        dialog.adjustSize();// TODO: why is this updating only on second call? reproduce: land->fishing->rock_smash->water
    };
    QComboBox *fieldChoices = new QComboBox;
    connect(fieldChoices, QOverload<int>::of(&QComboBox::currentIndexChanged), drawSlotWidgets);
    fieldChoices->addItems(getFieldNames());

    QLabel *fieldChoiceLabel = new QLabel("Field");

    // Button to create new fields in the JSON.
    QPushButton *addFieldButton = new QPushButton("Add New Field...");
    connect(addFieldButton, &QPushButton::clicked, [fieldChoices, &tempFields]() {
        QDialog newNameDialog(nullptr, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        newNameDialog.setWindowModality(Qt::NonModal);
        QDialogButtonBox newFieldButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &newNameDialog);
        connect(&newFieldButtonBox, &QDialogButtonBox::accepted, &newNameDialog, &QDialog::accept);
        connect(&newFieldButtonBox, &QDialogButtonBox::rejected, &newNameDialog, &QDialog::reject);

        QLineEdit *newNameEdit = new QLineEdit;
        newNameEdit->setClearButtonEnabled(true);

        QFormLayout newFieldForm(&newNameDialog);

        newFieldForm.addRow("Field Name", newNameEdit);
        newFieldForm.addRow(&newFieldButtonBox);

        if (newNameDialog.exec() == QDialog::Accepted) {
            QString newFieldName = newNameEdit->text();
            QVector<int> newFieldRates(1, 100);
            tempFields.append({newFieldName, newFieldRates, {}, {}});
            fieldChoices->addItem(newFieldName);
            fieldChoices->setCurrentIndex(fieldChoices->count() - 1);
        }
    });
    QPushButton *deleteFieldButton = new QPushButton("Delete Field");
    connect(deleteFieldButton, &QPushButton::clicked, [drawSlotWidgets, fieldChoices, &tempFields]() {
        if (tempFields.size() < 2) return;// don't delete last
        int index = fieldChoices->currentIndex();
        fieldChoices->removeItem(index);
        tempFields.remove(index);
        drawSlotWidgets(index);
    });

    QPushButton *addSlotButton = new QPushButton(QIcon(":/icons/add.ico"), "");
    addSlotButton->setFlat(true);
    connect(addSlotButton, &QPushButton::clicked, [&fieldChoices, &drawSlotWidgets, &tempFields]() {
        EncounterField &field = tempFields[fieldChoices->currentIndex()];
        field.encounterRates.append(1);
        drawSlotWidgets(fieldChoices->currentIndex());
    });
    QPushButton *removeSlotButton = new QPushButton(QIcon(":/icons/delete.ico"), "");
    removeSlotButton->setFlat(true);
    connect(removeSlotButton, &QPushButton::clicked, [&fieldChoices, &drawSlotWidgets, &tempFields]() {
        EncounterField &field = tempFields[fieldChoices->currentIndex()];
        int lastIndex = field.encounterRates.size() - 1;
        if (lastIndex > 0)
            field.encounterRates.removeLast();
        for (auto &g : field.groups) {
            field.groups[g.first].removeAll(lastIndex);
        }
        drawSlotWidgets(fieldChoices->currentIndex());
    });
    // TODO: method for editing groups?

    auto noteLabel = new QLabel(QStringLiteral("Note: This affects all maps, not just the current map."));
    grid.addWidget(noteLabel, GridRow::NoteLabel, 0, 1, -1, Qt::AlignCenter);

    QFrame firstRow;
    QHBoxLayout firstRowLayout;
    firstRowLayout.addWidget(fieldChoiceLabel);
    firstRowLayout.addWidget(fieldChoices);
    firstRowLayout.addWidget(deleteFieldButton);
    firstRowLayout.addWidget(addFieldButton);
    firstRowLayout.addWidget(removeSlotButton);
    firstRowLayout.addWidget(addSlotButton);
    firstRow.setLayout(&firstRowLayout);
    grid.addWidget(&firstRow, GridRow::Header, 0, 1, -1, Qt::AlignLeft);

    QHBoxLayout lastRow;
    lastRow.addWidget(totalLabel);
    lastRow.addWidget(&buttonBox);

    // To keep the total and button box at the bottom of the window.
    QVBoxLayout layout(&dialog);
    QFrame *frameTop = new QFrame;
    frameTop->setLayout(&grid);
    layout.addWidget(frameTop);
    QFrame *frameBottom = new QFrame;
    frameBottom->setLayout(&lastRow);
    layout.addWidget(frameBottom);

    if (dialog.exec() == QDialog::Accepted) {
        updateEncounterFields(tempFields);

        // Re-draw the tab accordingly.
        displayWildMonTables();
        saveEncounterTabData();
        emit wildMonTableEdited();
    }
}

void Editor::saveEncounterTabData() {
    if (!this->map || !this->project)
        return;

    // This function does not save to disk so it is safe to use before user clicks Save.
    QStackedWidget *stack = ui->stackedWidget_WildMons;
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;

    if (!stack->count()) return;

    OrderedMap<QString, WildPokemonHeader> &encounterMap = project->wildMonData[map->constantName()];

    for (int groupIndex = 0; groupIndex < stack->count(); groupIndex++) {
        MonTabWidget *tabWidget = static_cast<MonTabWidget *>(stack->widget(groupIndex));

        WildPokemonHeader &encounterHeader = encounterMap[labelCombo->itemText(groupIndex)];

        int fieldIndex = 0;
        for (EncounterField monField : project->wildMonFields) {
            QString fieldName = monField.name;

            if (!tabWidget->isTabEnabled(fieldIndex++)) {
                encounterHeader.wildMons.erase(fieldName);
                continue;
            }

            QTableView *monTable = tabWidget->tableAt(fieldIndex - 1);
            EncounterTableModel *model = static_cast<EncounterTableModel *>(monTable->model());
            encounterHeader.wildMons[fieldName] = model->encounterData();
        }
    }
}

EncounterTableModel* Editor::getCurrentWildMonTable() {
    auto tabWidget = static_cast<MonTabWidget*>(ui->stackedWidget_WildMons->currentWidget());
    if (!tabWidget) return nullptr;

    auto tableView = tabWidget->tableAt(tabWidget->currentIndex());
    if (!tableView) return nullptr;

    return static_cast<EncounterTableModel*>(tableView->model());
}

void Editor::updateEncounterFields(EncounterFields newFields) {
    EncounterFields oldFields = project->wildMonFields;
    // Go through fields and determine whether we need to update a field.
    // If the field is new, do nothing.
    // If the field is deleted, remove from all maps.
    // If the field is changed, change all maps accordingly.
    for (EncounterField oldField : oldFields) {
        QString oldFieldName = oldField.name;
        bool fieldDeleted = true;
        for (EncounterField newField : newFields) {
            QString newFieldName = newField.name;
            if (oldFieldName == newFieldName) {
                fieldDeleted = false;
                if (oldField.encounterRates.size() != newField.encounterRates.size()) {
                    for (auto mapPair : project->wildMonData) {
                        QString map = mapPair.first;
                        for (auto groupNamePair : project->wildMonData[map]) {
                            QString groupName = groupNamePair.first;
                            WildPokemonHeader &monHeader = project->wildMonData[map][groupName];
                            for (auto fieldNamePair : monHeader.wildMons) {
                                QString fieldName = fieldNamePair.first;
                                if (fieldName == oldFieldName) {
                                    monHeader.wildMons[fieldName].wildPokemon.resize(newField.encounterRates.size());
                                }
                            }
                        }
                    }
                }
            }
        }
        if (fieldDeleted) {
            for (auto mapPair : project->wildMonData) {
                QString map = mapPair.first;
                for (auto groupNamePair : project->wildMonData[map]) {
                    QString groupName = groupNamePair.first;
                    WildPokemonHeader &monHeader = project->wildMonData[map][groupName];
                    for (auto fieldNamePair : monHeader.wildMons) {
                        QString fieldName = fieldNamePair.first;
                        if (fieldName == oldFieldName) {
                            monHeader.wildMons.erase(fieldName);
                        }
                    }
                }
            }
        }
    }
    project->wildMonFields = newFields;
}

void Editor::displayConnection(MapConnection *connection) {
    if (!connection)
        return;

    if (connection->isDiving()) {
        displayDivingConnection(connection);
        return;
    }

    // Create connection image
    auto pixmapItem = new ConnectionPixmapItem(connection);
    scene->addItem(pixmapItem);
    maskNonVisibleConnectionTiles();
    connect(pixmapItem, &ConnectionPixmapItem::positionChanged, this, &Editor::maskNonVisibleConnectionTiles);

    // Create item for the list panel
    auto listItem = new ConnectionsListItem(ui->scrollAreaContents_ConnectionsList, pixmapItem->connection, project->mapNames());
    ui->layout_ConnectionsList->insertWidget(ui->layout_ConnectionsList->count() - 1, listItem); // Insert above the vertical spacer

    // Double clicking the pixmap or clicking the list item's map button opens the connected map
    connect(listItem, &ConnectionsListItem::openMapClicked, this, &Editor::openConnectedMap);
    connect(pixmapItem, &ConnectionPixmapItem::connectionItemDoubleClicked, this, &Editor::openConnectedMap);

    // Pressing the delete key on a selected connection's pixmap deletes it
    connect(pixmapItem, &ConnectionPixmapItem::deleteRequested, this, &Editor::removeConnection);

    // Sync the selection highlight between the list UI and the pixmap
    connect(pixmapItem, &ConnectionPixmapItem::selectionChanged, [=](bool selected) {
        listItem->setSelected(selected);
        if (selected) setSelectedConnectionItem(pixmapItem);
    });
    connect(listItem, &ConnectionsListItem::selected, [=] {
        setSelectedConnectionItem(pixmapItem);
    });

    // When the pixmap is deleted, remove its associated list item
    connect(pixmapItem, &ConnectionPixmapItem::destroyed, listItem, &ConnectionsListItem::deleteLater);

    connection_items.append(pixmapItem);

    // If this was a recent addition from the user we should select it.
    // We intentionally exclude connections added programmatically, e.g. by mirroring.
    if (connection_to_select == connection) {
        connection_to_select = nullptr;
        setSelectedConnectionItem(pixmapItem);
    }
}

void Editor::addNewConnection(const QString &mapName, const QString &direction) {
    if (!this->map)
        return;

    MapConnection *connection = new MapConnection(mapName, direction);

    // Mark this connection to be selected once its display elements have been created.
    // It's possible this is a Dive/Emerge connection, but that's ok (no selection will occur).
    this->connection_to_select = connection;

    this->map->commit(new MapConnectionAdd(this->map, connection));
}

void Editor::replaceConnection(const QString &mapName, const QString &direction) {
    if (!this->map)
        return;

    MapConnection *connection = this->map->getConnection(direction);
    if (!connection || connection->targetMapName() == mapName)
        return;

    this->map->commit(new MapConnectionChangeMap(connection, mapName));
}

void Editor::removeConnection(MapConnection *connection) {
    if (!this->map || !connection)
        return;
    this->map->commit(new MapConnectionRemove(this->map, connection));
}

void Editor::removeSelectedConnection() {
    if (selected_connection_item)
        removeConnection(selected_connection_item->connection);
}

void Editor::removeConnectionPixmap(MapConnection *connection) {
    if (!connection)
        return;

    if (connection->isDiving()) {
        removeDivingMapPixmap(connection);
        return;
    }

    int i;
    for (i = 0; i < connection_items.length(); i++) {
        if (connection_items.at(i)->connection == connection)
            break;
    }
    if (i == connection_items.length())
        return; // Connection is not displayed, nothing to do.

    auto pixmapItem = connection_items.takeAt(i);
    if (pixmapItem == selected_connection_item) {
        // This was the selected connection, select the next one up in the list.
        selected_connection_item = nullptr;
        if (i != 0) i--;
        if (connection_items.length() > i)
            setSelectedConnectionItem(connection_items.at(i));
    }

    if (pixmapItem->scene())
        pixmapItem->scene()->removeItem(pixmapItem);

    delete pixmapItem;
}

void Editor::displayDivingConnection(MapConnection *connection) {
    if (!connection)
        return;

    const QString direction = connection->direction();
    if (!MapConnection::isDiving(direction))
        return;

    // Note: We only support editing 1 Dive and Emerge connection per map.
    //       In a vanilla game only the first Dive/Emerge connection is considered, so allowing
    //       users to have multiple is likely to lead to confusion. In case users have changed
    //       this we won't delete extra diving connections, but we'll only display the first one.
    if (diving_map_items.value(direction))
        return;

    // Create map display
    auto comboBox = (direction == "dive") ? ui->comboBox_DiveMap : ui->comboBox_EmergeMap;
    auto item = new DivingMapPixmapItem(connection, comboBox);
    scene->addItem(item);
    diving_map_items.insert(direction, item);

    updateDivingMapsVisibility();
}

void Editor::renderDivingConnections() {
    for (auto &item : diving_map_items)
        item->updatePixmap();
}

void Editor::removeDivingMapPixmap(MapConnection *connection) {
    if (!connection)
        return;

    const QString direction = connection->direction();
    if (!diving_map_items.contains(direction))
        return;

    // If the diving map being removed is different than the one that's currently displayed we don't need to do anything.
    if (diving_map_items.value(direction)->connection() != connection)
        return;

    // Delete map image
    auto pixmapItem = diving_map_items.take(direction);
    if (pixmapItem->scene())
        pixmapItem->scene()->removeItem(pixmapItem);
    delete pixmapItem;

    // Reveal any previously-hidden connection (because we only ever display one diving map of each type).
    // Note: When this occurs as a result of the user clicking the 'X' clear button it seems the QComboBox
    //       doesn't expect the line edit to be immediately repopulated, and the 'X' doesn't reappear.
    //       As a workaround we wait before displaying the new text. The wait time is essentially arbitrary.
    for (auto i : map->getConnections()) {
        if (i->direction() == direction) {
            QTimer::singleShot(10, Qt::CoarseTimer, [this, i]() { displayDivingConnection(i); });
            break;
        }
    }
    updateDivingMapsVisibility();
}

bool Editor::setDivingMapName(const QString &mapName, const QString &direction) {
    if (!mapName.isEmpty() && !this->project->isKnownMap(mapName))
        return false;
    if (!MapConnection::isDiving(direction))
        return false;

    auto pixmapItem = diving_map_items.value(direction);
    MapConnection *connection = pixmapItem ? pixmapItem->connection() : nullptr;

    if (connection) {
        if (mapName == connection->targetMapName())
            return true; // No change

        // Update existing connection
        if (mapName.isEmpty()) {
            removeConnection(connection);
        } else {
            map->commit(new MapConnectionChangeMap(connection, mapName));
        }
    } else if (!mapName.isEmpty()) {
        // Create new connection
        addNewConnection(mapName, direction);
    }
    return true;
}

QString Editor::getDivingMapName(const QString &direction) const {
    auto pixmapItem = diving_map_items.value(direction);
    return (pixmapItem && pixmapItem->connection()) ? pixmapItem->connection()->targetMapName() : QString();
}

void Editor::onDivingMapEditingFinished(NoScrollComboBox *combo, const QString &direction) {
    if (!setDivingMapName(combo->currentText(), direction)) {
        // If user input was invalid, restore the combo to the previously-valid text.
        combo->setTextItem(getDivingMapName(direction));
    }
}

void Editor::updateDivingMapButton(QToolButton* button, const QString &mapName) {
    if (this->project) button->setDisabled(!this->project->isKnownMap(mapName));
}

void Editor::updateDivingMapsVisibility() {
    auto dive = diving_map_items.value("dive");
    auto emerge = diving_map_items.value("emerge");

    if (dive && emerge) {
        // Both connections in use, use separate sliders
        ui->stackedWidget_DiveMapOpacity->setCurrentIndex(0);
        dive->setOpacity(!porymapConfig.showDiveEmergeMaps ? 0 : static_cast<qreal>(porymapConfig.diveMapOpacity) / 100);
        emerge->setOpacity(!porymapConfig.showDiveEmergeMaps ? 0 : static_cast<qreal>(porymapConfig.emergeMapOpacity) / 100);
    } else {
        // One connection in use (or none), use single slider
        ui->stackedWidget_DiveMapOpacity->setCurrentIndex(1);
        qreal opacity = !porymapConfig.showDiveEmergeMaps ? 0 : static_cast<qreal>(porymapConfig.diveEmergeMapOpacity) / 100;
        if (dive) dive->setOpacity(opacity);
        else if (emerge) emerge->setOpacity(opacity);
    }
}

void Editor::setSelectedConnectionItem(ConnectionPixmapItem *pixmapItem) {
    if (!pixmapItem || pixmapItem == selected_connection_item)
        return;

    if (selected_connection_item) selected_connection_item->setSelected(false);
    selected_connection_item = pixmapItem;
    selected_connection_item->setSelected(true);
}

void Editor::setSelectedConnection(MapConnection *connection) {
    if (!connection)
        return;

    for (auto item : connection_items) {
        if (item->connection == connection) {
            setSelectedConnectionItem(item);
            break;
        }
    }
}

void Editor::onBorderMetatilesChanged() {
    displayMapBorder();
    updateBorderVisibility();
}

void Editor::onHoveredMovementPermissionChanged(uint16_t collision, uint16_t elevation) {
    this->ui->statusBar->showMessage(this->getMovementPermissionText(collision, elevation));
}

void Editor::onHoveredMovementPermissionCleared() {
    this->ui->statusBar->clearMessage();
}

QString Editor::getMetatileDisplayMessage(uint16_t metatileId) {
    Metatile *metatile = Tileset::getMetatile(metatileId, this->layout->tileset_primary, this->layout->tileset_secondary);
    QString label = Tileset::getMetatileLabel(metatileId, this->layout->tileset_primary, this->layout->tileset_secondary);
    QString message = QString("Metatile: %1").arg(Metatile::getMetatileIdString(metatileId));
    if (label.size())
        message += QString(" \"%1\"").arg(label);
    if (metatile && metatile->behavior() != 0) { // Skip MB_NORMAL
        const QString behaviorStr = this->project->metatileBehaviorMapInverse.value(metatile->behavior(), Util::toHexString(metatile->behavior()));
        message += QString(", Behavior: %1").arg(behaviorStr);
    }
    return message;
}

void Editor::onHoveredMetatileSelectionChanged(uint16_t metatileId) {
    this->ui->statusBar->showMessage(getMetatileDisplayMessage(metatileId));
}

void Editor::onHoveredMetatileSelectionCleared() {
    this->ui->statusBar->clearMessage();
}

void Editor::onSelectedMetatilesChanged() {
    QPoint size = this->metatile_selector_item->getSelectionDimensions();
    this->cursorMapTileRect->updateSelectionSize(size.x(), size.y());
    this->redrawCurrentMetatilesSelection();
}

void Editor::onWheelZoom(int s) {
    // Don't zoom the map when the user accidentally scrolls while performing a magic fill. (ctrl + middle button click)
    if (!(QApplication::mouseButtons() & Qt::MiddleButton)) {
        scaleMapView(s);
    }
}

const QList<double> zoomLevels = QList<double>
{
    0.5,
    0.75,
    1.0,
    1.5,
    2.0,
    3.0,
    4.0,
    6.0,
};

void Editor::scaleMapView(int s) {
    // Clamp the scale index to a valid value.
    int nextScaleIndex = this->scaleIndex + s;
    if (nextScaleIndex < 0)
        nextScaleIndex = 0;
    if (nextScaleIndex >= zoomLevels.size())
        nextScaleIndex = zoomLevels.size() - 1;

    // Early exit if the scale index hasn't changed.
    if (nextScaleIndex == this->scaleIndex)
        return;

    // Set the graphics views' scale transformation based
    // on the new scale amount.
    this->scaleIndex = nextScaleIndex;
    double scaleFactor = zoomLevels[nextScaleIndex];
    QTransform transform = QTransform::fromScale(scaleFactor, scaleFactor);
    ui->graphicsView_Map->setTransform(transform);
    ui->graphicsView_Connections->setTransform(transform);
}

void Editor::setPlayerViewRect(const QRectF &rect) {
    delete this->playerViewRect;
    this->playerViewRect = new MovableRect(&this->settings->playerViewRectEnabled, rect, qRgb(255, 255, 255));
    this->playerViewRect->setActive(getEditingLayout());
    if (ui->graphicsView_Map->scene())
        ui->graphicsView_Map->scene()->update();
}

void Editor::updateCursorRectPos(int x, int y) {
    if (this->playerViewRect)
        this->playerViewRect->updateLocation(x, y);
    if (this->cursorMapTileRect)
        this->cursorMapTileRect->updateLocation(x, y);
    if (ui->graphicsView_Map->scene())
        ui->graphicsView_Map->scene()->update();
}

void Editor::setCursorRectVisible(bool visible) {
    if (this->playerViewRect)
        this->playerViewRect->setVisible(visible);
    if (this->cursorMapTileRect)
        this->cursorMapTileRect->setVisible(visible);
    if (ui->graphicsView_Map->scene())
        ui->graphicsView_Map->scene()->update();
}

void Editor::onHoveredMapMetatileChanged(const QPoint &pos) {
    int x = pos.x();
    int y = pos.y();
    if (!layout || !layout->isWithinBounds(x, y))
        return;

    this->updateCursorRectPos(x, y);
    if (this->getEditingLayout()) {
        int blockIndex = y * layout->getWidth() + x;
        int metatileId = layout->blockdata.at(blockIndex).metatileId();
        this->ui->statusBar->showMessage(QString("X: %1, Y: %2, %3, Scale = %4x")
                              .arg(x)
                              .arg(y)
                              .arg(getMetatileDisplayMessage(metatileId))
                              .arg(QString::number(zoomLevels[this->scaleIndex], 'g', 2)));
    }
    else if (this->editMode == EditMode::Events) {
        this->ui->statusBar->showMessage(QString("X: %1, Y: %2, Scale = %3x")
                              .arg(x)
                              .arg(y)
                              .arg(QString::number(zoomLevels[this->scaleIndex], 'g', 2)));
    }

    Scripting::cb_BlockHoverChanged(x, y);
}

void Editor::onHoveredMapMetatileCleared() {
    this->setCursorRectVisible(false);
    if (map_item->getEditsEnabled()) {
        this->ui->statusBar->clearMessage();
    }
    Scripting::cb_BlockHoverCleared();
}

void Editor::onHoveredMapMovementPermissionChanged(int x, int y) {
    if (!layout || !layout->isWithinBounds(x, y))
        return;

    this->updateCursorRectPos(x, y);
    if (this->getEditingLayout()) {
        int blockIndex = y * layout->getWidth() + x;
        uint16_t collision = layout->blockdata.at(blockIndex).collision();
        uint16_t elevation = layout->blockdata.at(blockIndex).elevation();
        QString message = QString("X: %1, Y: %2, %3")
                            .arg(x)
                            .arg(y)
                            .arg(this->getMovementPermissionText(collision, elevation));
        this->ui->statusBar->showMessage(message);
    }
    Scripting::cb_BlockHoverChanged(x, y);
}

void Editor::onHoveredMapMovementPermissionCleared() {
    this->setCursorRectVisible(false);
    if (this->getEditingLayout()) {
        this->ui->statusBar->clearMessage();
    }
    Scripting::cb_BlockHoverCleared();
}

QString Editor::getMovementPermissionText(uint16_t collision, uint16_t elevation) {
    QString message;
    if (collision != 0) {
        message = QString("Collision: Impassable (%1), Elevation: %2").arg(collision).arg(elevation);
    } else if (elevation == 0) {
        message = "Collision: Transition between elevations";
    } else if (elevation == 15) {
        message = "Collision: Multi-Level (Bridge)";
    } else if (elevation == 1) {
        message = "Collision: Surf";
    } else {
        message = QString("Collision: Passable, Elevation: %1").arg(elevation);
    }
    return message;
}

void Editor::unsetMap() {
    // disconnect previous map's signals so they are not firing
    // multiple times if set again in the future
    if (this->map) {
        this->map->pruneEditHistory();
        this->map->disconnect(this);

        // Don't let the file watcher accumulate map.json files.
        this->project->stopFileWatch(this->map->getJsonFilepath());
    }
    clearMapEvents();
    clearMapConnections();

    this->map = nullptr;
}

bool Editor::setMap(QString map_name) {
    if (!project || map_name.isEmpty()) {
        return false;
    }

    Map *loadedMap = project->loadMap(map_name);
    if (!loadedMap) {
        return false;
    }

    unsetMap();
    this->map = loadedMap;

    setLayout(map->layoutId());

    editGroup.addStack(map->editHistory());
    editGroup.setActiveStack(map->editHistory());

    this->selectedEvents.clear();
    if (!displayMap()) {
        return false;
    }
    displayWildMonTables();

    connect(map, &Map::openScriptRequested, this, &Editor::openScript);
    connect(map, &Map::connectionAdded, this, &Editor::displayConnection);
    connect(map, &Map::connectionRemoved, this, &Editor::removeConnectionPixmap);
    updateEvents();
    this->project->watchFile(map->getJsonFilepath());

    return true;
}

bool Editor::setLayout(QString layoutId) {
    if (!project || layoutId.isEmpty()) {
        return false;
    }

    QString prevLayoutName;
    if (this->layout) prevLayoutName = this->layout->name;

    Layout *loadedLayout = this->project->loadLayout(layoutId);
    if (!loadedLayout) {
        return false;
    }

    this->layout = loadedLayout;
    if (!displayLayout()) {
        return false;
    }

    editGroup.addStack(&this->layout->editHistory);

    map_ruler->setMapDimensions(QSize(this->layout->getWidth(), this->layout->getHeight()));
    connect(this->layout, &Layout::dimensionsChanged, map_ruler, &MapRuler::setMapDimensions);

    ui->comboBox_PrimaryTileset->blockSignals(true);
    ui->comboBox_SecondaryTileset->blockSignals(true);
    ui->comboBox_PrimaryTileset->setTextItem(this->layout->tileset_primary_label);
    ui->comboBox_SecondaryTileset->setTextItem(this->layout->tileset_secondary_label);
    ui->comboBox_PrimaryTileset->blockSignals(false);
    ui->comboBox_SecondaryTileset->blockSignals(false);

    const QSignalBlocker b0(this->ui->comboBox_LayoutSelector);
    int index = this->ui->comboBox_LayoutSelector->findText(layoutId);
    if (index < 0) index = 0;
    this->ui->comboBox_LayoutSelector->setCurrentIndex(index);

    if (this->layout->name != prevLayoutName)
        Scripting::cb_LayoutOpened(this->layout->name);

    return true;
}

void Editor::onMapStartPaint(QGraphicsSceneMouseEvent *event, LayoutPixmapItem *) {
    if (!this->getEditingLayout()) {
        return;
    }

    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());
    if (event->buttons() & Qt::RightButton && (mapEditAction == EditAction::Paint || mapEditAction == EditAction::Fill)) {
        this->cursorMapTileRect->initRightClickSelectionAnchor(pos.x(), pos.y());
    } else {
        this->cursorMapTileRect->initAnchor(pos.x(), pos.y());
    }
}

void Editor::onMapEndPaint(QGraphicsSceneMouseEvent *, LayoutPixmapItem *) {
    if (!this->getEditingLayout()) {
        return;
    }
    this->cursorMapTileRect->stopRightClickSelectionAnchor();
    this->cursorMapTileRect->stopAnchor();
}

void Editor::setSmartPathCursorMode(QGraphicsSceneMouseEvent *event)
{
    bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    if (settings->smartPathsEnabled) {
        if (!shiftPressed) {
            this->cursorMapTileRect->setSmartPathMode(true);
        } else {
            this->cursorMapTileRect->setSmartPathMode(false);
        }
    } else {
        if (shiftPressed) {
            this->cursorMapTileRect->setSmartPathMode(true);
        } else {
            this->cursorMapTileRect->setSmartPathMode(false);
        }
    }
}

void Editor::setStraightPathCursorMode(QGraphicsSceneMouseEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        this->cursorMapTileRect->setStraightPathMode(true);
    } else {
        this->cursorMapTileRect->setStraightPathMode(false);
    }
}

void Editor::mouseEvent_map(QGraphicsSceneMouseEvent *event, LayoutPixmapItem *item) {
    if (!item->getEditsEnabled()) {
        return;
    }

    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());

    if (this->getEditingLayout()) {
        if (mapEditAction == EditAction::Paint) {
            if (event->buttons() & Qt::RightButton) {
                item->updateMetatileSelection(event);
            } else if (event->buttons() & Qt::MiddleButton) {
                if (event->modifiers() & Qt::ControlModifier) {
                    item->magicFill(event);
                } else {
                    item->floodFill(event);
                }
            } else {
                if (event->type() == QEvent::GraphicsSceneMouseRelease) {
                    // Update the tile rectangle at the end of a click-drag selection
                    this->updateCursorRectPos(pos.x(), pos.y());
                }
                this->setSmartPathCursorMode(event);
                this->setStraightPathCursorMode(event);
                if (this->cursorMapTileRect->getStraightPathMode()) {
                    item->lockNondominantAxis(event);
                    pos = item->adjustCoords(pos);
                }
                item->paint(event);
            }
        } else if (mapEditAction == EditAction::Select) {
            item->select(event);
        } else if (mapEditAction == EditAction::Fill) {
            if (event->buttons() & Qt::RightButton) {
                item->updateMetatileSelection(event);
            } else if (event->modifiers() & Qt::ControlModifier) {
                item->magicFill(event);
            } else {
                item->floodFill(event);
            }
        } else if (mapEditAction == EditAction::Pick) {
            if (event->buttons() & Qt::RightButton) {
                item->updateMetatileSelection(event);
            } else if (event->type() != QEvent::GraphicsSceneMouseRelease) {
                item->pick(event);
            }
        } else if (mapEditAction == EditAction::Shift) {
            this->setStraightPathCursorMode(event);
            if (this->cursorMapTileRect->getStraightPathMode()) {
                item->lockNondominantAxis(event);
                pos = item->adjustCoords(pos);
            }
            item->shift(event);
        }
    } else if (this->editMode == EditMode::Events) {
        if (eventEditAction == EditAction::Paint && event->type() == QEvent::GraphicsSceneMousePress) {
            // Right-clicking while in paint mode will change mode to select.
            if (event->buttons() & Qt::RightButton) {
                setEditAction(EditAction::Select);
            } else {
                // Left-clicking while in paint mode will add a new event of the
                // type of the first currently selected events.
                Event::Type eventType = Event::Type::Object;
                if (!this->selectedEvents.isEmpty())
                    eventType = this->selectedEvents.first()->getEventType();

                Event* event = addNewEvent(eventType);
                if (event && event->getPixmapItem())
                    event->getPixmapItem()->moveTo(pos);
            }
        } else if (eventEditAction == EditAction::Select && event->type() == QEvent::GraphicsSceneMousePress) {
            if (!(event->modifiers() & Qt::ControlModifier) && this->selectedEvents.length() > 1) {
                // User is clearing group selection by clicking on the background
                selectMapEvent(this->selectedEvents.first());
            }
        } else if (eventEditAction == EditAction::Shift) {
            static QPoint selection_origin;

            if (event->type() == QEvent::GraphicsSceneMouseRelease) {
                this->eventShiftActionId++;
            } else {
                if (event->type() == QEvent::GraphicsSceneMousePress) {
                    selection_origin = QPoint(pos.x(), pos.y());
                } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
                    if (pos.x() != selection_origin.x() || pos.y() != selection_origin.y()) {
                        int xDelta = pos.x() - selection_origin.x();
                        int yDelta = pos.y() - selection_origin.y();
                        selection_origin = QPoint(pos.x(), pos.y());

                        this->map->commit(new EventShift(this->map->getEvents(), xDelta, yDelta, this->eventShiftActionId));
                    }
                }
            }
        }
    }
}

void Editor::mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item) {
    if (!item->getEditsEnabled()) {
        return;
    }

    QPoint pos = Metatile::coordFromPixmapCoord(event->pos());

    if (mapEditAction == EditAction::Paint) {
        if (event->buttons() & Qt::RightButton) {
            item->updateMovementPermissionSelection(event);
        } else if (event->buttons() & Qt::MiddleButton) {
            if (event->modifiers() & Qt::ControlModifier) {
                item->magicFill(event);
            } else {
                item->floodFill(event);
            }
        } else {
            this->setStraightPathCursorMode(event);
            if (this->cursorMapTileRect->getStraightPathMode()) {
                item->lockNondominantAxis(event);
                pos = item->adjustCoords(pos);
            }
            item->paint(event);
        }
    } else if (mapEditAction == EditAction::Select) {
        item->select(event);
    } else if (mapEditAction == EditAction::Fill) {
        if (event->buttons() & Qt::RightButton) {
            item->pick(event);
        } else if (event->modifiers() & Qt::ControlModifier) {
            item->magicFill(event);
        } else {
            item->floodFill(event);
        }
    } else if (mapEditAction == EditAction::Pick) {
        item->pick(event);
    } else if (mapEditAction == EditAction::Shift) {
        this->setStraightPathCursorMode(event);
        if (this->cursorMapTileRect->getStraightPathMode()) {
            item->lockNondominantAxis(event);
            pos = item->adjustCoords(pos);
        }
        item->shift(event);
    }
}

// On project close we want to leave the editor view empty.
// Otherwise a map is normally only cleared when a new one is being displayed.
void Editor::clearMap() {
    clearMetatileSelector();
    clearMovementPermissionSelector();
    clearMapMetatiles();
    clearMapMovementPermissions();
    clearBorderMetatiles();
    clearCurrentMetatilesSelection();
    clearMapEvents();
    clearMapConnections();
    clearMapBorder();
    clearMapGrid();
    clearWildMonTables();
    clearConnectionMask();

    // Clear pointers to objects deleted elsewhere
    current_view = nullptr;
    map = nullptr;

    // These are normally preserved between map displays, we only delete them now.
    if (scene) {
        scene->removeItem(this->map_ruler);
        delete scene;
    }
    delete metatile_selector_item;
    delete movement_permissions_selector_item;
}

bool Editor::displayMap() {
    if (!this->map)
        return false;

    displayMapEvents();
    displayMapConnections();
    maskNonVisibleConnectionTiles();
    return true;
}

bool Editor::displayLayout() {
    if (!this->layout)
        return false;

    if (!scene) {
        scene = new QGraphicsScene;
        MapSceneEventFilter *filter = new MapSceneEventFilter(scene);
        scene->installEventFilter(filter);
        connect(filter, &MapSceneEventFilter::wheelZoom, this, &Editor::onWheelZoom);
        scene->installEventFilter(this->map_ruler);
        this->map_ruler->setZValue(ZValue::Ruler);
        scene->addItem(this->map_ruler);
    }

    displayMetatileSelector();
    displayMapMetatiles();
    displayMovementPermissionSelector();
    displayMapMovementPermissions();
    displayBorderMetatiles();
    displayCurrentMetatilesSelection();
    displayMapBorder();
    displayMapGrid();
    maskNonVisibleConnectionTiles();

    if (map_item) {
        map_item->setVisible(false);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }

    return true;
}

void Editor::clearMetatileSelector() {
    if (metatile_selector_item && metatile_selector_item->scene()) {
        metatile_selector_item->scene()->removeItem(metatile_selector_item);
        delete scene_metatiles;
    }
}

void Editor::displayMetatileSelector() {
    clearMetatileSelector();

    scene_metatiles = new QGraphicsScene;
    if (!metatile_selector_item) {
        metatile_selector_item = new MetatileSelector(8, this->layout);
        connect(metatile_selector_item, &MetatileSelector::hoveredMetatileSelectionChanged,
                this, &Editor::onHoveredMetatileSelectionChanged);
        connect(metatile_selector_item, &MetatileSelector::hoveredMetatileSelectionCleared,
                this, &Editor::onHoveredMetatileSelectionCleared);
        connect(metatile_selector_item, &MetatileSelector::selectedMetatilesChanged,
                this, &Editor::onSelectedMetatilesChanged);
        metatile_selector_item->select(0);
    } else {
        metatile_selector_item->setLayout(this->layout);
        if (metatile_selector_item->primaryTileset
         && metatile_selector_item->primaryTileset != this->layout->tileset_primary)
            emit tilesetUpdated(this->layout->tileset_primary->name);
        if (metatile_selector_item->secondaryTileset
         && metatile_selector_item->secondaryTileset != this->layout->tileset_secondary)
            emit tilesetUpdated(this->layout->tileset_secondary->name);
        metatile_selector_item->setTilesets(this->layout->tileset_primary, this->layout->tileset_secondary);
    }

    scene_metatiles->addItem(metatile_selector_item);
}

void Editor::clearMapMetatiles() {
    if (map_item && scene) {
        scene->removeItem(map_item);
        delete map_item;
    }
}

void Editor::displayMapMetatiles() {
    clearMapMetatiles();

    map_item = new LayoutPixmapItem(this->layout, this->metatile_selector_item, this->settings);
    connect(map_item, &LayoutPixmapItem::mouseEvent, this, &Editor::mouseEvent_map);
    connect(map_item, &LayoutPixmapItem::startPaint, this, &Editor::onMapStartPaint);
    connect(map_item, &LayoutPixmapItem::endPaint, this, &Editor::onMapEndPaint);
    connect(map_item, &LayoutPixmapItem::hoveredMapMetatileChanged, this, &Editor::onHoveredMapMetatileChanged);
    connect(map_item, &LayoutPixmapItem::hoveredMapMetatileCleared, this, &Editor::onHoveredMapMetatileCleared);

    map_item->draw(true);
    scene->addItem(map_item);

    // Scene rect is the map plus a margin that gives enough space to scroll and see the edge of the player view rectangle.
    scene->setSceneRect(this->layout->getVisibleRect() + QMargins(3,3,3,3));
}

void Editor::clearMapMovementPermissions() {
    if (collision_item && scene) {
        scene->removeItem(collision_item);
        delete collision_item;
    }
}

void Editor::displayMapMovementPermissions() {
    clearMapMovementPermissions();

    collision_item = new CollisionPixmapItem(this->layout, ui->spinBox_SelectedCollision, ui->spinBox_SelectedElevation,
                                             this->metatile_selector_item, this->settings, &this->collisionOpacity);
    connect(collision_item, &CollisionPixmapItem::mouseEvent, this, &Editor::mouseEvent_collision);
    connect(collision_item, &CollisionPixmapItem::hoveredMapMovementPermissionChanged,
            this, &Editor::onHoveredMapMovementPermissionChanged);
    connect(collision_item, &CollisionPixmapItem::hoveredMapMovementPermissionCleared,
            this, &Editor::onHoveredMapMovementPermissionCleared);

    collision_item->draw(true);
    scene->addItem(collision_item);
}

void Editor::clearBorderMetatiles() {
    if (selected_border_metatiles_item && selected_border_metatiles_item->scene()) {
        selected_border_metatiles_item->scene()->removeItem(selected_border_metatiles_item);
        delete selected_border_metatiles_item;
        delete scene_selected_border_metatiles;
    }
}

void Editor::displayBorderMetatiles() {
    clearBorderMetatiles();

    scene_selected_border_metatiles = new QGraphicsScene;
    selected_border_metatiles_item = new BorderMetatilesPixmapItem(this->layout, this->metatile_selector_item);
    selected_border_metatiles_item->draw();
    scene_selected_border_metatiles->addItem(selected_border_metatiles_item);

    connect(selected_border_metatiles_item, &BorderMetatilesPixmapItem::hoveredBorderMetatileSelectionChanged,
            this, &Editor::onHoveredMetatileSelectionChanged);
    connect(selected_border_metatiles_item, &BorderMetatilesPixmapItem::hoveredBorderMetatileSelectionCleared,
            this, &Editor::onHoveredMetatileSelectionCleared);
    connect(selected_border_metatiles_item, &BorderMetatilesPixmapItem::borderMetatilesChanged,
            this, &Editor::onBorderMetatilesChanged);
}

void Editor::clearCurrentMetatilesSelection() {
    if (current_metatile_selection_item && current_metatile_selection_item->scene()) {
        current_metatile_selection_item->scene()->removeItem(current_metatile_selection_item);
        delete current_metatile_selection_item;
        current_metatile_selection_item = nullptr;
        delete scene_current_metatile_selection;
    }
}

void Editor::displayCurrentMetatilesSelection() {
    clearCurrentMetatilesSelection();

    scene_current_metatile_selection = new QGraphicsScene;
    current_metatile_selection_item = new CurrentSelectedMetatilesPixmapItem(this->layout, this->metatile_selector_item);
    current_metatile_selection_item->draw();
    scene_current_metatile_selection->addItem(current_metatile_selection_item);
}

void Editor::redrawCurrentMetatilesSelection() {
    if (current_metatile_selection_item) {
        current_metatile_selection_item->setLayout(this->layout);
        current_metatile_selection_item->draw();
        emit currentMetatilesSelectionChanged();
    }
}

void Editor::clearMovementPermissionSelector() {
    if (movement_permissions_selector_item && movement_permissions_selector_item->scene()) {
        movement_permissions_selector_item->scene()->removeItem(movement_permissions_selector_item);
        delete scene_collision_metatiles;
    }
}

void Editor::displayMovementPermissionSelector() {
    clearMovementPermissionSelector();

    scene_collision_metatiles = new QGraphicsScene;
    if (!movement_permissions_selector_item) {
        movement_permissions_selector_item = new MovementPermissionsSelector(this->collisionSheetPixmap);
        connect(movement_permissions_selector_item, &MovementPermissionsSelector::hoveredMovementPermissionChanged,
                this, &Editor::onHoveredMovementPermissionChanged);
        connect(movement_permissions_selector_item, &MovementPermissionsSelector::hoveredMovementPermissionCleared,
                this, &Editor::onHoveredMovementPermissionCleared);
        connect(movement_permissions_selector_item, &SelectablePixmapItem::selectionChanged, [this](int x, int y, int, int) {
            this->setCollisionTabSpinBoxes(x, y);
        });
        movement_permissions_selector_item->select(projectConfig.defaultCollision, projectConfig.defaultElevation);
    }

    scene_collision_metatiles->addItem(movement_permissions_selector_item);
}

void Editor::clearMapEvents() {
    if (events_group) {
        if (events_group->scene()) {
            events_group->scene()->removeItem(events_group);
        }
        delete events_group;
        events_group = nullptr;
    }
    this->selectedEvents.clear();
}

void Editor::displayMapEvents() {
    clearMapEvents();

    events_group = new QGraphicsItemGroup;
    scene->addItem(events_group);

    const auto events = map->getEvents();
    if (!events.isEmpty()) {
        this->selectedEvents.append(events.first());
    }
    for (const auto &event : events) {
        addEventPixmapItem(event);
    }

    events_group->setHandlesChildEvents(false);
}

EventPixmapItem *Editor::addEventPixmapItem(Event *event) {
    this->project->loadEventPixmap(event);
    auto item = new EventPixmapItem(event);
    connect(item, &EventPixmapItem::doubleClicked, this, &Editor::openEventMap);
    connect(item, &EventPixmapItem::dragged, this, &Editor::onEventDragged);
    connect(item, &EventPixmapItem::released, this, &Editor::onEventReleased);
    connect(item, &EventPixmapItem::selected, this, &Editor::selectMapEvent);
    connect(item, &EventPixmapItem::posChanged, [this, event] { updateWarpEventWarning(event); });
    connect(item, &EventPixmapItem::yChanged, [this, item] { updateEventPixmapItemZValue(item); });
    updateWarpEventWarning(event);
    redrawEventPixmapItem(item);
    this->events_group->addToGroup(item);
    return item;
}

void Editor::removeEventPixmapItem(Event *event) {
    auto item = event->getPixmapItem();
    if (!item) return;

    this->events_group->removeFromGroup(item);
    this->selectedEvents.removeOne(event);

    event->setPixmapItem(nullptr);
    delete item;
}

void Editor::clearMapConnections() {
    for (auto &item : connection_items) {
        if (item->scene())
            item->scene()->removeItem(item);
        delete item;
    }
    connection_items.clear();

    ui->comboBox_DiveMap->setCurrentText("");
    ui->comboBox_EmergeMap->setCurrentText("");

    for (auto &item : diving_map_items) {
        if (item->scene())
            item->scene()->removeItem(item);
        delete item;
    }
    diving_map_items.clear();

    // Reset to single opacity slider
    ui->stackedWidget_DiveMapOpacity->setCurrentIndex(1);

    selected_connection_item = nullptr;
}

void Editor::displayMapConnections() {
    clearMapConnections();

    for (auto connection : map->getConnections())
        displayConnection(connection);

    if (!connection_items.isEmpty())
        setSelectedConnectionItem(connection_items.first());
}

void Editor::clearConnectionMask() {
    if (connection_mask) {
        if (connection_mask->scene()) {
            connection_mask->scene()->removeItem(connection_mask);
        }
        delete connection_mask;
        connection_mask = nullptr;
    }
}

// Hides connected map tiles that cannot be seen from the current map
void Editor::maskNonVisibleConnectionTiles() {
    clearConnectionMask();

    QPainterPath mask;
    mask.addRect(scene->itemsBoundingRect().toRect());
    mask.addRect(layout->getVisibleRect());

    // Mask the tiles with the current theme's background color.
    QPen pen(ui->graphicsView_Map->palette().color(QPalette::Active, QPalette::Base));
    QBrush brush(ui->graphicsView_Map->palette().color(QPalette::Active, QPalette::Base));

    connection_mask = scene->addPath(mask, pen, brush);
    connection_mask->setZValue(ZValue::MapConnectionMask);
}

void Editor::clearMapBorder() {
    for (QGraphicsPixmapItem* item : borderItems) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    borderItems.clear();
}

void Editor::displayMapBorder() {
    clearMapBorder();

    QPixmap pixmap = this->layout->renderBorder();
    const QMargins borderMargins = layout->getBorderMargins();
    for (int y = -borderMargins.top(); y < this->layout->getHeight() + borderMargins.bottom(); y += this->layout->getBorderHeight())
    for (int x = -borderMargins.left(); x < this->layout->getWidth() + borderMargins.right(); x += this->layout->getBorderWidth()) {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
        item->setX(x * 16);
        item->setY(y * 16);
        item->setZValue(ZValue::MapBorder);
        scene->addItem(item);
        borderItems.append(item);
    }
}

void Editor::updateMapBorder() {
    QPixmap pixmap = this->layout->renderBorder(true);
    for (auto item : this->borderItems) {
        item->setPixmap(pixmap);
    }
}

void Editor::updateMapConnections() {
    for (auto item : connection_items)
        item->render(true);
}

void Editor::toggleGrid(bool checked) {
    if (porymapConfig.showGrid == checked)
        return;
    porymapConfig.showGrid = checked;

    // Synchronize action and checkbox
    const QSignalBlocker b_Action(ui->actionShow_Grid);
    const QSignalBlocker b_Checkbox(ui->checkBox_ToggleGrid);
    ui->actionShow_Grid->setChecked(checked);
    ui->checkBox_ToggleGrid->setChecked(checked);

    this->mapGrid->setVisible(checked);

    if (ui->graphicsView_Map->scene())
        ui->graphicsView_Map->scene()->update();
}

void Editor::clearMapGrid() {
    delete this->mapGrid;
    this->mapGrid = nullptr;
}

void Editor::displayMapGrid() {
    clearMapGrid();

    // Note: The grid lines are not added to the scene. They need to be drawn on top of the overlay
    //       elements of the scripting API, so they're painted manually in MapView::drawForeground.
    this->mapGrid = new QGraphicsItemGroup();

    const int pixelMapWidth = this->layout->getWidth() * 16;
    const int pixelMapHeight = this->layout->getHeight() * 16;

    // The grid can be moved with a user-specified x/y offset. The grid's dash patterns will only wrap in full pattern increments,
    // so we draw an additional row/column outside the map that can be revealed as the offset changes.
    const int offsetX = (this->gridSettings.offsetX % this->gridSettings.width) - this->gridSettings.width;
    const int offsetY = (this->gridSettings.offsetY % this->gridSettings.height) - this->gridSettings.height;

    QPen pen;
    pen.setColor(this->gridSettings.color);

    // Create vertical lines
    pen.setDashPattern(this->gridSettings.getVerticalDashPattern());
    for (int i = offsetX; i <= pixelMapWidth; i += this->gridSettings.width) {
        auto line = new QGraphicsLineItem(i, offsetY, i, pixelMapHeight);
        line->setPen(pen);
        this->mapGrid->addToGroup(line);
    }

    // Create horizontal lines
    pen.setDashPattern(this->gridSettings.getHorizontalDashPattern());
    for (int i = offsetY; i <= pixelMapHeight; i += this->gridSettings.height) {
        auto line = new QGraphicsLineItem(offsetX, i, pixelMapWidth, i);
        line->setPen(pen);
        this->mapGrid->addToGroup(line);
    }

    this->mapGrid->setVisible(porymapConfig.showGrid);
}

void Editor::updateMapGrid() {
    displayMapGrid();
    if (ui->graphicsView_Map->scene())
        ui->graphicsView_Map->scene()->update();
}

void Editor::updatePrimaryTileset(QString tilesetLabel, bool forceLoad)
{
    if (this->layout->tileset_primary_label != tilesetLabel || forceLoad)
    {
        this->layout->tileset_primary_label = tilesetLabel;
        this->layout->tileset_primary = project->getTileset(tilesetLabel, forceLoad);
        layout->clearBorderCache();
    }
}

void Editor::updateSecondaryTileset(QString tilesetLabel, bool forceLoad)
{
    if (this->layout->tileset_secondary_label != tilesetLabel || forceLoad)
    {
        this->layout->tileset_secondary_label = tilesetLabel;
        this->layout->tileset_secondary = project->getTileset(tilesetLabel, forceLoad);
        layout->clearBorderCache();
    }
}

void Editor::toggleBorderVisibility(bool visible, bool enableScriptCallback)
{
    porymapConfig.showBorder = visible;
    updateBorderVisibility();
    if (enableScriptCallback)
        Scripting::cb_BorderVisibilityToggled(visible);
}

void Editor::updateBorderVisibility() {
    // On the connections tab the border is always visible, and the connections can be edited.
    bool editingConnections = (ui->mainTabBar->currentIndex() == MainTab::Connections);
    bool visible = (editingConnections || ui->checkBox_ToggleBorder->isChecked());

    // Update border
    const qreal borderOpacity = editingConnections ? 0.4 : 1;
    for (QGraphicsPixmapItem* item : borderItems) {
        item->setVisible(visible);
        item->setOpacity(borderOpacity);
    }

    // Update map connections
    for (ConnectionPixmapItem* item : connection_items) {
        item->setVisible(visible);
        item->setEditable(editingConnections);
        item->setEnabled(visible);

        // When connecting a map to itself we don't bother to re-render the map connections in real-time,
        // i.e. if the user paints a new metatile on the map this isn't immediately reflected in the connection.
        // We're rendering them now, so we take the opportunity to do a full re-render for self-connections.
        bool fullRender = (this->map && item->connection && this->map->name() == item->connection->targetMapName());
        item->render(fullRender);
    }
}

void Editor::updateCustomMapAttributes()
{
    map->setCustomAttributes(ui->mapCustomAttributesFrame->table()->getAttributes());
    map->modify();
}

Tileset* Editor::getCurrentMapPrimaryTileset()
{
    QString tilesetLabel = this->layout->tileset_primary_label;
    return project->getTileset(tilesetLabel);
}

void Editor::redrawAllEvents() {
    if (this->map) redrawEvents(this->map->getEvents());
}

void Editor::redrawEvents(const QList<Event*> &events) {
    for (const auto &event : events) {
        redrawEventPixmapItem(event->getPixmapItem());
    }
}

qreal Editor::getEventOpacity(const Event *event) const {
    // There are 4 possible opacities for an event's sprite:
    // - Off the Events tab, and the event overlay is off (0.0)
    // - Off the Events tab, and the event overlay is on (0.5)
    // - On the Events tab, and the event has a default sprite (0.7)
    // - On the Events tab, and the event has a custom sprite (1.0)
    if (this->editMode != EditMode::Events)
        return porymapConfig.eventOverlayEnabled ? 0.5 : 0.0;
    return event->getUsesDefaultPixmap() ? 0.7 : 1.0;
}

void Editor::redrawEventPixmapItem(EventPixmapItem *item) {
    if (!item) return;
    Event *event = item->getEvent();
    if (!event) return;

    if (this->editMode == EditMode::Events) {
        item->setAcceptedMouseButtons(Qt::AllButtons);
        item->setSelected(this->selectedEvents.contains(event));
    } else {
        // Can't interact with event pixmaps outside of event editing mode.
        // We could do setEnabled(false), but rather than ignoring the mouse events this
        // would reject them, which would prevent painting on the map behind the events.
        item->setAcceptedMouseButtons(Qt::NoButton);
        item->setSelected(false);
    }
    updateEventPixmapItemZValue(item);
    item->setOpacity(getEventOpacity(event));
    item->setShapeMode(porymapConfig.eventSelectionShapeMode);
    item->render(project);
}

void Editor::updateEventPixmapItemZValue(EventPixmapItem *item) {
    if (!item) return;
    Event *event = item->getEvent();
    if (!event) return;

    if (item->isSelected()) {
        item->setZValue(ZValue::EventMaximum);
    } else {
        item->setZValue(event->getY() + ((ZValue::EventMaximum - ZValue::EventMinimum) / 2));
    }
}

void Editor::onEventDragged(Event *event, const QPoint &oldPosition, const QPoint &newPosition) {
    if (!this->map || !this->map_item)
        return;

    this->map_item->hoveredMapMetatileChanged(newPosition);

    // Drag all the other selected events (if any) with it
    QList<Event*> draggedEvents;
    if (this->selectedEvents.contains(event)) {
        draggedEvents = this->selectedEvents;
    } else {
        draggedEvents.append(event);
    }

    QPoint moveDistance = newPosition - oldPosition;
    this->map->commit(new EventMove(draggedEvents, moveDistance.x(), moveDistance.y(), this->eventMoveActionId));
}

void Editor::onEventReleased(Event *, const QPoint &) {
    this->eventMoveActionId++;
}

// Warp events display a warning if they're not positioned on a metatile with a warp behavior.
void Editor::updateWarpEventWarning(Event *event) {
    if (porymapConfig.warpBehaviorWarningDisabled)
        return;
    if (!project || !map || !map->layout() || !event || event->getEventType() != Event::Type::Warp)
        return;
    Block block;
    Metatile * metatile = nullptr;
    WarpEvent * warpEvent = static_cast<WarpEvent*>(event);
    if (map->layout()->getBlock(warpEvent->getX(), warpEvent->getY(), &block)) {
        metatile = Tileset::getMetatile(block.metatileId(), map->layout()->tileset_primary, map->layout()->tileset_secondary);
    }
    // metatile may be null if the warp is in the map border. Display the warning in this case
    bool validWarpBehavior = metatile && projectConfig.warpBehaviors.contains(metatile->behavior());
    warpEvent->setWarningEnabled(!validWarpBehavior);
}

// The warp event behavior warning is updated whenever the event moves or the event selection changes.
// It does not respond to changes in the underlying metatile. To capture the common case of a user painting
// metatiles on the Map tab then returning to the Events tab we update the warnings for all selected warp
// events when the Events tab is opened. This does not cover the case where metatiles are painted while
// still on the Events tab, such as by Undo/Redo or the scripting API.
void Editor::updateWarpEventWarnings() {
    if (porymapConfig.warpBehaviorWarningDisabled)
        return;
    for (const auto &event : this->selectedEvents)
        updateWarpEventWarning(event);
}

void Editor::shouldReselectEvents() {
    selectNewEvents = true;
}

// TODO: This is frequently used to do more work than necessary.
void Editor::updateEvents() {
    redrawAllEvents();
    emit eventsChanged();
}

void Editor::selectMapEvent(Event *event, bool toggle) {
    if (!event)
        return;

    if (!toggle) {
        // Selecting just this event
        this->selectedEvents.clear();
        this->selectedEvents.append(event);
    } else if (!this->selectedEvents.contains(event)) {
        // Adding event to group selection
        this->selectedEvents.append(event);
    } else if (this->selectedEvents.length() > 1) {
        // Removing event from group selection
        this->selectedEvents.removeOne(event);
    } else {
        // Attempting to toggle the only currently-selected event.
        // Unselecting an event this way would be unexpected, so we ignore it.
        return;
    }
    updateEvents();
}

void Editor::selectedEventIndexChanged(int index, Event::Group eventGroup) {
    int event_offs = Event::getIndexOffset(eventGroup);
    index = index - event_offs;
    Event *event = this->map->getEvent(eventGroup, index);

    if (event) {
        selectMapEvent(event);
    } else {
        updateEvents();
    }
}

bool Editor::canAddEvents(const QList<Event*> &events) {
    if (!this->project || !this->map)
        return false;

    QMap<Event::Group, int> newEventCounts;
    for (const auto &event : events) {
        Event::Group group = event->getEventGroup();
        int maxEvents = this->project->getMaxEvents(group);
        if (this->map->getNumEvents(group) + newEventCounts[group]++ >= maxEvents) {
            return false;
        }
    }
    return true;
}

void Editor::duplicateSelectedEvents() {
    if (this->selectedEvents.isEmpty() || !project || !map || !current_view || this->getEditingLayout())
        return;

    QList<Event *> duplicatedEvents;
    for (const auto &event : this->selectedEvents) {
        duplicatedEvents.append(event->duplicate());
    }
    if (!canAddEvents(duplicatedEvents)) {
        WarningMessage::show(QStringLiteral("Unable to duplicate, the maximum number of events would be exceeded."), ui->graphicsView_Map);
        qDeleteAll(duplicatedEvents);
        return;
    }
    this->map->commit(new EventDuplicate(this, this->map, duplicatedEvents));
}

Event *Editor::addNewEvent(Event::Type type) {
    if (!this->project || !this->map)
        return nullptr;

    Event::Group group = Event::typeToGroup(type);
    int maxEvents = this->project->getMaxEvents(group);
    if (this->map->getNumEvents(group) >= maxEvents) {
        WarningMessage::show(QString("The maximum number of %1 events (%2) has been reached.").arg(Event::groupToString(group)).arg(maxEvents), ui->graphicsView_Map);
        return nullptr;
    }

    Event *event = Event::create(type);
    if (!event)
        return nullptr;

    event->setMap(this->map);
    event->setDefaultValues(this->project);

    // This will add the event to the map, create the event pixmap item, and select the event.
    this->map->commit(new EventCreate(this, this->map, event));

    auto pixmapItem = event->getPixmapItem();
    if (pixmapItem) {
        auto halfSize = ui->graphicsView_Map->size() / 2;
        auto centerPos = ui->graphicsView_Map->mapToScene(halfSize.width(), halfSize.height());
        pixmapItem->moveTo(Metatile::coordFromPixmapCoord(centerPos));
    }

    return event;
}

void Editor::deleteSelectedEvents() {
    if (this->selectedEvents.isEmpty() || !this->map || this->editMode != EditMode::Events)
        return;

    QList<Event*> eventsToDelete;
    bool skipWarning = porymapConfig.eventDeleteWarningDisabled;
    for (auto event : this->selectedEvents) {
        const QString idName = event->getIdName();
        if (skipWarning || idName.isEmpty()) {
            eventsToDelete.append(event);
        } else {
            // If an event with a ID #define is deleted, its ID is also deleted (by the user's project, not Porymap).
            // Warn the user about this and give them a chance to abort.
            WarningMessage msgBox(QStringLiteral("Deleting this event may also delete the constant listed below. This can stop your project from compiling.\n\n"
                                                 "Are you sure you want to delete this event?"),
                                  ui->graphicsView_Map);
            msgBox.setInformativeText(idName);
            msgBox.setIconPixmap(event->getPixmap());
            msgBox.setStandardButtons(QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.addButton(QStringLiteral("Delete"), QMessageBox::DestructiveRole);
            msgBox.setCheckBox(new QCheckBox(QStringLiteral("Don't warn me again")));

            QAbstractButton* deleteAllButton = nullptr;
            if (this->selectedEvents.length() > 1) {
                deleteAllButton = msgBox.addButton(QStringLiteral("Delete All"), QMessageBox::DestructiveRole);
                msgBox.addButton(QStringLiteral("Skip"), QMessageBox::NoRole);
            }

            msgBox.exec();
            auto clickedButton = msgBox.clickedButton();
            auto clickedRole = msgBox.buttonRole(clickedButton);
            porymapConfig.eventDeleteWarningDisabled = msgBox.checkBox()->isChecked();
            if (clickedRole == QMessageBox::DestructiveRole) {
                // Confirmed deleting this event.
                eventsToDelete.append(event);
                if (deleteAllButton && clickedButton == deleteAllButton) {
                    // Confirmed deleting all events, no more warning.
                    skipWarning = true;
                }
            } else if (clickedRole == QMessageBox::NoRole) {
                // Declined deleting this event.
                continue;
            } else if (clickedRole == QMessageBox::RejectRole) {
                // Canceled delete.
                return;
            }
        }
        // TODO: Are we just calling this to invalidate connections?
        event->setPixmapItem(event->getPixmapItem());
    }
    if (eventsToDelete.isEmpty())
        return;

    // Get the index for the event that should be selected after this event has been deleted.
    // Select event at next smallest index when deleting a single event.
    // If deleting multiple events, just let editor work out next selected.
    Event *nextSelectedEvent = nullptr;
    if (eventsToDelete.length() == 1) {
        Event *eventToDelete = eventsToDelete.first();
        Event::Group event_group = eventToDelete->getEventGroup();
        int index = this->map->getIndexOfEvent(eventToDelete);
        if (index != this->map->getNumEvents(event_group) - 1)
            index++;
        else
            index--;
        nextSelectedEvent = this->map->getEvent(event_group, index);
    }

    this->map->commit(new EventDelete(this, this->map, eventsToDelete, nextSelectedEvent));
}

void Editor::openMapScripts() const {
    openInTextEditor(map->getScriptsFilepath());
}

void Editor::openScript(const QString &scriptLabel) const {
    // Find the location of scriptLabel.
    QStringList scriptPaths(map->getScriptsFilepath());
    scriptPaths << project->getEventScriptsFilepaths();
    int lineNum = 0;
    QString scriptPath = scriptPaths.first();
    for (const auto &path : scriptPaths) {
        lineNum = ParseUtil::getScriptLineNumber(path, scriptLabel);
        if (lineNum != 0) {
            scriptPath = path;
            break;
        }
    }

    openInTextEditor(scriptPath, lineNum);
}

void Editor::openMapJson(const QString &mapName) const {
    openInTextEditor(Map::getJsonFilepath(mapName));
}

void Editor::openLayoutJson(const QString &layoutId) const {
    QString path = QDir::cleanPath(QString("%1/%2").arg(projectConfig.projectDir()).arg(projectConfig.getFilePath(ProjectFilePath::json_layouts)));
    QString idField = QString("\"id\": \"%1\",").arg(layoutId);
    openInTextEditor(path, ParseUtil::getJsonLineNumber(path, idField));
}

void Editor::openInTextEditor(const QString &path, int lineNum) {
    QString command = porymapConfig.textEditorGotoLine;
    if (command.isEmpty()) {
        // Open map scripts in the system's default editor.
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        if (command.contains("%F")) {
            if (command.contains("%L"))
                command.replace("%L", QString::number(lineNum));
            command.replace("%F", '\"' + path + '\"');
        } else {
            command += " \"" + path + '\"';
        }
        Editor::startDetachedProcess(command);
    }
}

void Editor::openProjectInTextEditor() const {
    QString command = porymapConfig.textEditorOpenFolder;
    if (command.contains("%D"))
        command.replace("%D", '\"' + project->root + '\"');
    else
        command += " \"" + project->root + '\"';
    startDetachedProcess(command);
}

bool Editor::startDetachedProcess(const QString &command, const QString &workingDirectory, qint64 *pid) {
    logInfo("Executing command: " + command);
    QProcess process;
#ifdef Q_OS_WIN
    QStringList arguments = ParseUtil::splitShellCommand(command);
    const QString program = arguments.takeFirst();
    QFileInfo programFileInfo(program);
    if (programFileInfo.isExecutable()) {
        process.setProgram(program);
        process.setArguments(arguments);
    } else {
        // program is a batch script (such as VSCode's 'code' script) and needs to be started by cmd.exe.
        process.setProgram(QProcessEnvironment::systemEnvironment().value("COMSPEC"));
        // Windows is finicky with quotes on the command-line. I can't explain why this difference is necessary.
        if (command.startsWith('"'))
            process.setNativeArguments("/c \"" + command + '"');
        else
            process.setArguments(QStringList() << "/c" << program << arguments);
    }
#else
    QStringList arguments = ParseUtil::splitShellCommand(command);
    process.setProgram(arguments.takeFirst());
    process.setArguments(arguments);
#endif
    process.setWorkingDirectory(workingDirectory);
    return process.startDetached(pid);
}

void Editor::setCollisionTabSpinBoxes(uint16_t collision, uint16_t elevation) {
    const QSignalBlocker blocker1(ui->spinBox_SelectedCollision);
    const QSignalBlocker blocker2(ui->spinBox_SelectedElevation);
    ui->spinBox_SelectedCollision->setValue(collision);
    ui->spinBox_SelectedElevation->setValue(elevation);
}

// Custom collision graphics may be provided by the user.
void Editor::setCollisionGraphics() {
    QString filepath = projectConfig.collisionSheetPath;

    QImage imgSheet;
    if (filepath.isEmpty()) {
        // No custom collision image specified, use the default.
        imgSheet = this->defaultCollisionImgSheet;
    } else {
        // Try to load custom collision image
        QString validPath = Project::getExistingFilepath(filepath);
        if (!validPath.isEmpty()) filepath = validPath; // Otherwise allow it to fail with the original path
        imgSheet = QImage(filepath);
        if (imgSheet.isNull()) {
            // Custom collision image failed to load, use default
            logWarn(QString("Failed to load custom collision image '%1', using default.").arg(filepath));
            imgSheet = this->defaultCollisionImgSheet;
        }
    }

    // Users are not required to provide an image that gives an icon for every elevation/collision combination.
    // Instead they tell us how many are provided in their image by specifying the number of columns and rows.
    const int imgColumns = projectConfig.collisionSheetSize.width();
    const int imgRows = projectConfig.collisionSheetSize.height();

    // Create a pixmap for the selector on the Collision tab. If a project was previously opened we'll also need to refresh the selector.
    this->collisionSheetPixmap = QPixmap::fromImage(imgSheet).scaled(MovementPermissionsSelector::CellWidth * imgColumns,
                                                                     MovementPermissionsSelector::CellHeight * imgRows);
    if (this->movement_permissions_selector_item)
        this->movement_permissions_selector_item->setBasePixmap(this->collisionSheetPixmap);

    for (auto sublist : collisionIcons)
        qDeleteAll(sublist);
    collisionIcons.clear();

    // Use the image sheet to create an icon for each collision/elevation combination.
    // Any icons for combinations that aren't provided by the image sheet are also created now using default graphics.
    const int w = 16, h = 16;
    imgSheet = imgSheet.scaled(w * imgColumns, h * imgRows);
    for (int collision = 0; collision <= Block::getMaxCollision(); collision++) {
        // If (collision >= imgColumns) here, it's a valid collision value, but it is not represented with an icon on the image sheet.
        // In this case we just use the rightmost collision icon. This is mostly to support the vanilla case, where technically 0-3
        // are valid collision values, but 1-3 have the same meaning, so the vanilla collision selector image only has 2 columns.
        int x = ((collision < imgColumns) ? collision : (imgColumns - 1)) * w;

        QList<const QImage*> sublist;
        for (int elevation = 0; elevation <= Block::getMaxElevation(); elevation++) {
            if (elevation < imgRows) {
                // This elevation has an icon on the image sheet, add it to the list
                int y = elevation * h;
                sublist.append(new QImage(imgSheet.copy(x, y, w, h)));
            } else {
                // This is a valid elevation value, but it has no icon on the image sheet.
                // Give it a placeholder "?" icon (red if impassable, white otherwise)
                sublist.append(new QImage(this->collisionPlaceholder.copy(x != 0 ? w : 0, 0, w, h)));
            }
        }
        collisionIcons.append(sublist);
    }
}
