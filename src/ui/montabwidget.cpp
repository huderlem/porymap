#include "montabwidget.h"
#include "noscrollcombobox.h"
#include "editor.h"
#include "encountertablemodel.h"
#include "encountertabledelegates.h"



MonTabWidget::MonTabWidget(Editor *editor, QWidget *parent) : QTabWidget(parent) {
    this->editor = editor;
    populate();
    this->tabBar()->installEventFilter(this);
}

bool MonTabWidget::eventFilter(QObject *, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        return true;
    }
    return false;
}

void MonTabWidget::populate() {
    EncounterFields fields = editor->project->wildMonFields;
    activeTabs.resize(fields.size());
    activeTabs.fill(false);

    addDeleteTabButtons.resize(fields.size());
    addDeleteTabButtons.fill(nullptr);
    copyTabButtons.resize(fields.size());
    copyTabButtons.fill(nullptr);

    int index = 0;
    for (EncounterField field : fields) {
        QTableView *table = new QTableView(this);
        table->clearFocus();
        addTab(table, field.name);

        QPushButton *buttonAddDelete = new QPushButton(QIcon(":/icons/add.ico"), "");
        connect(buttonAddDelete, &QPushButton::clicked, [=]() { actionAddDeleteTab(index); });
        addDeleteTabButtons[index] = buttonAddDelete;
        this->tabBar()->setTabButton(index, QTabBar::RightSide, buttonAddDelete);

        QPushButton *buttonCopy = new QPushButton(QIcon(":/icons/clipboard.ico"), "");
        connect(buttonCopy, &QPushButton::clicked, [=]() {actionCopyTab(index); });
        copyTabButtons[index] = buttonCopy;
        this->tabBar()->setTabButton(index, QTabBar::LeftSide, buttonCopy);

        index++;
    }
}

void MonTabWidget::actionCopyTab(int index) {
    QMenu contextMenu(this);

    for (int i = 0; i < this->tabBar()->count(); i++) {
        if (index == i) continue;
        if (!activeTabs[i]) continue;

        QString tabText = this->tabBar()->tabText(i);
        QAction *actionCopyFrom = new QAction(QString("Copy encounters from %1").arg(tabText), &contextMenu);

        connect(actionCopyFrom, &QAction::triggered, [=](){
            EncounterTableModel *model = static_cast<EncounterTableModel *>(this->tableAt(i)->model());
            WildMonInfo copyInfo = model->encounterData();
            clearTableAt(index);
            WildMonInfo newInfo = getDefaultMonInfo(editor->project->wildMonFields.at(index));
            combineEncounters(newInfo, copyInfo);
            populateTab(index, newInfo);
            emit editor->wildMonDataChanged();
        });

        contextMenu.addAction(actionCopyFrom);
    }
    contextMenu.exec(mapToGlobal(this->copyTabButtons[index]->pos() + QPoint(0, this->copyTabButtons[index]->height())));
}

void MonTabWidget::actionAddDeleteTab(int index) {
    if (activeTabs[index]) {
        // delete tab
        clearTableAt(index);
        deactivateTab(index);
        editor->saveEncounterTabData();
        emit editor->wildMonDataChanged();
    }
    else {
        // add tab
        clearTableAt(index);
        populateTab(index, getDefaultMonInfo(editor->project->wildMonFields.at(index)));
        editor->saveEncounterTabData();
        setCurrentIndex(index);
        emit editor->wildMonDataChanged();
    }
}

void MonTabWidget::clearTableAt(int tabIndex) {
    QTableView *table = tableAt(tabIndex);
    if (table) {
        table->reset();
        table->horizontalHeader()->hide();
    }
}

void MonTabWidget::deactivateTab(int tabIndex) {
    QTableView *speciesTable = tableAt(tabIndex);

    EncounterTableModel *oldModel = static_cast<EncounterTableModel *>(speciesTable->model());
    WildMonInfo monInfo = oldModel->encounterData();
    monInfo.active = false;
    EncounterTableModel *newModel = new EncounterTableModel(monInfo, editor->project->wildMonFields, tabIndex, this);
    speciesTable->setModel(newModel);

    setTabActive(tabIndex, false);
}

void MonTabWidget::populateTab(int tabIndex, WildMonInfo monInfo) {
    QTableView *speciesTable = tableAt(tabIndex);

    EncounterTableModel *model = new EncounterTableModel(monInfo, editor->project->wildMonFields, tabIndex, this);
    connect(model, &EncounterTableModel::edited, editor, &Editor::saveEncounterTabData);
    speciesTable->setModel(model);

    speciesTable->setItemDelegateForColumn(EncounterTableModel::ColumnType::Species, new SpeciesComboDelegate(editor->project, this));
    speciesTable->setItemDelegateForColumn(EncounterTableModel::ColumnType::MinLevel, new SpinBoxDelegate(editor->project, this));
    speciesTable->setItemDelegateForColumn(EncounterTableModel::ColumnType::MaxLevel, new SpinBoxDelegate(editor->project, this));
    speciesTable->setItemDelegateForColumn(EncounterTableModel::ColumnType::EncounterRate, new SpinBoxDelegate(editor->project, this));

    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::Slot, QHeaderView::ResizeToContents);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::Group, QHeaderView::ResizeToContents);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::Species, QHeaderView::Stretch);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::MinLevel, QHeaderView::Stretch);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::MaxLevel, QHeaderView::Stretch);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::SlotRatio, QHeaderView::ResizeToContents);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::EncounterChance, QHeaderView::ResizeToContents);
    speciesTable->horizontalHeader()->setSectionResizeMode(EncounterTableModel::ColumnType::EncounterRate, QHeaderView::ResizeToContents);

    // give enough vertical space for icons + margins
    speciesTable->verticalHeader()->setMinimumSectionSize(40);

    if (editor->project->wildMonFields[tabIndex].groups.empty()) {
        speciesTable->setColumnHidden(1, true);
    }

    speciesTable->horizontalHeader()->show();
    this->setTabActive(tabIndex, true);
}

QTableView *MonTabWidget::tableAt(int tabIndex) {
    return static_cast<QTableView *>(this->widget(tabIndex));
}

void MonTabWidget::setTabActive(int index, bool active) {
    activeTabs[index] = active;
    setTabEnabled(index, active);
    if (!active) {
        this->addDeleteTabButtons[index]->setIcon(QIcon(":/icons/add.ico"));
        this->copyTabButtons[index]->hide();
    } else {
        this->addDeleteTabButtons[index]->setIcon(QIcon(":/icons/delete.ico"));
        this->copyTabButtons[index]->show();
    }
}
