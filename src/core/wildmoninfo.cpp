//

#include "wildmoninfo.h"
#include "project.h"



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

void createSpeciesTableRow(Project *project, QTableWidget *table, WildPokemon mon, int index) {
    //
    

    //
    //QHBoxLayout *speciesHBox = new QHBoxLayout;
    //QTableWidgetItem *monItem = new QTableWidgetItem();
            
    QPixmap monIcon = QPixmap(project->speciesToIconPath.value(mon.species)).copy(0, 0, 32, 32);

    QLabel *monNum = new QLabel(QString("%1.").arg(QString::number(index)));

    QLabel *monLabel = new QLabel();
    monLabel->setPixmap(monIcon);

    QComboBox *monSelector = new QComboBox;
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

    table->setCellWidget(index - 1, 0, monNum);
    table->setCellWidget(index - 1, 1, speciesSelector);
    //table->setCellWidget(index, 1, monLabel);
    //table->setCellWidget(index, 2, monSelector);
    table->setCellWidget(index - 1, 2, minLevel);
    table->setCellWidget(index - 1, 3, maxLevel);
    table->setCellWidget(index - 1, 4, percentLabel);
}


//
QWidget *newSpeciesTableEntry(Project *project, WildPokemon mon, int index) {

    QMap<int, QString> landPercentages = QMap<int, QString>({
        {1, "20"}, {2, "20"},
        {3, "10"}, {4, "10"}, {5, "10"}, {6, "10"},
        {7, "5"}, {8, "5"},
        {9, "4"}, {10, "4"},
        {11, "1"}, {12, "1"}
    });

    //
    QHBoxLayout *speciesHBox = new QHBoxLayout;
    QTableWidgetItem *monItem = new QTableWidgetItem();
            
    QPixmap monIcon = QPixmap(project->speciesToIconPath.value(mon.species)).copy(0, 0, 32, 32);

    QLabel *monNum = new QLabel(QString("%1.").arg(QString::number(index)));

    QLabel *monLabel = new QLabel();
    monLabel->setPixmap(monIcon);

    QComboBox *monSelector = new QComboBox;
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

    // percentage
    QLabel *percentLabel = new QLabel(landPercentages[index]);

    speciesHBox->addWidget(monNum);
    speciesHBox->addWidget(monLabel);
    speciesHBox->addWidget(monSelector);
    speciesHBox->addWidget(minLevel);
    speciesHBox->addWidget(maxLevel);
    speciesHBox->addWidget(percentLabel);

    return (QWidget *)speciesHBox;
}
