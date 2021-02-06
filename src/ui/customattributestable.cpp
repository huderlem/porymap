#include "customattributestable.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QScrollBar>

CustomAttributesTable::CustomAttributesTable(Event *event, QWidget *parent) :
    QFrame(parent)
{
    this->event = event;

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Custom Attributes");
    layout->addWidget(label);

    QFrame *buttonsFrame = new QFrame(this);
    buttonsFrame->setLayout(new QHBoxLayout());
    QPushButton *addButton = new QPushButton(this);
    QPushButton *deleteButton = new QPushButton(this);
    addButton->setText("Add");
    deleteButton->setText("Delete");
    buttonsFrame->layout()->addWidget(addButton);
    buttonsFrame->layout()->addWidget(deleteButton);
    buttonsFrame->layout()->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
    buttonsFrame->layout()->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(buttonsFrame);

    this->table = new QTableWidget(this);
    this->table->setColumnCount(2);
    this->table->setHorizontalHeaderLabels(QStringList({"Key", "Value"}));
    this->table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(this->table);

    for (auto it = event->customValues.begin(); it != event->customValues.end(); it++) {
        int rowIndex = this->table->rowCount();
        this->table->insertRow(rowIndex);
        this->table->setItem(rowIndex, 0, new QTableWidgetItem(it.key()));
        this->table->setItem(rowIndex, 1, new QTableWidgetItem(it.value()));
    }

    connect(addButton, &QPushButton::clicked, [=]() {
        int rowIndex = this->table->rowCount();
        this->table->insertRow(rowIndex);
        this->table->selectRow(rowIndex);
        this->event->customValues = this->getTableFields();
        this->resizeVertically();
    });

    connect(deleteButton, &QPushButton::clicked, [=]() {
        int rowCount = this->table->rowCount();
        if (rowCount > 0) {
            QModelIndexList indexList = this->table->selectionModel()->selectedIndexes();
            QList<QPersistentModelIndex> persistentIndexes;
            for (QModelIndex index : indexList) {
                QPersistentModelIndex persistentIndex(index);
                persistentIndexes.append(persistentIndex);
            }

            for (QPersistentModelIndex index : persistentIndexes) {
                this->table->removeRow(index.row());
            }

            if (this->table->rowCount() > 0) {
                this->table->selectRow(0);
            }

            this->event->customValues = this->getTableFields();
            this->resizeVertically();
        }
    });

    connect(this->table, &QTableWidget::cellChanged, [=]() {
        this->event->customValues = this->getTableFields();
    });

    this->resizeVertically();
}

CustomAttributesTable::~CustomAttributesTable()
{
}

QMap<QString, QString> CustomAttributesTable::getTableFields() {
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
    return fields;
}

void CustomAttributesTable::resizeVertically() {
    int horizontalHeaderHeight = this->table->horizontalHeader()->height();
    int rowHeight = 0;
    for (int i = 0; i < this->table->rowCount(); i++) {
        rowHeight += this->table->rowHeight(0);
    }
    int totalHeight = horizontalHeaderHeight + rowHeight;
    if (this->table->rowCount() == 0) {
        totalHeight += 1;
    } else {
        totalHeight += 2;
    }
    this->table->setMinimumHeight(totalHeight);
    this->table->setMaximumHeight(totalHeight);
}
