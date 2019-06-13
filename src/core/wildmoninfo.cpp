//

#include "wildmoninfo.h"
#include "project.h"



// TODO: remove this necessity
static QMap<int, QString> landPercentages = QMap<int, QString>({
    {1, "20"}, {2, "20"},
    {3, "10"}, {4, "10"}, {5, "10"}, {6, "10"},
    {7, "5"}, {8, "5"},
    {9, "4"}, {10, "4"},
    {11, "1"}, {12, "1"}
});

void clearTabWidget(QLayout *tab) {
    QLayoutItem *item = tab->itemAt(0);
    if (item) tab->removeItem(item);
}

void clearTable(QTableWidget *table) {
    //
    if (table) table->clear();
}

void createSpeciesTableRow(Project *project, QTableWidget *table, WildPokemon mon, int index) {
    //
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
    minLevel->setMinimum(1);
    minLevel->setMaximum(100);
    maxLevel->setMinimum(1);
    maxLevel->setMaximum(100);
    minLevel->setValue(mon.minLevel);
    maxLevel->setValue(mon.maxLevel);

    // percentage -- add to json settings
    QLabel *percentLabel = new QLabel(landPercentages[index]);

    QFrame *speciesSelector = new QFrame;
    QHBoxLayout *speciesSelectorLayout = new QHBoxLayout;
    speciesSelectorLayout->addWidget(monLabel);
    speciesSelectorLayout->addWidget(monSelector);
    speciesSelector->setLayout(speciesSelectorLayout);

    // prevent the spinboxes from being stupidly tall
    QFrame *minLevelFrame = new QFrame;
    QVBoxLayout *minLevelSpinboxLayout = new QVBoxLayout;
    minLevelSpinboxLayout->addWidget(minLevel);
    minLevelFrame->setLayout(minLevelSpinboxLayout);
    QFrame *maxLevelFrame = new QFrame;
    QVBoxLayout *maxLevelSpinboxLayout = new QVBoxLayout;
    maxLevelSpinboxLayout->addWidget(maxLevel);
    maxLevelFrame->setLayout(maxLevelSpinboxLayout);

    // add widgets to the table
    table->setCellWidget(index - 1, 0, monNum);
    table->setCellWidget(index - 1, 1, speciesSelector);
    table->setCellWidget(index - 1, 2, minLevelFrame);
    table->setCellWidget(index - 1, 3, maxLevelFrame);
    table->setCellWidget(index - 1, 4, percentLabel);
}

void populateWildMonTabWidget(QTabWidget *tabWidget, QVector<QString> fields) {
    QPushButton *newTabButton = new QPushButton("Configure JSON...");
    QObject::connect(newTabButton, &QPushButton::clicked, [=](){
        // TODO
        qDebug() << "configure json pressed";
    });
    tabWidget->setCornerWidget(newTabButton);

    for (QString field : fields) {
        QTableWidget *table = new QTableWidget;
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setFocusPolicy(Qt::NoFocus);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        tabWidget->addTab(table, field);
    }
}
