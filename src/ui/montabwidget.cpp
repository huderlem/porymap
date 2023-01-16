#include "montabwidget.h"
#include "noscrollcombobox.h"
#include "editor.h"

MonTabWidget::MonTabWidget(Editor *editor, QWidget *parent) : QTabWidget(parent) {
    this->editor = editor;
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
    auto fields = editor->project->encounterFieldTypes;
    activeTabs = QVector<bool>(fields.size(), false);

    for (QString field : fields) {
        QTableWidget *table = new QTableWidget(this);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setFocusPolicy(Qt::NoFocus);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setTabKeyNavigation(false);
        table->clearFocus();
        addTab(table, field);
    }
}

void MonTabWidget::askActivateTab(int tabIndex, QPoint menuPos) {
    if (activeTabs[tabIndex]) return;

    QMenu contextMenu(this);

    QString tabText = tabBar()->tabText(tabIndex);
    QAction actionActivateTab(QString("Add %1 data for this map...").arg(tabText), this);
    connect(&actionActivateTab, &QAction::triggered, [=](){
        clearTableAt(tabIndex);
        auto monInfo = getDefaultMonInfo();
        populateTab(tabIndex, monInfo, tabText);
        editor->saveEncounterTabData();
        setCurrentIndex(tabIndex);
        emit editor->wildMonDataChanged();
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

static bool wildPokemonSameBesidesEncounterRate(WildPokemon &a, WildPokemon &b) {
    return a.species == b.species && a.minLevel == b.minLevel && a.maxLevel == b.maxLevel;
}

static QVector<MonTableEntry> getPokemonEntries(QVector<QVector<WildPokemon>> &wildPokemon) {
    // For each wild pokemon, create a MonTableEntry with the encounter rate for each time of day.
    // If the same pokemon appears in multiple slots, add the encounter rate to the existing entry.
    QVector<MonTableEntry> pokemonEntries;
    for (int time = 0; time < wildPokemon.size(); time++) {
        for (WildPokemon mon : wildPokemon[time]) {
            bool found = false;
            for (MonTableEntry &entry : pokemonEntries) {
                if (wildPokemonSameBesidesEncounterRate(mon, entry)) {
                    entry.encounterRateByTime[time] += mon.encounterRate;
                    found = true;
                    break;
                }
            }
            if (!found) {
                MonTableEntry entry(mon);
                entry.encounterRateByTime[time] = mon.encounterRate;
                pokemonEntries.append(entry);
            }
        }
    }
    return pokemonEntries;
}

void MonTabWidget::populateTab(int tabIndex, WildMonInfo &monInfo, QString fieldName) {
    QTableWidget *speciesTable = tableAt(tabIndex);
    auto wildPokemon = getPokemonEntries(monInfo.wildPokemon);

    int fieldIndex = 0;
    for (QString field : editor->project->encounterFieldTypes) {
        if (field == fieldName) break;
        fieldIndex++;
    }

    // Add empty rows to the table up to 15 rows.
    while (wildPokemon.size() < 15) {
        wildPokemon.append(WildPokemon());
    }

    speciesTable->setRowCount(wildPokemon.size());
    speciesTable->setColumnCount(5 + editor->project->timesOfDay.size());

    QStringList landMonTableHeaders;
    landMonTableHeaders << "Slot";
    landMonTableHeaders << "Species" << "Min Level" << "Max Level"; 
    for (QString time : editor->project->timesOfDay) {
        // Add header for Encounter Chance ([time]])
        landMonTableHeaders << QString("Encounter Chance - %1").arg(time);
    }
    landMonTableHeaders << "Map Encounter Rate";
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
    connect(encounterRate, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) {
        editor->saveEncounterTabData();
        emit editor->wildMonDataChanged();
    });
    encounterLayout->addWidget(encounterRate);
    encounterFrame->setLayout(encounterLayout);
    speciesTable->setCellWidget(0, speciesTable->columnCount() - 1, encounterFrame);

    for(int i = 0; i < wildPokemon.size(); i++) {
        createSpeciesTableRow(speciesTable, wildPokemon[i], i, fieldName);
    }
    this->setTabActive(tabIndex, true);
}

void MonTabWidget::createSpeciesTableRow(QTableWidget *table, MonTableEntry mon, int index, QString fieldName) {
    QPixmap monIcon = QPixmap(editor->project->speciesToIconPath.value(mon.species)).copy(0, 0, 32, 32);

    QLabel *monNum = new QLabel(QString("%1.").arg(QString::number(index)));

    QLabel *monLabel = new QLabel();
    monLabel->setPixmap(monIcon);

    NoScrollComboBox *monSelector = new NoScrollComboBox;
    monSelector->addItems(editor->project->speciesToIconPath.keys());
    monSelector->setCurrentText(mon.species);
    monSelector->setEditable(true);

    QObject::connect(monSelector, &QComboBox::currentTextChanged, [=](QString newSpecies) {
        QPixmap monIcon = QPixmap(editor->project->speciesToIconPath.value(newSpecies)).copy(0, 0, 32, 32);
        monLabel->setPixmap(monIcon);
        emit editor->wildMonDataChanged();
        if (!monIcon.isNull()) editor->saveEncounterTabData();
    });

    QSpinBox *minLevel = new QSpinBox;
    QSpinBox *maxLevel = new QSpinBox;
    minLevel->setMinimum(editor->project->miscConstants.value("min_level_define").toInt());
    minLevel->setMaximum(editor->project->miscConstants.value("max_level_define").toInt());
    maxLevel->setMinimum(editor->project->miscConstants.value("min_level_define").toInt());
    maxLevel->setMaximum(editor->project->miscConstants.value("max_level_define").toInt());
    minLevel->setValue(mon.minLevel);
    maxLevel->setValue(mon.maxLevel);

    // Connect level spinboxes so max is never less than min.
    connect(minLevel, QOverload<int>::of(&QSpinBox::valueChanged), [maxLevel, this](int min) {
        maxLevel->setMinimum(min);
        editor->saveEncounterTabData();
        emit editor->wildMonDataChanged();
    });

    connect(maxLevel, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) {
        editor->saveEncounterTabData();
        emit editor->wildMonDataChanged();
    });

    int fieldIndex = 0;
    for (QString field : editor->project->encounterFieldTypes) {
        if (field == fieldName) break;
        fieldIndex++;
    }

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
    

    table->setCellWidget(index, 0, monNum);
    table->setCellWidget(index, 1, speciesSelector);
    table->setCellWidget(index, 2, minLevelFrame);
    table->setCellWidget(index, 3, maxLevelFrame);

    int column = 4;
    for (auto rate : mon.encounterRateByTime) {
        QDoubleSpinBox *encounterRate = new QDoubleSpinBox;
        encounterRate->setMinimum(0.0);
        encounterRate->setMaximum(100.0);
        encounterRate->setDecimals(2);
        encounterRate->setSingleStep(0.1);
        // Convert from 0-256 to 0-100
        encounterRate->setValue(rate / 2.56);

        connect(encounterRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](int) {
            editor->saveEncounterTabData();
            emit editor->wildMonDataChanged();
        });

        QFrame *encounterRateFrame = new QFrame;
        QVBoxLayout *encounterRateSpinboxLayout = new QVBoxLayout;
        encounterRateSpinboxLayout->addWidget(encounterRate);
        encounterRateFrame->setLayout(encounterRateSpinboxLayout);
        table->setCellWidget(index, column++, encounterRateFrame);
    }
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
