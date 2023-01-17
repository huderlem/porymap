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
    // Press right mouse button to activate tab.
    if (event->type() == QEvent::MouseButtonPress
     && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
        QPoint eventPos = static_cast<QMouseEvent *>(event)->pos();
        int tabIndex = tabBar()->tabAt(eventPos);
        if (tabIndex > -1) {
            askActivateTab(tabIndex, eventPos);
        }
        return true;
    }
    else if (event->type() == QEvent::Wheel) {
        return true;
    }
    return false;
}

void MonTabWidget::populate() {
    EncounterFields fields = editor->project->wildMonFields;
    activeTabs = QVector<bool>(fields.size(), false);

    for (EncounterField field : fields) {
        QTableView *table = new QTableView(this);
        table->clearFocus();
        addTab(table, field.name);
    }
}

void MonTabWidget::askActivateTab(int tabIndex, QPoint menuPos) {
    if (activeTabs[tabIndex]) {
        // copy from another tab
        QMenu contextMenu(this);

        for (int i = 0; i < this->tabBar()->count(); i++) {
            if (tabIndex == i) continue;
            if (!activeTabs[i]) continue;

            QString tabText = this->tabBar()->tabText(i);
            QAction *actionCopyFrom = new QAction(QString("Copy encounters from %1").arg(tabText), &contextMenu);

            connect(actionCopyFrom, &QAction::triggered, [=](){
                EncounterTableModel *model = static_cast<EncounterTableModel *>(this->tableAt(i)->model());
                WildMonInfo copyInfo = model->encounterData();
                clearTableAt(tabIndex);
                WildMonInfo newInfo = getDefaultMonInfo(editor->project->wildMonFields.at(tabIndex));
                combineEncounters(newInfo, copyInfo);
                populateTab(tabIndex, newInfo);
                emit editor->wildMonDataChanged();
            });

            contextMenu.addAction(actionCopyFrom);
        }
        contextMenu.exec(mapToGlobal(menuPos));
    }
    else {
        QMenu contextMenu(this);

        QString tabText = tabBar()->tabText(tabIndex);
        QAction actionActivateTab(QString("Add %1 data for this map...").arg(tabText), this);
        connect(&actionActivateTab, &QAction::triggered, [=](){
            clearTableAt(tabIndex);
            populateTab(tabIndex, getDefaultMonInfo(editor->project->wildMonFields.at(tabIndex)));
            editor->saveEncounterTabData();
            setCurrentIndex(tabIndex);
            emit editor->wildMonDataChanged();
        });
        contextMenu.addAction(&actionActivateTab);
        contextMenu.exec(mapToGlobal(menuPos));
    }
}

void MonTabWidget::clearTableAt(int tabIndex) {
    QTableView *table = tableAt(tabIndex);
    if (table) {
        table->reset();
        table->horizontalHeader()->hide();
    }
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
        setTabToolTip(index, "Right-click an inactive tab to add new fields.");
    } else {
        setTabToolTip(index, QString());
    }
}
