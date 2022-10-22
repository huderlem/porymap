#include "customattributestable.h"
#include "parseutil.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QScrollBar>
#include <QInputDialog>

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
    this->table->setColumnCount(3);
    this->table->setHorizontalHeaderLabels(QStringList({"Type", "Key", "Value"}));
    this->table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(this->table);

    QMap<QString, QJsonValue> customValues = this->event->getCustomValues();
    for (auto it = customValues.begin(); it != customValues.end(); it++)
        CustomAttributesTable::addAttribute(this->table, it.key(), it.value());

    connect(addButton, &QPushButton::clicked, [=]() {
        bool ok;
        QJsonValue value = CustomAttributesTable::pickType(this, &ok);
        if (ok){
            CustomAttributesTable::addAttribute(this->table, "", value, true);
            this->event->setCustomValues(CustomAttributesTable::getAttributes(this->table));
            this->resizeVertically();
        }
    });

    connect(deleteButton, &QPushButton::clicked, [=]() {
        if (CustomAttributesTable::deleteSelectedAttributes(this->table)) {
            this->event->setCustomValues(CustomAttributesTable::getAttributes(this->table));
            this->resizeVertically();
        }
    });

    connect(this->table, &QTableWidget::cellChanged, [=]() {
        this->event->setCustomValues(CustomAttributesTable::getAttributes(this->table));
    });

    this->resizeVertically();
}

CustomAttributesTable::~CustomAttributesTable()
{
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

const QMap<QString, QJsonValue> CustomAttributesTable::getAttributes(QTableWidget * table) {
    QMap<QString, QJsonValue> fields;
    if (!table) return fields;

    for (int row = 0; row < table->rowCount(); row++) {
        QString key = "";
        QTableWidgetItem *typeItem = table->item(row, 0);
        QTableWidgetItem *keyItem = table->item(row, 1);
        QTableWidgetItem *valueItem = table->item(row, 2);

        if (keyItem) key = keyItem->text();
        if (key.isEmpty() || !typeItem || !valueItem)
            continue;

        // Read from the table data which JSON type to save the value as
        QJsonValue::Type type = static_cast<QJsonValue::Type>(typeItem->data(Qt::UserRole).toInt());
        QJsonValue value;
        switch (type)
        {
        case QJsonValue::String:
            value = QJsonValue(valueItem->text());
            break;
        case QJsonValue::Double:
            value = QJsonValue(valueItem->text().toInt());
            break;
        case QJsonValue::Bool:
            value = QJsonValue(valueItem->checkState() == Qt::Checked);
            break;
        default:
            // All other types will just be preserved
            value = valueItem->data(Qt::UserRole).toJsonValue();
            break;
        }
        fields[key] = value;
    }
    return fields;
}

QJsonValue CustomAttributesTable::pickType(QWidget * parent, bool * ok) {
    const QMap<QString, QJsonValue> valueTypes = {
        {"String",  QJsonValue(QString(""))},
        {"Number",  QJsonValue(0)},
        {"Boolean", QJsonValue(false)},
    };
    QStringList typeNames = valueTypes.keys();
    QString selection = QInputDialog::getItem(parent, "", "Choose Value Type", typeNames, typeNames.indexOf("String"), false, ok);
    return valueTypes.value(selection);
}

void CustomAttributesTable::addAttribute(QTableWidget * table, QString key, QJsonValue value, bool isNew) {
    if (!table) return;
    QTableWidgetItem * valueItem;
    QJsonValue::Type type = value.type();
    switch (type)
    {
    case QJsonValue::String:
    case QJsonValue::Double:
        valueItem = new QTableWidgetItem(ParseUtil::jsonToQString(value));
        break;
    case QJsonValue::Bool:
        valueItem = new QTableWidgetItem("");
        valueItem->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
        valueItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        break;
    default:
        valueItem = new QTableWidgetItem("This value cannot be edited from this table");
        valueItem->setFlags(Qt::ItemIsSelectable);
        valueItem->setData(Qt::UserRole, value); // Preserve the value for writing to the file
        break;
    }

    const QHash<QJsonValue::Type, QString> typeToName = {
        {QJsonValue::Bool, "Bool"},
        {QJsonValue::Double, "Number"},
        {QJsonValue::String, "String"},
        {QJsonValue::Array, "Array"},
        {QJsonValue::Object, "Object"},
        {QJsonValue::Null, "Null"},
        {QJsonValue::Undefined, "Null"},
    };
    QTableWidgetItem * typeItem = new QTableWidgetItem(typeToName[type]);
    typeItem->setFlags(Qt::ItemIsEnabled);
    typeItem->setData(Qt::UserRole, type); // Record the type for writing to the file
    typeItem->setTextAlignment(Qt::AlignCenter);

    int rowIndex = table->rowCount();
    table->insertRow(rowIndex);
    table->setItem(rowIndex, 0, typeItem);
    table->setItem(rowIndex, 1, new QTableWidgetItem(key));
    table->setItem(rowIndex, 2, valueItem);

    if (isNew) {
        valueItem->setText(""); // Erase the "0" in new numbers
        table->selectRow(rowIndex);
    }
}

bool CustomAttributesTable::deleteSelectedAttributes(QTableWidget * table) {
    if (!table)
        return false;

    int rowCount = table->rowCount();
    if (rowCount <= 0)
        return false;

    QModelIndexList indexList = table->selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> persistentIndexes;
    for (QModelIndex index : indexList) {
        QPersistentModelIndex persistentIndex(index);
        persistentIndexes.append(persistentIndex);
    }

    for (QPersistentModelIndex index : persistentIndexes) {
        table->removeRow(index.row());
    }

    if (table->rowCount() > 0) {
        table->selectRow(0);
    }
    return true;
}
