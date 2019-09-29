#include "montabwidget.h"
#include "noscrollcombobox.h"
#include "project.h"

MonTabWidget::MonTabWidget(Project *project, QWidget *parent) : QTabWidget(parent) {
    this->project = project;
    populate();
    installEventFilter(this);
}

bool MonTabWidget::eventFilter(QObject *, QEvent *event) {
    // Press right mouse button to activate tab.
    if (event->type() == QEvent::MouseButtonPress
     && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
        QPoint eventPos = static_cast<QMouseEvent *>(event)->pos();
        int tabIndex = tabBar()->tabAt(eventPos);
        if (tabIndex > -1) {
            askActivateTab(tabIndex, eventPos);
        }
    }
    return false;
}

void MonTabWidget::populate() {
    EncounterFields fields = project->wildMonFields;
    activeTabs = QVector<bool>(fields.size(), false);

    for (EncounterField field : fields) {
        QTableWidget *table = new QTableWidget;
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setFocusPolicy(Qt::NoFocus);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->clearFocus();
        addTab(table, field.name);
    }
}

void MonTabWidget::askActivateTab(int tabIndex, QPoint menuPos) {
    if (activeTabs[tabIndex]) return;

    QMenu contextMenu(this);

    QString tabText = tabBar()->tabText(tabIndex);
    QAction actionActivateTab(QString("Add %1 data for this map...").arg(tabText), this);
    connect(&actionActivateTab, &QAction::triggered, [=](){
        clearTableAt(tabIndex);
        populateTab(tabIndex, getDefaultMonInfo(project->wildMonFields.at(tabIndex)), tabText);
        setCurrentIndex(tabIndex);
    });
    contextMenu.addAction(&actionActivateTab);
    contextMenu.exec(mapToGlobal(menuPos));
}

void MonTabWidget::clearTableAt(int tabIndex) {
    QTableWidget *table = tableAt(tabIndex);
    if (table) {
        table->clear();
        table->horizontalHeader()->hide();
    }
}

void MonTabWidget::populateTab(int tabIndex, WildMonInfo monInfo, QString fieldName) {
    QTableWidget *speciesTable = tableAt(tabIndex);

    int fieldIndex = 0;
    for (EncounterField field : project->wildMonFields) {
        if (field.name == fieldName) break;
        fieldIndex++;
    }
    bool insertGroupLabel = false;
    if (!project->wildMonFields[fieldIndex].groups.isEmpty()) insertGroupLabel = true;

    speciesTable->setRowCount(monInfo.wildPokemon.size());
    speciesTable->setColumnCount(insertGroupLabel ? 8 : 7);

    QStringList landMonTableHeaders;
    landMonTableHeaders << "Slot";
    if (insertGroupLabel) landMonTableHeaders << "Group";
    landMonTableHeaders << "Species" << "Min Level" << "Max Level" 
                        << "Encounter Chance" << "Slot Ratio" << "Encounter Rate";
    speciesTable->setHorizontalHeaderLabels(landMonTableHeaders);
    speciesTable->horizontalHeader()->show();
    speciesTable->verticalHeader()->hide();
    speciesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    speciesTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    speciesTable->setShowGrid(false);

    QFrame *encounterFrame = new QFrame;
    QHBoxLayout *encounterLayout = new QHBoxLayout;

    QSpinBox *encounterRate = new QSpinBox;
    encounterRate->setMinimum(0);
    encounterRate->setMaximum(180);
    encounterRate->setValue(monInfo.encounterRate);
    encounterLayout->addWidget(encounterRate);
    encounterFrame->setLayout(encounterLayout);
    speciesTable->setCellWidget(0, insertGroupLabel? 7 : 6, encounterFrame);

    int i = 0;
    for (WildPokemon mon : monInfo.wildPokemon) {
        createSpeciesTableRow(speciesTable, mon, i++, fieldName);
    }
    this->setTabActive(tabIndex, true);
}

void MonTabWidget::createSpeciesTableRow(QTableWidget *table, WildPokemon mon, int index, QString fieldName) {
    QPixmap monIcon = QPixmap(project->speciesToIconPath.value(mon.species)).copy(0, 0, 32, 32);

    QLabel *monNum = new QLabel(QString("%1.").arg(QString::number(index)));

    QLabel *monLabel = new QLabel();
    monLabel->setPixmap(monIcon);

    NoScrollComboBox *monSelector = new NoScrollComboBox;
    monSelector->addItems(project->speciesToIconPath.keys());
    monSelector->setCurrentText(mon.species);
    monSelector->setEditable(true);

    QObject::connect(monSelector, &QComboBox::currentTextChanged, [=](QString newSpecies){
        QPixmap monIcon = QPixmap(project->speciesToIconPath.value(newSpecies)).copy(0, 0, 32, 32);
        monLabel->setPixmap(monIcon);
    });

    QSpinBox *minLevel = new QSpinBox;
    QSpinBox *maxLevel = new QSpinBox;
    minLevel->setMinimum(project->miscConstants.value("min_level_define").toInt());
    minLevel->setMaximum(project->miscConstants.value("max_level_define").toInt());
    maxLevel->setMinimum(project->miscConstants.value("min_level_define").toInt());
    maxLevel->setMaximum(project->miscConstants.value("max_level_define").toInt());
    minLevel->setValue(mon.minLevel);
    maxLevel->setValue(mon.maxLevel);

    // Connect level spinboxes so max is never less than min.
    connect(minLevel, QOverload<int>::of(&QSpinBox::valueChanged), [maxLevel](int min){
        maxLevel->setMinimum(min);
    });

    int fieldIndex = 0;
    for (EncounterField field : project->wildMonFields) {
        if (field.name == fieldName) break;
        fieldIndex++;
    }

    double slotChanceTotal = 0.0;
    if (!project->wildMonFields[fieldIndex].groups.isEmpty()) {
        for (QString groupKey : project->wildMonFields[fieldIndex].groups.keys()) {
            if (project->wildMonFields[fieldIndex].groups[groupKey].contains(index)) {
                for (int chanceIndex : project->wildMonFields[fieldIndex].groups[groupKey]) {
                    slotChanceTotal += static_cast<double>(project->wildMonFields[fieldIndex].encounterRates[chanceIndex]);
                }
                break;
            }
        }
    } else {
        for (auto chance : project->wildMonFields[fieldIndex].encounterRates) {
            slotChanceTotal += static_cast<double>(chance);
        }
    }
    
    QLabel *percentLabel = new QLabel(QString("%1%").arg(
        QString::number(project->wildMonFields[fieldIndex].encounterRates[index] / slotChanceTotal * 100.0, 'f', 2)
    ));
    QLabel *ratioLabel = new QLabel(QString("%1").arg(
        QString::number(project->wildMonFields[fieldIndex].encounterRates[index]
    )));

    QFrame *speciesSelector = new QFrame;
    QHBoxLayout *speciesSelectorLayout = new QHBoxLayout;
    speciesSelectorLayout->addWidget(monLabel);
    speciesSelectorLayout->addWidget(monSelector);
    speciesSelector->setLayout(speciesSelectorLayout);

    // Prevent the spinboxes from being stupidly tall.
    QFrame *minLevelFrame = new QFrame;
    QVBoxLayout *minLevelSpinboxLayout = new QVBoxLayout;
    minLevelSpinboxLayout->addWidget(minLevel);
    minLevelFrame->setLayout(minLevelSpinboxLayout);
    QFrame *maxLevelFrame = new QFrame;
    QVBoxLayout *maxLevelSpinboxLayout = new QVBoxLayout;
    maxLevelSpinboxLayout->addWidget(maxLevel);
    maxLevelFrame->setLayout(maxLevelSpinboxLayout);

    bool insertGroupLabel = false;
    if (!project->wildMonFields[fieldIndex].groups.isEmpty()) insertGroupLabel = true;
    table->setCellWidget(index, 0, monNum);
    if (insertGroupLabel) {
        QString groupName = QString();
        for (QString groupKey : project->wildMonFields[fieldIndex].groups.keys()) {
            if (project->wildMonFields[fieldIndex].groups[groupKey].contains(index)) {
                groupName = groupKey;
                break;
            }
        }
        QLabel *groupNameLabel = new QLabel(groupName);
        table->setCellWidget(index, 1, groupNameLabel);
    }
    table->setCellWidget(index, insertGroupLabel? 2 : 1, speciesSelector);
    table->setCellWidget(index, insertGroupLabel? 3 : 2, minLevelFrame);
    table->setCellWidget(index, insertGroupLabel? 4 : 3, maxLevelFrame);
    table->setCellWidget(index, insertGroupLabel? 5 : 4, percentLabel);
    table->setCellWidget(index, insertGroupLabel? 6 : 5, ratioLabel);
}

QTableWidget *MonTabWidget::tableAt(int tabIndex) {
    return static_cast<QTableWidget *>(this->widget(tabIndex));
}

void MonTabWidget::setTabActive(int index, bool active) {
    activeTabs[index] = active;
    setTabEnabled(index, active);
    if (!active) {
        setTabToolTip(index, "Right-click an inactive tab to add new fields.");
    } else {
        setTabToolTip(index, QString());
    }
}
