#include "customattributestable.h"
#include "customattributesdialog.h"
#include "parseutil.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QScrollBar>
#include <QInputDialog>

// TODO: Tooltip-- "Custom fields will be added to the map.json file for the current map."?
// TODO: Fix squishing when first element is added
CustomAttributesTable::CustomAttributesTable(QWidget *parent) :
    QFrame(parent)
{
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
    this->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->table->horizontalHeader()->setVisible(false);
    this->table->verticalHeader()->setVisible(false);
    layout->addWidget(this->table);
    layout->addStretch(1);

    connect(addButton, &QPushButton::clicked, [this]() {
        bool ok;
        CustomAttributesDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            emit this->edited();
        }
    });

    connect(deleteButton, &QPushButton::clicked, [this]() {
        if (this->deleteSelectedAttributes()) {
            emit this->edited();
        }
    });

    connect(this->table, &QTableWidget::cellChanged, [this]() {
        emit this->edited();
    });
}

CustomAttributesTable::~CustomAttributesTable()
{
}

// TODO: Fix Header table size on first open
void CustomAttributesTable::resizeVertically() {
    int height = 0;
    for (int i = 0; i < this->table->rowCount(); i++) {
        height += this->table->rowHeight(i);
    }

    // Header disappears if there are no entries
    if (this->table->rowCount() == 0) {
        this->table->horizontalHeader()->setVisible(false);
    } else {
        this->table->horizontalHeader()->setVisible(true);
        height += this->table->horizontalHeader()->height() + 2;
    }

    this->table->setMinimumHeight(height);
    this->table->setMaximumHeight(height);
    this->updateGeometry();
}

QMap<QString, QJsonValue> CustomAttributesTable::getAttributes() const {
    QMap<QString, QJsonValue> fields;
    for (int row = 0; row < this->table->rowCount(); row++) {
        QString key = "";
        QTableWidgetItem *typeItem = this->table->item(row, 0);
        QTableWidgetItem *keyItem = this->table->item(row, 1);
        QTableWidgetItem *valueItem = this->table->item(row, 2);

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

void CustomAttributesTable::setAttributes(const QMap<QString, QJsonValue> attributes) {
    this->table->setRowCount(0);
    for (auto it = attributes.cbegin(); it != attributes.cend(); it++)
        this->addAttribute(it.key(), it.value(), true);
    this->resizeVertically();
}

void CustomAttributesTable::addAttribute(QString key, QJsonValue value) {
    this->addAttribute(key, value, false);
}

void CustomAttributesTable::addAttribute(QString key, QJsonValue value, bool init) {
    const QSignalBlocker blocker(this->table);
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

    static const QHash<QJsonValue::Type, QString> typeToName = {
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

    int rowIndex = this->table->rowCount();
    this->table->insertRow(rowIndex);
    this->table->setItem(rowIndex, 0, typeItem);
    this->table->setItem(rowIndex, 1, new QTableWidgetItem(key));
    this->table->setItem(rowIndex, 2, valueItem);

    if (!init) {
        valueItem->setText(""); // Erase the "0" in new numbers
        this->table->selectRow(rowIndex);
        this->resizeVertically();
    }
}

void CustomAttributesTable::setDefaultAttribute(QString key, QJsonValue value) {
    // TODO
}

void CustomAttributesTable::unsetDefaultAttribute(QString key) {
    // TODO
}

bool CustomAttributesTable::deleteSelectedAttributes() {
    int rowCount = this->table->rowCount();
    if (rowCount <= 0)
        return false;

    QModelIndexList indexList = this->table->selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> persistentIndexes;
    for (QModelIndex index : indexList) {
        QPersistentModelIndex persistentIndex(index);
        persistentIndexes.append(persistentIndex);
    }

    if (persistentIndexes.isEmpty())
        return false;

    for (QPersistentModelIndex index : persistentIndexes) {
        this->table->removeRow(index.row());
    }

    if (this->table->rowCount() > 0) {
        this->table->selectRow(0);
    }
    this->resizeVertically();
    return true;
}
