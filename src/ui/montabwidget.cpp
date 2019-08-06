#include "montabwidget.h"
#include "project.h"

MonTabWidget::MonTabWidget(Project *project, QWidget *parent) : QTabWidget(parent) {
    this->project = project;
    populate();
    installEventFilter(this);
}

bool MonTabWidget::eventFilter(QObject *object, QEvent *event) {
    // Press right mouse button to activate tab.
    if (event->type() == QEvent::MouseButtonPress
     && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
        QPoint eventPos = static_cast<QMouseEvent *>(event)->pos();
        int tabIndex = tabBar()->tabAt(eventPos);
        if (tabIndex > -1) {
            askActivateTab(tabIndex, eventPos);
        }
    }
}

void MonTabWidget::populate() {
    Fields fields = project->wildMonFields;
    activeTabs = QVector<bool>(fields.size(), false);

    for (QPair<QString, QVector<int>> field : fields) {
        QTableWidget *table = new QTableWidget;
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setFocusPolicy(Qt::NoFocus);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->clearFocus();
        addTab(table, field.first);
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

    speciesTable->setRowCount(monInfo.wildPokemon.size());
    speciesTable->setColumnCount(6);

    QStringList landMonTableHeaders;
    landMonTableHeaders << "Slot" << "Species" << "Min Level" << "Max Level" << "Slot Ratio" << "Encounter Rate";
    speciesTable->setHorizontalHeaderLabels(landMonTableHeaders);
    speciesTable->horizontalHeader()->show();
    speciesTable->verticalHeader()->hide();
    speciesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    speciesTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    speciesTable->setShowGrid(false);

    QFrame *encounterFrame = new QFrame;
    QHBoxLayout *encounterLayout = new QHBoxLayout;

    QSlider *encounterRate = new QSlider(Qt::Horizontal);
    encounterRate->setMinimum(0);
    encounterRate->setMaximum(100);

    QLabel *encounterLabel = new QLabel;
    connect(encounterRate, &QSlider::valueChanged, [=](int value){
        encounterLabel->setText(QString("%1%").arg(QString::number(value)));
    });
    encounterRate->setValue(monInfo.encounterRate);
    // for some reason the signal is not being emitted above
    encounterLabel->setText(QString("%1%").arg(QString::number(monInfo.encounterRate)));

    encounterLayout->addWidget(encounterLabel);
    encounterLayout->addWidget(encounterRate);

    encounterFrame->setLayout(encounterLayout);

    speciesTable->setCellWidget(0, 5, encounterFrame);

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

    QComboBox *monSelector = new QComboBox;
    monSelector->setMinimumContentsLength(20);
    monSelector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
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
    for (auto field : project->wildMonFields) {
        if (field.first == fieldName) break;
        fieldIndex++;
    }
    QLabel *percentLabel = new QLabel(QString("%1").arg(
        QString::number(project->wildMonFields[fieldIndex].second[index]
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

    table->setCellWidget(index, 0, monNum);
    table->setCellWidget(index, 1, speciesSelector);
    table->setCellWidget(index, 2, minLevelFrame);
    table->setCellWidget(index, 3, maxLevelFrame);
    table->setCellWidget(index, 4, percentLabel);
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
