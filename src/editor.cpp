#include "editor.h"
#include "event.h"
#include "imageproviders.h"
#include "log.h"
#include "mapconnection.h"
#include "currentselectedmetatilespixmapitem.h"
#include "mapsceneeventfilter.h"
#include "montabwidget.h"
#include <QCheckBox>
#include <QPainter>
#include <QMouseEvent>
#include <QDir>
#include <math.h>

static bool selectingEvent = false;

Editor::Editor(Ui::MainWindow* ui)
{
    this->ui = ui;
    this->selected_events = new QList<DraggablePixmapItem*>;
    this->settings = new Settings();
    this->playerViewRect = new MovableRect(&this->settings->playerViewRectEnabled, 30 * 8, 20 * 8, qRgb(255, 255, 255));
    this->cursorMapTileRect = new CursorTileRect(&this->settings->cursorTileRectEnabled, qRgb(255, 255, 255));
}

Editor::~Editor()
{
    delete this->selected_events;
    delete this->settings;
    delete this->playerViewRect;
    delete this->cursorMapTileRect;

    closeProject();
}

void Editor::saveProject() {
    if (project) {
        saveUiFields();
        project->saveAllMaps();
        project->saveAllDataStructures();
    }
}

void Editor::save() {
    if (project && map) {
        saveUiFields();
        project->saveMap(map);
        project->saveAllDataStructures();
    }
}

void Editor::saveUiFields() {
    saveEncounterTabData();
}

void Editor::undo() {
    if (current_view && map_item->paintingMode == MapPixmapItem::PaintMode::Metatiles) {
        map->undo();
        map_item->draw();
        collision_item->draw();
        selected_border_metatiles_item->draw();
        onBorderMetatilesChanged();
    }
}

void Editor::redo() {
    if (current_view && map_item->paintingMode == MapPixmapItem::PaintMode::Metatiles) {
        map->redo();
        map_item->draw();
        collision_item->draw();
        selected_border_metatiles_item->draw();
        onBorderMetatilesChanged();
    }
}

void Editor::closeProject() {
    if (this->project) {
        delete this->project;
        this->project = nullptr;
    }
}

void Editor::setEditingMap() {
    current_view = map_item;
    if (map_item) {
        map_item->paintingMode = MapPixmapItem::PaintMode::Metatiles;
        displayMapConnections();
        map_item->draw();
        map_item->setVisible(true);
        setConnectionsVisibility(ui->checkBox_ToggleBorder->isChecked());
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
    setConnectionItemsVisible(false);
    this->cursorMapTileRect->stopSingleTileMode();
    this->cursorMapTileRect->setVisibility(true);

    setMapEditingButtonsEnabled(true);
}

void Editor::setEditingCollision() {
    current_view = collision_item;
    if (collision_item) {
        displayMapConnections();
        collision_item->draw();
        collision_item->setVisible(true);
        setConnectionsVisibility(ui->checkBox_ToggleBorder->isChecked());
    }
    if (map_item) {
        map_item->paintingMode = MapPixmapItem::PaintMode::Metatiles;
        map_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
    setConnectionItemsVisible(false);
    this->cursorMapTileRect->setSingleTileMode();
    this->cursorMapTileRect->setVisibility(true);

    setMapEditingButtonsEnabled(true);
}

void Editor::setEditingObjects() {
    current_view = map_item;
    if (events_group) {
        events_group->setVisible(true);
    }
    if (map_item) {
        map_item->paintingMode = MapPixmapItem::PaintMode::EventObjects;
        displayMapConnections();
        map_item->draw();
        map_item->setVisible(true);
        setConnectionsVisibility(ui->checkBox_ToggleBorder->isChecked());
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
    setConnectionItemsVisible(false);
    this->cursorMapTileRect->setSingleTileMode();
    this->cursorMapTileRect->setVisibility(false);

    setMapEditingButtonsEnabled(false);
}

void Editor::setMapEditingButtonsEnabled(bool enabled) {
    this->ui->toolButton_Fill->setEnabled(enabled);
    this->ui->toolButton_Dropper->setEnabled(enabled);
    this->ui->pushButton_ChangeDimensions->setEnabled(enabled);
    // If the fill button is pressed, unpress it and select the pointer.
    if (!enabled && (this->ui->toolButton_Fill->isChecked() || this->ui->toolButton_Dropper->isChecked())) {
        this->map_edit_mode = "select";
        this->settings->mapCursor = QCursor();
        this->cursorMapTileRect->setSingleTileMode();
        this->ui->toolButton_Fill->setChecked(false);
        this->ui->toolButton_Dropper->setChecked(false);
        this->ui->toolButton_Select->setChecked(true);
    }
    this->ui->checkBox_smartPaths->setEnabled(enabled);
}

void Editor::setEditingConnections() {
    current_view = map_item;
    if (map_item) {
        map_item->paintingMode = MapPixmapItem::PaintMode::Disabled;
        map_item->draw();
        map_item->setVisible(true);
        populateConnectionMapPickers();
        ui->label_NumConnections->setText(QString::number(map->connections.length()));
        setConnectionsVisibility(false);
        setDiveEmergeControls();
        bool controlsEnabled = selected_connection_item != nullptr;
        setConnectionEditControlsEnabled(controlsEnabled);
        if (selected_connection_item) {
            onConnectionOffsetChanged(selected_connection_item->connection->offset.toInt());
            setConnectionMap(selected_connection_item->connection->map_name);
            setCurrentConnectionDirection(selected_connection_item->connection->direction);
        }
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    setBorderItemsVisible(true, 0.4);
    setConnectionItemsVisible(true);
    this->cursorMapTileRect->setSingleTileMode();
    this->cursorMapTileRect->setVisibility(false);
}

void Editor::displayWildMonTables() {
    QStackedWidget *stack = ui->stackedWidget_WildMons;
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;

    // delete widgets from previous map data if they exist
    while (stack->count()) {
        QWidget *oldWidget = stack->widget(0);
        stack->removeWidget(oldWidget);
        delete oldWidget;
    }

    labelCombo->clear();

    // Don't try to read encounter data if it doesn't exist on disk for this map.
    if (!project->wildMonData.contains(map->constantName)) {
        return;
    }

    for (auto groupPair : project->wildMonData[map->constantName])
        labelCombo->addItem(groupPair.first);

    labelCombo->setCurrentText(labelCombo->itemText(0));

    int labelIndex = 0;
    for (auto labelPair : project->wildMonData[map->constantName]) {

        QString label = labelPair.first;

        WildPokemonHeader header = project->wildMonData[map->constantName][label];

        MonTabWidget *tabWidget = new MonTabWidget(this);
        stack->insertWidget(labelIndex++, tabWidget);

        int tabIndex = 0;
        for (EncounterField monField : project->wildMonFields) {
            QString fieldName = monField.name;

            tabWidget->clearTableAt(tabIndex);

            if (project->wildMonData.contains(map->constantName) && header.wildMons[fieldName].active) {
                tabWidget->populateTab(tabIndex, header.wildMons[fieldName], fieldName);
            } else {
                tabWidget->setTabActive(tabIndex, false);
            }
            tabIndex++;
        }
    }
    stack->setCurrentIndex(0);
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
    form.addRow(new QLabel("Group Base Label:"), lineEdit);
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression("[_A-Za-z0-9]*"));
    lineEdit->setValidator(validator);
    connect(lineEdit, &QLineEdit::textChanged, [this, &lineEdit, &buttonBox](QString text){
        if (this->project->encounterGroupLabels.contains(text)) {
            QPalette palette = lineEdit->palette();
            QColor color = Qt::red;
            color.setAlpha(25);
            palette.setColor(QPalette::Base, color);
            lineEdit->setPalette(palette);
            buttonBox.button(QDialogButtonBox::Ok)->setDisabled(true);
        } else {
            lineEdit->setPalette(QPalette());
            buttonBox.button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    });
    // Give a default value to the label.
    lineEdit->setText(QString("g%1%2").arg(map->name).arg(stack->count()));

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
    connect(copyCheckbox, &QCheckBox::stateChanged, [=](int state){
        if (state == Qt::Checked) {
            int fieldIndex = 0;
            MonTabWidget *monWidget = static_cast<MonTabWidget *>(stack->widget(stack->currentIndex()));
            for (EncounterField monField : project->wildMonFields) {
                fieldCheckboxes[fieldIndex]->setChecked(monWidget->isTabEnabled(fieldIndex));
                fieldCheckboxes[fieldIndex]->setEnabled(false);
                fieldIndex++;
            }
        } else if (state == Qt::Unchecked) {
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
    connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        WildPokemonHeader header;
        for (EncounterField& monField : project->wildMonFields) {
            QString fieldName = monField.name;
            header.wildMons[fieldName].active = false;
            header.wildMons[fieldName].encounterRate = 0;
        }

        MonTabWidget *tabWidget = new MonTabWidget(this);
        stack->insertWidget(stack->count(), tabWidget);

        labelCombo->addItem(lineEdit->text());
        labelCombo->setCurrentIndex(labelCombo->count() - 1);

        int tabIndex = 0;
        for (EncounterField &monField : project->wildMonFields) {
            QString fieldName = monField.name;
            tabWidget->clearTableAt(tabIndex);
            if (fieldCheckboxes[tabIndex]->isChecked()) {
                if (copyCheckbox->isChecked()) {
                    MonTabWidget *copyFrom = static_cast<MonTabWidget *>(stack->widget(stackIndex));
                    if (copyFrom->isTabEnabled(tabIndex))
                        header.wildMons.insert(fieldName, copyMonInfoFromTab(copyFrom->tableAt(tabIndex), monField));
                    else
                        header.wildMons.insert(fieldName, getDefaultMonInfo(monField));
                } else {
                    header.wildMons.insert(fieldName, getDefaultMonInfo(monField));
                }
                tabWidget->populateTab(tabIndex, header.wildMons[fieldName], fieldName);
            } else {
                tabWidget->setTabActive(tabIndex, false);
            }
            tabIndex++;
        }
        saveEncounterTabData();
        emit wildMonDataChanged();
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
        for (QString key : currentField.groups.keys())
            groupTotals.insert(key, 0);// add to group map and initialize total to zero
        for (auto slot : fieldSlots) {
            QSpinBox *spinner = slot->findChild<QSpinBox *>();
            int val = spinner->value();
            currentField.encounterRates[spinnerIndex] = val;
            if (!currentField.groups.isEmpty()) {
                for (QString key : currentField.groups.keys()) {
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
        if (!currentField.groups.isEmpty()) {
            groupTotalMessage += "Totals: ";
            for (QString key : currentField.groups.keys()) {
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
        connect(chanceSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [&chanceSpinner, &updateTotal, &currentField](int) {
            updateTotal(currentField);
        });

        bool useGroups = !currentField.groups.isEmpty();

        QFrame *slotChoiceFrame = new QFrame;
        QVBoxLayout *slotChoiceLayout = new QVBoxLayout;
        if (useGroups) {
            QComboBox *groupCombo = new QComboBox;
            connect(groupCombo, QOverload<const QString &>::of(&QComboBox::activated), [&tempFields, &currentField, index](QString newGroupName) {
                for (EncounterField &field : tempFields) {
                    if (field.name == currentField.name) {
                        for (QString groupName : field.groups.keys()) {
                            if (field.groups[groupName].contains(index)) {
                                field.groups[groupName].removeAll(index);
                                break;
                            }
                        }
                        for (QString groupName : field.groups.keys()) {
                            if (groupName == newGroupName) field.groups[newGroupName].append(index);
                        }
                        break;
                    }
                }
            });
            groupCombo->addItems(currentField.groups.keys());
            QString currentGroupName;
            for (QString groupName : currentField.groups.keys()) {
                if (currentField.groups[groupName].contains(index)) {
                    currentGroupName = groupName;
                    break;
                }
            }
            groupCombo->setCurrentText(currentGroupName);
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

    QGridLayout grid;

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);

    connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    // lambda: Get a QStringList of the existing field names.
    auto getFieldNames = [&tempFields]() {
        QStringList fieldNames;
        for (EncounterField field : tempFields)
            fieldNames.append(field.name);
        return fieldNames;
    };

    // lambda: Draws the slot widgets onto a grid (4 wide) on the dialog window.
    auto drawSlotWidgets = [this, &dialog, &grid, &createNewSlot, &fieldSlots, &updateTotal, &tempFields](int index) {
        // Clear them first.
        while (!fieldSlots.isEmpty()) {
            auto slot = fieldSlots.takeFirst();
            grid.removeWidget(slot);
            delete slot;
        }

        EncounterField &currentField = tempFields[index];
        for (int i = 0; i < currentField.encounterRates.size(); i++) {
            grid.addWidget(createNewSlot(i, currentField), i / 4 + 1, i % 4);
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
        connect(&newFieldButtonBox, SIGNAL(accepted()), &newNameDialog, SLOT(accept()));
        connect(&newFieldButtonBox, SIGNAL(rejected()), &newNameDialog, SLOT(reject()));

        QLineEdit *newNameEdit = new QLineEdit;

        QFormLayout newFieldForm(&newNameDialog);

        newFieldForm.addRow("Field Name", newNameEdit);
        newFieldForm.addRow(&newFieldButtonBox);

        if (newNameDialog.exec() == QDialog::Accepted) {
            QString newFieldName = newNameEdit->text();
            QVector<int> newFieldRates(1, 100);
            tempFields.append({newFieldName, newFieldRates, {}});
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
    connect(addSlotButton, &QPushButton::clicked, [this, &fieldChoices, &drawSlotWidgets, &tempFields]() {
        EncounterField &field = tempFields[fieldChoices->currentIndex()];
        field.encounterRates.append(1);
        drawSlotWidgets(fieldChoices->currentIndex());
    });
    QPushButton *removeSlotButton = new QPushButton(QIcon(":/icons/delete.ico"), "");
    removeSlotButton->setFlat(true);
    connect(removeSlotButton, &QPushButton::clicked, [this, &fieldChoices, &drawSlotWidgets, &tempFields]() {
        EncounterField &field = tempFields[fieldChoices->currentIndex()];
        if (field.encounterRates.size() > 1)
            field.encounterRates.removeLast();
        drawSlotWidgets(fieldChoices->currentIndex());
    });

    QFrame firstRow;
    QHBoxLayout firstRowLayout;
    firstRowLayout.addWidget(fieldChoiceLabel);
    firstRowLayout.addWidget(fieldChoices);
    firstRowLayout.addWidget(deleteFieldButton);
    firstRowLayout.addWidget(addFieldButton);
    firstRowLayout.addWidget(removeSlotButton);
    firstRowLayout.addWidget(addSlotButton);
    firstRow.setLayout(&firstRowLayout);
    grid.addWidget(&firstRow, 0, 0, 1, 4, Qt::AlignLeft);

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
        emit wildMonDataChanged();
    }
}

void Editor::saveEncounterTabData() {
    // This function does not save to disk so it is safe to use before user clicks Save.
    QStackedWidget *stack = ui->stackedWidget_WildMons;
    QComboBox *labelCombo = ui->comboBox_EncounterGroupLabel;

    if (!stack->count()) return;

    tsl::ordered_map<QString, WildPokemonHeader> &encounterMap = project->wildMonData[map->constantName];

    for (int groupIndex = 0; groupIndex < stack->count(); groupIndex++) {
        MonTabWidget *tabWidget = static_cast<MonTabWidget *>(stack->widget(groupIndex));

        WildPokemonHeader &encounterHeader = encounterMap[labelCombo->itemText(groupIndex)];

        int fieldIndex = 0;
        for (EncounterField monField : project->wildMonFields) {
            QString fieldName = monField.name;

            if (!tabWidget->isTabEnabled(fieldIndex++)) continue;

            QTableWidget *monTable = static_cast<QTableWidget *>(tabWidget->widget(fieldIndex - 1));
            QVector<WildPokemon> newWildMons;
            encounterHeader.wildMons[fieldName] = copyMonInfoFromTab(monTable, monField);
        }
    }
}

// Update encounters for every map based on the new encounter JSON field data.
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
                            for (QString fieldName : monHeader.wildMons.keys()) {
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
                    for (QString fieldName : monHeader.wildMons.keys()) {
                        if (fieldName == oldFieldName) {
                            monHeader.wildMons.remove(fieldName);
                        }
                    }
                }
            }
        }
    }
    project->wildMonFields = newFields;
}

void Editor::setDiveEmergeControls() {
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);
    ui->comboBox_DiveMap->setCurrentText("");
    ui->comboBox_EmergeMap->setCurrentText("");
    for (MapConnection* connection : map->connections) {
        if (connection->direction == "dive") {
            ui->comboBox_DiveMap->setCurrentText(connection->map_name);
        } else if (connection->direction == "emerge") {
            ui->comboBox_EmergeMap->setCurrentText(connection->map_name);
        }
    }
    ui->comboBox_DiveMap->blockSignals(false);
    ui->comboBox_EmergeMap->blockSignals(false);
}

void Editor::populateConnectionMapPickers() {
    ui->comboBox_ConnectedMap->blockSignals(true);
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);

    ui->comboBox_ConnectedMap->clear();
    ui->comboBox_ConnectedMap->addItems(*project->mapNames);
    ui->comboBox_DiveMap->clear();
    ui->comboBox_DiveMap->addItems(*project->mapNames);
    ui->comboBox_EmergeMap->clear();
    ui->comboBox_EmergeMap->addItems(*project->mapNames);

    ui->comboBox_ConnectedMap->blockSignals(false);
    ui->comboBox_DiveMap->blockSignals(true);
    ui->comboBox_EmergeMap->blockSignals(true);
}

void Editor::setConnectionItemsVisible(bool visible) {
    for (ConnectionPixmapItem* item : connection_edit_items) {
        item->setVisible(visible);
        item->setEnabled(visible);
    }
}

void Editor::setBorderItemsVisible(bool visible, qreal opacity) {
    for (QGraphicsPixmapItem* item : borderItems) {
        item->setVisible(visible);
        item->setOpacity(opacity);
    }
}

void Editor::setCurrentConnectionDirection(QString curDirection) {
    if (!selected_connection_item)
        return;
    Map *connected_map = project->getMap(selected_connection_item->connection->map_name);
    if (!connected_map) {
        return;
    }

    selected_connection_item->connection->direction = curDirection;

    QPixmap pixmap = connected_map->renderConnection(*selected_connection_item->connection, map->layout);
    int offset = selected_connection_item->connection->offset.toInt(nullptr, 0);
    selected_connection_item->initialOffset = offset;
    int x = 0, y = 0;
    if (selected_connection_item->connection->direction == "up") {
        x = offset * 16;
        y = -pixmap.height();
    } else if (selected_connection_item->connection->direction == "down") {
        x = offset * 16;
        y = map->getHeight() * 16;
    } else if (selected_connection_item->connection->direction == "left") {
        x = -pixmap.width();
        y = offset * 16;
    } else if (selected_connection_item->connection->direction == "right") {
        x = map->getWidth() * 16;
        y = offset * 16;
    }

    selected_connection_item->basePixmap = pixmap;
    QPainter painter(&pixmap);
    painter.setPen(QColor(255, 0, 255));
    painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
    painter.end();
    selected_connection_item->setPixmap(pixmap);
    selected_connection_item->initialX = x;
    selected_connection_item->initialY = y;
    selected_connection_item->blockSignals(true);
    selected_connection_item->setX(x);
    selected_connection_item->setY(y);
    selected_connection_item->setZValue(-1);
    selected_connection_item->blockSignals(false);

    setConnectionEditControlValues(selected_connection_item->connection);
}

void Editor::updateCurrentConnectionDirection(QString curDirection) {
    if (!selected_connection_item)
        return;

    QString originalDirection = selected_connection_item->connection->direction;
    setCurrentConnectionDirection(curDirection);
    updateMirroredConnectionDirection(selected_connection_item->connection, originalDirection);
}

void Editor::onConnectionMoved(MapConnection* connection) {
    updateMirroredConnectionOffset(connection);
    onConnectionOffsetChanged(connection->offset.toInt());
}

void Editor::onConnectionOffsetChanged(int newOffset) {
    ui->spinBox_ConnectionOffset->blockSignals(true);
    ui->spinBox_ConnectionOffset->setValue(newOffset);
    ui->spinBox_ConnectionOffset->blockSignals(false);

}

void Editor::setConnectionEditControlValues(MapConnection* connection) {
    QString mapName = connection ? connection->map_name : "";
    QString direction = connection ? connection->direction : "";
    int offset = connection ? connection->offset.toInt() : 0;

    ui->comboBox_ConnectedMap->blockSignals(true);
    ui->comboBox_ConnectionDirection->blockSignals(true);
    ui->spinBox_ConnectionOffset->blockSignals(true);

    ui->comboBox_ConnectedMap->setCurrentText(mapName);
    ui->comboBox_ConnectionDirection->setCurrentText(direction);
    ui->spinBox_ConnectionOffset->setValue(offset);

    ui->comboBox_ConnectedMap->blockSignals(false);
    ui->comboBox_ConnectionDirection->blockSignals(false);
    ui->spinBox_ConnectionOffset->blockSignals(false);
}

void Editor::setConnectionEditControlsEnabled(bool enabled) {
    ui->comboBox_ConnectionDirection->setEnabled(enabled);
    ui->comboBox_ConnectedMap->setEnabled(enabled);
    ui->spinBox_ConnectionOffset->setEnabled(enabled);

    if (!enabled) {
        setConnectionEditControlValues(nullptr);
    }
}

void Editor::onConnectionItemSelected(ConnectionPixmapItem* connectionItem) {
    if (!connectionItem)
        return;

    for (ConnectionPixmapItem* item : connection_edit_items) {
        bool isSelectedItem = item == connectionItem;
        int zValue = isSelectedItem ? 0 : -1;
        qreal opacity = isSelectedItem ? 1 : 0.75;
        item->setZValue(zValue);
        item->render(opacity);
        if (isSelectedItem) {
            QPixmap pixmap = item->pixmap();
            QPainter painter(&pixmap);
            painter.setPen(QColor(255, 0, 255));
            painter.drawRect(0, 0, pixmap.width() - 1, pixmap.height() - 1);
            painter.end();
            item->setPixmap(pixmap);
        }
    }
    selected_connection_item = connectionItem;
    setConnectionEditControlsEnabled(true);
    setConnectionEditControlValues(selected_connection_item->connection);
    ui->spinBox_ConnectionOffset->setMaximum(selected_connection_item->getMaxOffset());
    ui->spinBox_ConnectionOffset->setMinimum(selected_connection_item->getMinOffset());
    onConnectionOffsetChanged(selected_connection_item->connection->offset.toInt());
}

void Editor::setSelectedConnectionFromMap(QString mapName) {
    // Search for the first connection that connects to the given map map.
    for (ConnectionPixmapItem* item : connection_edit_items) {
        if (item->connection->map_name == mapName) {
            onConnectionItemSelected(item);
            break;
        }
    }
}

void Editor::onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem) {
    emit loadMapRequested(connectionItem->connection->map_name, map->name);
}

void Editor::onConnectionDirectionChanged(QString newDirection) {
    ui->comboBox_ConnectionDirection->blockSignals(true);
    ui->comboBox_ConnectionDirection->setCurrentText(newDirection);
    ui->comboBox_ConnectionDirection->blockSignals(false);
}

void Editor::onBorderMetatilesChanged() {
    displayMapBorder();
    setBorderItemsVisible(ui->checkBox_ToggleBorder->isChecked());
}

void Editor::onHoveredMovementPermissionChanged(uint16_t collision, uint16_t elevation) {
    this->ui->statusBar->showMessage(this->getMovementPermissionText(collision, elevation));
}

void Editor::onHoveredMovementPermissionCleared() {
    this->ui->statusBar->clearMessage();
}

void Editor::onHoveredMetatileSelectionChanged(uint16_t metatileId) {
    Metatile *metatile = Tileset::getMetatile(metatileId, map->layout->tileset_primary, map->layout->tileset_secondary);
    QString message;
    QString hexString = QString("%1").arg(metatileId, 3, 16, QChar('0')).toUpper();
    if (metatile && metatile->label.size() != 0) {
        message = QString("Metatile: 0x%1 \"%2\"").arg(hexString, metatile->label);
    } else {
        message = QString("Metatile: 0x%1").arg(hexString);
    }
    this->ui->statusBar->showMessage(message);
}

void Editor::onHoveredMetatileSelectionCleared() {
    this->ui->statusBar->clearMessage();
}

void Editor::onSelectedMetatilesChanged() {
    QPoint size = this->metatile_selector_item->getSelectionDimensions();
    this->cursorMapTileRect->updateSelectionSize(size.x(), size.y());
    this->redrawCurrentMetatilesSelection();
}

void Editor::onHoveredMapMetatileChanged(int x, int y) {
    this->playerViewRect->updateLocation(x, y);
    this->cursorMapTileRect->updateLocation(x, y);
    if (map_item->paintingMode == MapPixmapItem::PaintMode::Metatiles
     && x >= 0 && x < map->getWidth() && y >= 0 && y < map->getHeight()) {
        int blockIndex = y * map->getWidth() + x;
        int tile = map->layout->blockdata->blocks->at(blockIndex).tile;
        this->ui->statusBar->showMessage(QString("X: %1, Y: %2, Metatile: 0x%3, Scale = %4x")
                              .arg(x)
                              .arg(y)
                              .arg(QString("%1").arg(tile, 3, 16, QChar('0')).toUpper())
                              .arg(QString::number(pow(scale_base, scale_exp), 'g', 2)));
    }
}

void Editor::onHoveredMapMetatileCleared() {
    this->playerViewRect->setVisible(false);
    this->cursorMapTileRect->setVisible(false);
    if (map_item->paintingMode == MapPixmapItem::PaintMode::Metatiles) {
        this->ui->statusBar->clearMessage();
    }
}

void Editor::onHoveredMapMovementPermissionChanged(int x, int y) {
    this->playerViewRect->updateLocation(x, y);
    this->cursorMapTileRect->updateLocation(x, y);
    if (map_item->paintingMode == MapPixmapItem::PaintMode::Metatiles
     && x >= 0 && x < map->getWidth() && y >= 0 && y < map->getHeight()) {
        int blockIndex = y * map->getWidth() + x;
        uint16_t collision = map->layout->blockdata->blocks->at(blockIndex).collision;
        uint16_t elevation = map->layout->blockdata->blocks->at(blockIndex).elevation;
        QString message = QString("X: %1, Y: %2, %3")
                            .arg(x)
                            .arg(y)
                            .arg(this->getMovementPermissionText(collision, elevation));
        this->ui->statusBar->showMessage(message);
    }
}

void Editor::onHoveredMapMovementPermissionCleared() {
    this->playerViewRect->setVisible(false);
    this->cursorMapTileRect->setVisible(false);
    if (map_item->paintingMode == MapPixmapItem::PaintMode::Metatiles) {
        this->ui->statusBar->clearMessage();
    }
}

QString Editor::getMovementPermissionText(uint16_t collision, uint16_t elevation){
    QString message;
    if (collision == 0 && elevation == 0) {
        message = "Collision: Transition between elevations";
    } else if (collision == 0 && elevation == 15) {
        message = "Collision: Multi-Level (Bridge)";
    } else if (collision == 0 && elevation == 1) {
        message = "Collision: Surf";
    } else if (collision == 0) {
        message = QString("Collision: Passable, Elevation: %1").arg(elevation);
    } else {
        message = QString("Collision: Impassable, Elevation: %1").arg(elevation);
    }
    return message;
}

void Editor::setConnectionsVisibility(bool visible) {
    for (QGraphicsPixmapItem* item : connection_items) {
        item->setVisible(visible);
        item->setActive(visible);
    }
}

bool Editor::setMap(QString map_name) {
    if (map_name.isEmpty()) {
        return false;
    }

    if (project) {
        Map *loadedMap = project->loadMap(map_name);
        if (!loadedMap) {
            return false;
        }

        map = loadedMap;
        selected_events->clear();
        if (!displayMap()) {
            return false;
        }
        updateSelectedEvents();
    }

    return true;
}

void Editor::onMapStartPaint(QGraphicsSceneMouseEvent *event, MapPixmapItem *item) {
    if (!(item->paintingMode == MapPixmapItem::PaintMode::Metatiles)) {
        return;
    }

    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;
    if (event->buttons() & Qt::RightButton && (map_edit_mode == "paint" || map_edit_mode == "fill")) {
        this->cursorMapTileRect->initRightClickSelectionAnchor(x, y);
    } else {
        this->cursorMapTileRect->initAnchor(x, y);
    }
}

void Editor::onMapEndPaint(QGraphicsSceneMouseEvent *, MapPixmapItem *item) {
    if (!(item->paintingMode == MapPixmapItem::PaintMode::Metatiles)) {
        return;
    }
    this->cursorMapTileRect->stopRightClickSelectionAnchor();
    this->cursorMapTileRect->stopAnchor();
}

void Editor::setSmartPathCursorMode(QGraphicsSceneMouseEvent *event)
{
    bool smartPathsEnabled = event->modifiers() & Qt::ShiftModifier;
    if (smartPathsEnabled || settings->smartPathsEnabled) {
        this->cursorMapTileRect->setSmartPathMode();
    } else {
        this->cursorMapTileRect->setNormalPathMode();
    }
}

void Editor::mouseEvent_map(QGraphicsSceneMouseEvent *event, MapPixmapItem *item) {
    // TODO: add event tab object painting tool buttons stuff here
    if (item->paintingMode == MapPixmapItem::PaintMode::Disabled) {
        return;
    }

    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;

    if (item->paintingMode == MapPixmapItem::PaintMode::Metatiles) {
        if (map_edit_mode == "paint") {
            if (event->buttons() & Qt::RightButton) {
                item->updateMetatileSelection(event);
            } else if (event->buttons() & Qt::MiddleButton) {
                if (event->modifiers() & Qt::ControlModifier) {
                    item->magicFill(event);
                } else {
                    item->floodFill(event);
                }
            } else {
                this->setSmartPathCursorMode(event);
                item->paint(event);
            }
        } else if (map_edit_mode == "select") {
            item->select(event);
        } else if (map_edit_mode == "fill") {
            if (event->buttons() & Qt::RightButton) {
                item->updateMetatileSelection(event);
            } else if (event->modifiers() & Qt::ControlModifier) {
                item->magicFill(event);
            } else {
                item->floodFill(event);
            }
        } else if (map_edit_mode == "pick") {
            if (event->buttons() & Qt::RightButton) {
                item->updateMetatileSelection(event);
            } else {
                item->pick(event);
            }
        } else if (map_edit_mode == "shift") {
            item->shift(event);
        }
    } else if (item->paintingMode == MapPixmapItem::PaintMode::EventObjects) {
        if (map_edit_mode == "paint" && event->type() == QEvent::GraphicsSceneMousePress) {
            // Right-clicking while in paint mode will change mode to select.
            if (event->buttons() & Qt::RightButton) {
                this->map_edit_mode = "select";
                this->settings->mapCursor = QCursor();
                this->cursorMapTileRect->setSingleTileMode();
                this->ui->toolButton_Paint->setChecked(false);
                this->ui->toolButton_Select->setChecked(true);
            } else {
                // Left-clicking while in paint mode will add a new event of the
                // type of the first currently selected events.
                // Disallow adding heal locations, deleting them is not possible yet
                QString eventType = this->selected_events->first()->event->get("event_type");
                if (eventType != "event_heal_location") {
                    DraggablePixmapItem * newEvent = addNewEvent(eventType);
                    if (newEvent) {
                        newEvent->move(x, y);
                        selectMapEvent(newEvent, false);
                    }
                }
            }
        } else if (map_edit_mode == "select") {
            // do nothing here, at least for now
        } else if (map_edit_mode == "shift" && item->map) {
            static QPoint selection_origin;

            if (event->type() == QEvent::GraphicsSceneMouseRelease) {
                // TODO: commit / update history here
            } else {
                if (event->type() == QEvent::GraphicsSceneMousePress) {
                    selection_origin = QPoint(x, y);
                } else if (event->type() == QEvent::GraphicsSceneMouseMove) {
                    if (x != selection_origin.x() || y != selection_origin.y()) {
                        int xDelta = x - selection_origin.x();
                        int yDelta = y - selection_origin.y();

                        for (DraggablePixmapItem *item : *(getObjects())) {
                            item->move(xDelta, yDelta);
                        }
                        selection_origin = QPoint(x, y);
                    }
                }
            }
        }
    }
    this->playerViewRect->updateLocation(x, y);
    this->cursorMapTileRect->updateLocation(x, y);
}

void Editor::mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item) {
    if (item->paintingMode != MapPixmapItem::PaintMode::Metatiles) {
        return;
    }

    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 16;
    int y = static_cast<int>(pos.y()) / 16;
    this->playerViewRect->updateLocation(x, y);
    this->cursorMapTileRect->updateLocation(x, y);
    if (map_edit_mode == "paint") {
        if (event->buttons() & Qt::RightButton) {
            item->updateMovementPermissionSelection(event);
        } else if (event->buttons() & Qt::MiddleButton) {
            if (event->modifiers() & Qt::ControlModifier) {
                item->magicFill(event);
            } else {
                item->floodFill(event);
            }
        } else {
            item->paint(event);
        }
    } else if (map_edit_mode == "select") {
        item->select(event);
    } else if (map_edit_mode == "fill") {
        if (event->buttons() & Qt::RightButton) {
            item->pick(event);
        } else if (event->modifiers() & Qt::ControlModifier) {
            item->magicFill(event);
        } else {
            item->floodFill(event);
        }
    } else if (map_edit_mode == "pick") {
        item->pick(event);
    } else if (map_edit_mode == "shift") {
        item->shift(event);
    }
}

bool Editor::displayMap() {
    if (!scene) {
        scene = new QGraphicsScene;
        MapSceneEventFilter *filter = new MapSceneEventFilter();
        scene->installEventFilter(filter);
        connect(filter, &MapSceneEventFilter::wheelZoom, this, &Editor::wheelZoom);
    }

    if (map_item && scene) {
        scene->removeItem(map_item);
        delete map_item;
        scene->removeItem(this->playerViewRect);
        scene->removeItem(this->cursorMapTileRect);
    }

    displayMetatileSelector();
    displayMovementPermissionSelector();
    displayMapMetatiles();
    displayMapMovementPermissions();
    displayBorderMetatiles();
    displayCurrentMetatilesSelection();
    displayMapEvents();
    displayMapConnections();
    displayMapBorder();
    displayMapGrid();
    displayWildMonTables();

    this->playerViewRect->setZValue(1000);
    this->cursorMapTileRect->setZValue(1001);
    scene->addItem(this->playerViewRect);
    scene->addItem(this->cursorMapTileRect);

    if (map_item) {
        map_item->setVisible(false);
    }
    if (collision_item) {
        collision_item->setVisible(false);
    }
    if (events_group) {
        events_group->setVisible(false);
    }
    return true;
}

void Editor::displayMetatileSelector() {
    if (metatile_selector_item && metatile_selector_item->scene()) {
        metatile_selector_item->scene()->removeItem(metatile_selector_item);
        delete scene_metatiles;
    }

    scene_metatiles = new QGraphicsScene;
    if (!metatile_selector_item) {
        metatile_selector_item = new MetatileSelector(8, map->layout->tileset_primary, map->layout->tileset_secondary);
        connect(metatile_selector_item, SIGNAL(hoveredMetatileSelectionChanged(uint16_t)),
                this, SLOT(onHoveredMetatileSelectionChanged(uint16_t)));
        connect(metatile_selector_item, SIGNAL(hoveredMetatileSelectionCleared()),
                this, SLOT(onHoveredMetatileSelectionCleared()));
        connect(metatile_selector_item, SIGNAL(selectedMetatilesChanged()),
                this, SLOT(onSelectedMetatilesChanged()));
        metatile_selector_item->select(0);
    } else {
        metatile_selector_item->setTilesets(map->layout->tileset_primary, map->layout->tileset_secondary);
    }

    scene_metatiles->addItem(metatile_selector_item);
}

void Editor::displayMapMetatiles() {
    map_item = new MapPixmapItem(map, this->metatile_selector_item, this->settings);
    connect(map_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,MapPixmapItem*)),
            this, SLOT(mouseEvent_map(QGraphicsSceneMouseEvent*,MapPixmapItem*)));
    connect(map_item, SIGNAL(startPaint(QGraphicsSceneMouseEvent*,MapPixmapItem*)),
            this, SLOT(onMapStartPaint(QGraphicsSceneMouseEvent*,MapPixmapItem*)));
    connect(map_item, SIGNAL(endPaint(QGraphicsSceneMouseEvent*,MapPixmapItem*)),
            this, SLOT(onMapEndPaint(QGraphicsSceneMouseEvent*,MapPixmapItem*)));
    connect(map_item, SIGNAL(hoveredMapMetatileChanged(int, int)),
            this, SLOT(onHoveredMapMetatileChanged(int, int)));
    connect(map_item, SIGNAL(hoveredMapMetatileCleared()),
            this, SLOT(onHoveredMapMetatileCleared()));

    map_item->draw(true);
    scene->addItem(map_item);

    int tw = 16;
    int th = 16;
    scene->setSceneRect(
        -BORDER_DISTANCE * tw,
        -BORDER_DISTANCE * th,
        map_item->pixmap().width() + BORDER_DISTANCE * 2 * tw,
        map_item->pixmap().height() + BORDER_DISTANCE * 2 * th
    );
}

void Editor::displayMapMovementPermissions() {
    if (collision_item && scene) {
        scene->removeItem(collision_item);
        delete collision_item;
    }
    collision_item = new CollisionPixmapItem(map, this->movement_permissions_selector_item, this->metatile_selector_item, this->settings, &this->collisionOpacity);
    connect(collision_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)),
            this, SLOT(mouseEvent_collision(QGraphicsSceneMouseEvent*,CollisionPixmapItem*)));
    connect(collision_item, SIGNAL(hoveredMapMovementPermissionChanged(int, int)),
            this, SLOT(onHoveredMapMovementPermissionChanged(int, int)));
    connect(collision_item, SIGNAL(hoveredMapMovementPermissionCleared()),
            this, SLOT(onHoveredMapMovementPermissionCleared()));

    collision_item->draw(true);
    scene->addItem(collision_item);
}

void Editor::displayBorderMetatiles() {
    if (selected_border_metatiles_item && selected_border_metatiles_item->scene()) {
        selected_border_metatiles_item->scene()->removeItem(selected_border_metatiles_item);
        delete selected_border_metatiles_item;
    }

    scene_selected_border_metatiles = new QGraphicsScene;
    selected_border_metatiles_item = new BorderMetatilesPixmapItem(map, this->metatile_selector_item);
    selected_border_metatiles_item->draw();
    scene_selected_border_metatiles->addItem(selected_border_metatiles_item);

    connect(selected_border_metatiles_item, SIGNAL(borderMetatilesChanged()), this, SLOT(onBorderMetatilesChanged()));
}

void Editor::displayCurrentMetatilesSelection() {
    if (scene_current_metatile_selection_item && scene_current_metatile_selection_item->scene()) {
        scene_current_metatile_selection_item->scene()->removeItem(scene_current_metatile_selection_item);
        delete scene_current_metatile_selection_item;
    }

    scene_current_metatile_selection = new QGraphicsScene;
    scene_current_metatile_selection_item = new CurrentSelectedMetatilesPixmapItem(map, this->metatile_selector_item);
    scene_current_metatile_selection_item->draw();
    scene_current_metatile_selection->addItem(scene_current_metatile_selection_item);
}

void Editor::redrawCurrentMetatilesSelection() {
    if (scene_current_metatile_selection_item) {
        scene_current_metatile_selection_item->draw();
        emit currentMetatilesSelectionChanged();
    }
}

void Editor::displayMovementPermissionSelector() {
    if (movement_permissions_selector_item && movement_permissions_selector_item->scene()) {
        movement_permissions_selector_item->scene()->removeItem(movement_permissions_selector_item);
        delete scene_collision_metatiles;
    }

    scene_collision_metatiles = new QGraphicsScene;
    if (!movement_permissions_selector_item) {
        movement_permissions_selector_item = new MovementPermissionsSelector();
        connect(movement_permissions_selector_item, SIGNAL(hoveredMovementPermissionChanged(uint16_t, uint16_t)),
                this, SLOT(onHoveredMovementPermissionChanged(uint16_t, uint16_t)));
        connect(movement_permissions_selector_item, SIGNAL(hoveredMovementPermissionCleared()),
                this, SLOT(onHoveredMovementPermissionCleared()));
        movement_permissions_selector_item->select(0, 3);
    }

    scene_collision_metatiles->addItem(movement_permissions_selector_item);
}

void Editor::displayMapEvents() {
    if (events_group) {
        for (QGraphicsItem *child : events_group->childItems()) {
            events_group->removeFromGroup(child);
            delete child;
        }

        if (events_group->scene()) {
            events_group->scene()->removeItem(events_group);
        }

        delete events_group;
    }

    selected_events->clear();

    events_group = new QGraphicsItemGroup;
    scene->addItem(events_group);

    QList<Event *> events = map->getAllEvents();
    project->loadEventPixmaps(events);
    for (Event *event : events) {
        addMapEvent(event);
    }
    //objects_group->setFiltersChildEvents(false);
    events_group->setHandlesChildEvents(false);

    emit objectsChanged();
}

DraggablePixmapItem *Editor::addMapEvent(Event *event) {
    DraggablePixmapItem *object = new DraggablePixmapItem(event, this);
    event->setFrameFromMovement(project->facingDirections.value(event->get("movement_type")));
    object->updatePixmap();
    if (!event->usingSprite) {
        object->setOpacity(0.7);
    }
    events_group->addToGroup(object);
    return object;
}

void Editor::displayMapConnections() {
    for (QGraphicsPixmapItem* item : connection_items) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    connection_items.clear();

    for (ConnectionPixmapItem* item : connection_edit_items) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    selected_connection_item = nullptr;
    connection_edit_items.clear();

    for (MapConnection *connection : map->connections) {
        if (connection->direction == "dive" || connection->direction == "emerge") {
            continue;
        }
        createConnectionItem(connection, false);
    }

    if (!connection_edit_items.empty()) {
        onConnectionItemSelected(connection_edit_items.first());
    }
}

void Editor::createConnectionItem(MapConnection* connection, bool hide) {
    Map *connected_map = project->getMap(connection->map_name);
    if (!connected_map) {
        return;
    }

    QPixmap pixmap = connected_map->renderConnection(*connection, map->layout);
    int offset = connection->offset.toInt(nullptr, 0);
    int x = 0, y = 0;
    if (connection->direction == "up") {
        x = offset * 16;
        y = -pixmap.height();
    } else if (connection->direction == "down") {
        x = offset * 16;
        y = map->getHeight() * 16;
    } else if (connection->direction == "left") {
        x = -pixmap.width();
        y = offset * 16;
    } else if (connection->direction == "right") {
        x = map->getWidth() * 16;
        y = offset * 16;
    }

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
    item->setZValue(-1);
    item->setX(x);
    item->setY(y);
    scene->addItem(item);
    connection_items.append(item);
    item->setVisible(!hide);

    ConnectionPixmapItem *connection_edit_item = new ConnectionPixmapItem(pixmap, connection, x, y, map->getWidth(), map->getHeight());
    connection_edit_item->setX(x);
    connection_edit_item->setY(y);
    connection_edit_item->setZValue(-1);
    scene->addItem(connection_edit_item);
    connect(connection_edit_item, SIGNAL(connectionMoved(MapConnection*)), this, SLOT(onConnectionMoved(MapConnection*)));
    connect(connection_edit_item, SIGNAL(connectionItemSelected(ConnectionPixmapItem*)), this, SLOT(onConnectionItemSelected(ConnectionPixmapItem*)));
    connect(connection_edit_item, SIGNAL(connectionItemDoubleClicked(ConnectionPixmapItem*)), this, SLOT(onConnectionItemDoubleClicked(ConnectionPixmapItem*)));
    connection_edit_items.append(connection_edit_item);
}

void Editor::displayMapBorder() {
    for (QGraphicsPixmapItem* item : borderItems) {
        if (item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    borderItems.clear();

    int borderWidth = map->getBorderWidth();
    int borderHeight = map->getBorderHeight();
    int borderHorzDist = getBorderDrawDistance(borderWidth);
    int borderVertDist = getBorderDrawDistance(borderHeight);
    QPixmap pixmap = map->renderBorder();
    for (int y = -borderVertDist; y < map->getHeight() + borderVertDist; y += borderHeight)
    for (int x = -borderHorzDist; x < map->getWidth() + borderHorzDist; x += borderWidth) {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pixmap);
        item->setX(x * 16);
        item->setY(y * 16);
        item->setZValue(-2);
        scene->addItem(item);
        borderItems.append(item);
    }
}

void Editor::updateMapBorder() {
    QPixmap pixmap = this->map->renderBorder(true);
    for (auto item : this->borderItems) {
        item->setPixmap(pixmap);
    }
}

void Editor::updateMapConnections() {
    if (connection_items.size() != connection_edit_items.size())
        return;

    for (int i = 0; i < connection_items.size(); i++) {
        Map *connected_map = project->getMap(connection_edit_items[i]->connection->map_name);
        if (!connected_map)
            continue;

        QPixmap pixmap = connected_map->renderConnection(*(connection_edit_items[i]->connection), map->layout);
        connection_items[i]->setPixmap(pixmap);
        connection_edit_items[i]->basePixmap = pixmap;
        connection_edit_items[i]->setPixmap(pixmap);
    }
}

int Editor::getBorderDrawDistance(int dimension) {
    // Draw sufficient border blocks to fill the player's view (BORDER_DISTANCE)
    if (dimension >= BORDER_DISTANCE) {
        return dimension;
    } else if (dimension) {
        return dimension * (BORDER_DISTANCE / dimension + (BORDER_DISTANCE % dimension ? 1 : 0));
    } else {
        return BORDER_DISTANCE;
    }
}

void Editor::displayMapGrid() {
    for (QGraphicsLineItem* item : gridLines) {
        if (item && item->scene()) {
            item->scene()->removeItem(item);
        }
        delete item;
    }
    gridLines.clear();
    ui->checkBox_ToggleGrid->disconnect();

    int pixelWidth = map->getWidth() * 16;
    int pixelHeight = map->getHeight() * 16;
    for (int i = 0; i <= map->getWidth(); i++) {
        int x = i * 16;
        QGraphicsLineItem *line = scene->addLine(x, 0, x, pixelHeight);
        line->setVisible(ui->checkBox_ToggleGrid->isChecked());
        gridLines.append(line);
        connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
    for (int j = 0; j <= map->getHeight(); j++) {
        int y = j * 16;
        QGraphicsLineItem *line = scene->addLine(0, y, pixelWidth, y);
        line->setVisible(ui->checkBox_ToggleGrid->isChecked());
        gridLines.append(line);
        connect(ui->checkBox_ToggleGrid, &QCheckBox::toggled, [=](bool checked){line->setVisible(checked);});
    }
}

void Editor::updateConnectionOffset(int offset) {
    if (!selected_connection_item)
        return;

    selected_connection_item->blockSignals(true);
    offset = qMin(offset, selected_connection_item->getMaxOffset());
    offset = qMax(offset, selected_connection_item->getMinOffset());
    selected_connection_item->connection->offset = QString::number(offset);
    if (selected_connection_item->connection->direction == "up" || selected_connection_item->connection->direction == "down") {
        selected_connection_item->setX(selected_connection_item->initialX + (offset - selected_connection_item->initialOffset) * 16);
    } else if (selected_connection_item->connection->direction == "left" || selected_connection_item->connection->direction == "right") {
        selected_connection_item->setY(selected_connection_item->initialY + (offset - selected_connection_item->initialOffset) * 16);
    }
    selected_connection_item->blockSignals(false);
    updateMirroredConnectionOffset(selected_connection_item->connection);
}

void Editor::setConnectionMap(QString mapName) {
    if (!mapName.isEmpty() && !project->mapNames->contains(mapName)) {
        logError(QString("Invalid map name '%1' specified for connection.").arg(mapName));
        return;
    }
    if (!selected_connection_item)
        return;

    if (mapName.isEmpty()) {
        removeCurrentConnection();
        return;
    }

    QString originalMapName = selected_connection_item->connection->map_name;
    setConnectionEditControlsEnabled(true);
    selected_connection_item->connection->map_name = mapName;
    setCurrentConnectionDirection(selected_connection_item->connection->direction);
    updateMirroredConnectionMap(selected_connection_item->connection, originalMapName);
}

void Editor::addNewConnection() {
    // Find direction with least number of connections.
    QMap<QString, int> directionCounts = QMap<QString, int>({{"up", 0}, {"right", 0}, {"down", 0}, {"left", 0}});
    for (MapConnection* connection : map->connections) {
        directionCounts[connection->direction]++;
    }
    QString minDirection = "up";
    int minCount = INT_MAX;
    for (QString direction : directionCounts.keys()) {
        if (directionCounts[direction] < minCount) {
            minDirection = direction;
            minCount = directionCounts[direction];
        }
    }

    // Don't connect the map to itself.
    QString defaultMapName = project->mapNames->first();
    if (defaultMapName == map->name) {
        defaultMapName = project->mapNames->value(1);
    }

    MapConnection* newConnection = new MapConnection;
    newConnection->direction = minDirection;
    newConnection->offset = "0";
    newConnection->map_name = defaultMapName;
    map->connections.append(newConnection);
    createConnectionItem(newConnection, true);
    onConnectionItemSelected(connection_edit_items.last());
    ui->label_NumConnections->setText(QString::number(map->connections.length()));

    updateMirroredConnection(newConnection, newConnection->direction, newConnection->map_name);
}

void Editor::updateMirroredConnectionOffset(MapConnection* connection) {
    updateMirroredConnection(connection, connection->direction, connection->map_name);
}
void Editor::updateMirroredConnectionDirection(MapConnection* connection, QString originalDirection) {
    updateMirroredConnection(connection, originalDirection, connection->map_name);
}
void Editor::updateMirroredConnectionMap(MapConnection* connection, QString originalMapName) {
    updateMirroredConnection(connection, connection->direction, originalMapName);
}
void Editor::removeMirroredConnection(MapConnection* connection) {
    updateMirroredConnection(connection, connection->direction, connection->map_name, true);
}
void Editor::updateMirroredConnection(MapConnection* connection, QString originalDirection, QString originalMapName, bool isDelete) {
    if (!ui->checkBox_MirrorConnections->isChecked())
        return;
    Map* otherMap = project->getMap(originalMapName);
    if (!otherMap)
        return;

    static QMap<QString, QString> oppositeDirections = QMap<QString, QString>({
        {"up", "down"}, {"right", "left"},
        {"down", "up"}, {"left", "right"},
        {"dive", "emerge"},{"emerge", "dive"}});
    QString oppositeDirection = oppositeDirections.value(originalDirection);

    // Find the matching connection in the connected map.
    MapConnection* mirrorConnection = nullptr;
    for (MapConnection* conn : otherMap->connections) {
        if (conn->direction == oppositeDirection && conn->map_name == map->name) {
            mirrorConnection = conn;
        }
    }

    if (isDelete) {
        if (mirrorConnection) {
            otherMap->connections.removeOne(mirrorConnection);
            delete mirrorConnection;
        }
        return;
    }

    if (connection->direction != originalDirection || connection->map_name != originalMapName) {
        if (mirrorConnection) {
            otherMap->connections.removeOne(mirrorConnection);
            delete mirrorConnection;
            mirrorConnection = nullptr;
            otherMap = project->getMap(connection->map_name);
        }
    }

    // Create a new mirrored connection, if a matching one doesn't already exist.
    if (!mirrorConnection) {
        mirrorConnection = new MapConnection;
        mirrorConnection->direction = oppositeDirections.value(connection->direction);
        mirrorConnection->map_name = map->name;
        otherMap->connections.append(mirrorConnection);
    }

    mirrorConnection->offset = QString::number(-connection->offset.toInt());
}

void Editor::removeCurrentConnection() {
    if (!selected_connection_item)
        return;

    map->connections.removeOne(selected_connection_item->connection);
    connection_edit_items.removeOne(selected_connection_item);
    removeMirroredConnection(selected_connection_item->connection);

    if (selected_connection_item && selected_connection_item->scene()) {
        selected_connection_item->scene()->removeItem(selected_connection_item);
        delete selected_connection_item;
    }

    selected_connection_item = nullptr;
    setConnectionEditControlsEnabled(false);
    ui->spinBox_ConnectionOffset->setValue(0);
    ui->label_NumConnections->setText(QString::number(map->connections.length()));

    if (connection_edit_items.length() > 0) {
        onConnectionItemSelected(connection_edit_items.last());
    }
}

void Editor::updateDiveMap(QString mapName) {
    updateDiveEmergeMap(mapName, "dive");
}

void Editor::updateEmergeMap(QString mapName) {
    updateDiveEmergeMap(mapName, "emerge");
}

void Editor::updateDiveEmergeMap(QString mapName, QString direction) {
    if (!mapName.isEmpty() && !project->mapNamesToMapConstants->contains(mapName)) {
        logError(QString("Invalid %1 connection map name: '%2'").arg(direction).arg(mapName));
        return;
    }

    MapConnection* connection = nullptr;
    for (MapConnection* conn : map->connections) {
        if (conn->direction == direction) {
            connection = conn;
            break;
        }
    }

    if (mapName.isEmpty()) {
        // Remove dive/emerge connection
        if (connection) {
            map->connections.removeOne(connection);
            removeMirroredConnection(connection);
        }
    } else {
        if (!connection) {
            connection = new MapConnection;
            connection->direction = direction;
            connection->offset = "0";
            connection->map_name = mapName;
            map->connections.append(connection);
            updateMirroredConnection(connection, connection->direction, connection->map_name);
        } else {
            QString originalMapName = connection->map_name;
            connection->map_name = mapName;
            updateMirroredConnectionMap(connection, originalMapName);
        }
    }

    ui->label_NumConnections->setText(QString::number(map->connections.length()));
}

void Editor::updatePrimaryTileset(QString tilesetLabel, bool forceLoad)
{
    if (map->layout->tileset_primary_label != tilesetLabel || forceLoad)
    {
        map->layout->tileset_primary_label = tilesetLabel;
        map->layout->tileset_primary = project->getTileset(tilesetLabel, forceLoad);
        emit tilesetChanged(map->name);
    }
}

void Editor::updateSecondaryTileset(QString tilesetLabel, bool forceLoad)
{
    if (map->layout->tileset_secondary_label != tilesetLabel || forceLoad)
    {
        map->layout->tileset_secondary_label = tilesetLabel;
        map->layout->tileset_secondary = project->getTileset(tilesetLabel, forceLoad);
        emit tilesetChanged(map->name);
    }
}

void Editor::toggleBorderVisibility(bool visible)
{
    this->setBorderItemsVisible(visible);
    this->setConnectionsVisibility(visible);
}

void Editor::updateCustomMapHeaderValues(QTableWidget *table)
{
    QMap<QString, QString> fields;
    for (int row = 0; row < table->rowCount(); row++) {
        QString keyStr = "";
        QString valueStr = "";
        QTableWidgetItem *key = table->item(row, 0);
        QTableWidgetItem *value = table->item(row, 1);
        if (key) keyStr = key->text();
        if (value) valueStr = value->text();
        fields[keyStr] = valueStr;
    }
    map->customHeaders = fields;
}

Tileset* Editor::getCurrentMapPrimaryTileset()
{
    QString tilesetLabel = map->layout->tileset_primary_label;
    return project->getTileset(tilesetLabel);
}

void DraggablePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *mouse) {
    active = true;
    last_x = static_cast<int>(mouse->pos().x() + this->pos().x()) / 16;
    last_y = static_cast<int>(mouse->pos().y() + this->pos().y()) / 16;
    this->editor->selectMapEvent(this, mouse->modifiers() & Qt::ControlModifier);
    //this->editor->updateSelectedEvents();
    selectingEvent = true;
}

void DraggablePixmapItem::move(int x, int y) {
    event->setX(event->x() + x);
    event->setY(event->y() + y);
    updatePosition();
    emitPositionChanged();
}

void DraggablePixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *mouse) {
    if (active) {
        int x = static_cast<int>(mouse->pos().x() + this->pos().x()) / 16;
        int y = static_cast<int>(mouse->pos().y() + this->pos().y()) / 16;
        this->editor->playerViewRect->updateLocation(x, y);
        this->editor->cursorMapTileRect->updateLocation(x, y);
        if (x != last_x || y != last_y) {
            if (editor->selected_events->contains(this)) {
                for (DraggablePixmapItem *item : *editor->selected_events) {
                    item->move(x - last_x, y - last_y);
                }
            } else {
                move(x - last_x, y - last_y);
            }
            last_x = x;
            last_y = y;
        }
    }
}

void DraggablePixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *) {
    active = false;
}

void DraggablePixmapItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *) {
    if (this->event->get("event_type") == EventType::Warp) {
        QString destMap = this->event->get("destination_map_name");
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(this->event->get("destination_map_name"), this->event->get("destination_warp"));
        }
    }
    else if (this->event->get("event_type") == EventType::SecretBase) {
        QString baseId = this->event->get("secret_base_id");
        QString destMap = editor->project->mapConstantsToMapNames->value("MAP_" + baseId.left(baseId.lastIndexOf("_")));
        if (destMap != NONE_MAP_NAME) {
            emit editor->warpEventDoubleClicked(destMap, "0");
        }
    }
}

QList<DraggablePixmapItem *> *Editor::getObjects() {
    QList<DraggablePixmapItem *> *list = new QList<DraggablePixmapItem *>;
    for (Event *event : map->getAllEvents()) {
        for (QGraphicsItem *child : events_group->childItems()) {
            DraggablePixmapItem *item = static_cast<DraggablePixmapItem *>(child);
            if (item->event == event) {
                list->append(item);
                break;
            }
        }
    }
    return list;
}

void Editor::redrawObject(DraggablePixmapItem *item) {
    if (item) {
        item->setPixmap(item->event->pixmap.copy(item->event->frame * item->event->spriteWidth % item->event->pixmap.width(), 0, item->event->spriteWidth, item->event->spriteHeight));
        item->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
        if (selected_events && selected_events->contains(item)) {
            QImage image = item->pixmap().toImage();
            QPainter painter(&image);
            painter.setPen(QColor(250, 0, 255));
            painter.drawRect(0, 0, image.width() - 1, image.height() - 1);
            painter.end();
            item->setPixmap(QPixmap::fromImage(image));
        }
        item->updatePosition();
    }
}

void Editor::updateSelectedEvents() {
    for (DraggablePixmapItem *item : *(getObjects())) {
        redrawObject(item);
    }
    emit selectedObjectsChanged();
}

void Editor::selectMapEvent(DraggablePixmapItem *object) {
    selectMapEvent(object, false);
}

void Editor::selectMapEvent(DraggablePixmapItem *object, bool toggle) {
    if (selected_events && object) {
        if (selected_events->contains(object)) {
            if (toggle) {
                selected_events->removeOne(object);
            }
        } else {
            if (!toggle) {
                selected_events->clear();
            }
            selected_events->append(object);
        }
        updateSelectedEvents();
    }
}

DraggablePixmapItem* Editor::addNewEvent(QString event_type) {
    if (project && map && !event_type.isEmpty()) {
        Event *event = Event::createNewEvent(event_type, map->name, project);
        event->put("map_name", map->name);
        if (event_type == "event_heal_location") {
            HealLocation hl = HealLocation::fromEvent(event);
            project->healLocations.append(hl);
            event->put("index", project->healLocations.length());
        }
        map->addEvent(event);
        project->loadEventPixmaps(map->getAllEvents());
        DraggablePixmapItem *object = addMapEvent(event);
        return object;
    }
    return nullptr;
}

void Editor::deleteEvent(Event *event) {
    Map *map = project->getMap(event->get("map_name"));
    if (map) {
        map->removeEvent(event);
    }
    //selected_events->removeAll(event);
    //updateSelectedObjects();
}

// It doesn't seem to be possible to prevent the mousePress event
// from triggering both event's DraggablePixmapItem and the background mousePress.
// Since the DraggablePixmapItem's event fires first, we can set a temp
// variable "selectingEvent" so that we can detect whether or not the user
// is clicking on the background instead of an event.
void Editor::objectsView_onMousePress(QMouseEvent *event) {
    // make sure we are in object editing mode
    if (map_item && map_item->paintingMode != MapPixmapItem::PaintMode::EventObjects) {
        return;
    }
    if (this->map_edit_mode == "paint" && event->buttons() & Qt::RightButton) {
        this->map_edit_mode = "select";
        this->settings->mapCursor = QCursor();
        this->cursorMapTileRect->setSingleTileMode();
        this->ui->toolButton_Paint->setChecked(false);
        this->ui->toolButton_Select->setChecked(true);
    }
    bool multiSelect = event->modifiers() & Qt::ControlModifier;
    if (!selectingEvent && !multiSelect && selected_events->length() > 1) {
        DraggablePixmapItem *first = selected_events->first();
        selected_events->clear();
        selected_events->append(first);
        updateSelectedEvents();
    }
    selectingEvent = false;
}
