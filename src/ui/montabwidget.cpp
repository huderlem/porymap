#include "montabwidget.h"
#include "noscrollcombobox.h"
#include "editor.h"
#include "encountertablemodel.h"
#include "encountertabledelegates.h"



static WildMonInfo encounterClipboard;

MonTabWidget::MonTabWidget(Editor *editor, QWidget *parent) : QTabWidget(parent) {
    this->editor = editor;
    populate();
    this->tabBar()->installEventFilter(this);
}

MonTabWidget::~MonTabWidget() {

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
        table->setEditTriggers(QAbstractItemView::AllEditTriggers);
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

void MonTabWidget::copy(int index) {
    EncounterTableModel *model = static_cast<EncounterTableModel *>(this->tableAt(index)->model());
    encounterClipboard = model->encounterData();
}

void MonTabWidget::paste(int index) {
    if (!encounterClipboard.active) return;

    clearTableAt(index);
    WildMonInfo newInfo = getDefaultMonInfo(this->editor->project->wildMonFields.at(index));
    combineEncounters(newInfo, encounterClipboard);
    populateTab(index, newInfo);
    emit editor->wildMonTableEdited();
}

void MonTabWidget::actionCopyTab(int index) {
    QMenu contextMenu(this);

    QAction *actionCopy = new QAction("Copy", &contextMenu);
    connect(actionCopy, &QAction::triggered, [=]() { copy(index); });
    contextMenu.addAction(actionCopy);

    if (encounterClipboard.active) {
        QAction *actionPaste = new QAction("Paste", &contextMenu);
        connect(actionPaste, &QAction::triggered, [=]() { paste(index); });
        contextMenu.addAction(actionPaste);
    }

    contextMenu.exec(mapToGlobal(this->copyTabButtons[index]->pos() + QPoint(0, this->copyTabButtons[index]->height())));
}

void MonTabWidget::actionAddDeleteTab(int index) {
    clearTableAt(index);
    if (activeTabs[index]) {
        // delete tab
        deactivateTab(index);
        editor->saveEncounterTabData();
    }
    else {
        // add tab
        populateTab(index, getDefaultMonInfo(editor->project->wildMonFields.at(index)));
        editor->saveEncounterTabData();
        setCurrentIndex(index);
    }
    emit editor->wildMonTableEdited();
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
    connect(model, &EncounterTableModel::edited, editor, &Editor::wildMonTableEdited);
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
    if (tabIndex < 0)
        return nullptr;
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
