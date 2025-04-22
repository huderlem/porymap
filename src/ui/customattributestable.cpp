#include "customattributestable.h"
#include "parseutil.h"
#include "noscrollspinbox.h"
#include "utility.h"
#include <QHeaderView>
#include <QScrollBar>

enum Column {
    Key,
    Value,
    Count
};

enum DataRole {
    JsonType = Qt::UserRole,
    OriginalValue,
};

CustomAttributesTable::CustomAttributesTable(QWidget *parent) :
    QTableWidget(parent)
{
    this->setColumnCount(Column::Count);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalHeaderLabels(QStringList({"Key", "Value"}));
    this->horizontalHeader()->setStretchLastSection(true);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    connect(this, &QTableWidget::cellChanged, this, &CustomAttributesTable::edited);

    // Key cells are uneditable, but users should be allowed to select one and press delete to remove the row.
    // Adding the "Selectable" flag to the Key cell changes its appearance to match the Value cell, which
    // makes it confusing that you can't edit the Key cell. To keep the uneditable appearance and allow
    // deleting rows by selecting Key cells, we select the full row when a Key cell is selected.
    connect(this, &QTableWidget::cellPressed, [this](int row, int column) {
        if (column == Column::Key) {
            this->selectRow(row);
        }
    });
}

QJsonObject CustomAttributesTable::getAttributes() const {
    QJsonObject fields;
    for (int row = 0; row < this->rowCount(); row++) {
        auto keyValuePair = this->getAttribute(row);
        if (!keyValuePair.first.isEmpty())
            fields[keyValuePair.first] = keyValuePair.second;
    }
    return fields;
}

QPair<QString, QJsonValue> CustomAttributesTable::getAttribute(int row) const {
    auto keyItem = this->item(row, Column::Key);
    if (!keyItem)
        return {};

    // Read from the table data which JSON type to save the value as
    QJsonValue::Type type = static_cast<QJsonValue::Type>(keyItem->data(DataRole::JsonType).toInt());

    QJsonValue value;
    if (type == QJsonValue::String) {
        value = QJsonValue(this->item(row, Column::Value)->text());
    } else if (type == QJsonValue::Double) {
        auto spinBox = static_cast<NoScrollSpinBox*>(this->cellWidget(row, Column::Value));
        value = QJsonValue(spinBox->value());
    } else if (type == QJsonValue::Bool) {
        value = QJsonValue(this->item(row, Column::Value)->checkState() == Qt::Checked);
    } else {
        // All other types will just be preserved
        value = this->item(row, Column::Value)->data(DataRole::OriginalValue).toJsonValue();
    }

    return {keyItem->text(), value};
}

int CustomAttributesTable::addAttribute(const QString &key, const QJsonValue &value) {
    // Stop 'edited' signals from being emitted before we finish creating the new table data.
    const QSignalBlocker blocker(this);

    // Certain key names cannot be used (if they would overwrite a field used outside this table)
    if (m_restrictedKeys.contains(key))
        return -1;

    // Overwrite existing key (if present)
    if (m_keys.contains(key))
        this->removeAttribute(key);

    // Add new row
    int rowIndex = this->rowCount();
    this->insertRow(rowIndex);

    QJsonValue::Type type = value.type();

    // Add key name to table
    auto keyItem = new QTableWidgetItem(key);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setData(DataRole::JsonType, type); // Record the type for writing to the file
    keyItem->setTextAlignment(Qt::AlignCenter);
    keyItem->setToolTip(Util::toHtmlParagraph(key)); // Display name as tool tip in case it's too long to see in the cell
    this->setItem(rowIndex, Column::Key, keyItem);

    // Add value to table
    switch (type) {
    case QJsonValue::String: {
        // Add a regular cell item for editing text
        this->setItem(rowIndex, Column::Value, new QTableWidgetItem(ParseUtil::jsonToQString(value)));
        break;
    } case QJsonValue::Double: {
        // Add a spin box for editing number values
        auto spinBox = new NoScrollSpinBox(this);
        spinBox->setMinimum(INT_MIN);
        spinBox->setMaximum(INT_MAX);
        spinBox->setValue(ParseUtil::jsonToInt(value));
        // This connection will be handled by QTableWidget::cellChanged for other cell types
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CustomAttributesTable::edited);
        this->setCellWidget(rowIndex, Column::Value, spinBox);
        break;
    } case QJsonValue::Bool: {
        // Add a checkable cell item for editing bools
        auto valueItem = new QTableWidgetItem("");
        valueItem->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
        valueItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        this->setItem(rowIndex, Column::Value, valueItem);
        break;
    } default: {
        // Arrays, objects, or null/undefined values cannot be edited
        auto valueItem = new QTableWidgetItem("This value cannot be edited from this table");
        valueItem->setFlags(Qt::NoItemFlags);
        valueItem->setData(DataRole::OriginalValue, value); // Preserve the value for writing to the file
        this->setItem(rowIndex, Column::Value, valueItem);
        break;
    }}
    m_keys.insert(key);

    return rowIndex;
}

// For the user adding an attribute by interacting with the table
void CustomAttributesTable::addNewAttribute(const QString &key, const QJsonValue &value) {
    int row = this->addAttribute(key, value);
    if (row < 0) return;
    this->resizeVertically();
    this->selectRow(row);
    emit this->edited();
}

// For programmatically populating the table
void CustomAttributesTable::setAttributes(const QJsonObject &attributes) {
    m_keys.clear();
    this->setRowCount(0); // Clear old values
    for (auto it = attributes.constBegin(); it != attributes.constEnd(); it++)
        this->addAttribute(it.key(), it.value());
    this->resizeVertically();
}

void CustomAttributesTable::removeAttribute(const QString &key) {
    for (int row = 0; row < this->rowCount(); row++) {
        auto keyItem = this->item(row, Column::Key);
        if (keyItem && keyItem->text() == key) {
            m_keys.remove(key);
            this->removeRow(row);
            break;
        }
    }
}

bool CustomAttributesTable::deleteSelectedAttributes() {
    if (this->isEmpty())
        return false;

    QModelIndexList indexList = this->selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> persistentIndexes;
    for (const auto &index : indexList) {
        QPersistentModelIndex persistentIndex(index);
        persistentIndexes.append(persistentIndex);
    }

    if (persistentIndexes.isEmpty())
        return false;

    for (const auto &index : persistentIndexes) {
        auto row = index.row();
        auto item = this->item(row, Column::Key);
        if (item) m_keys.remove(item->text());
        this->removeRow(row);
    }
    this->resizeVertically();

    if (this->rowCount() > 0) {
        this->selectRow(0);
    }
    emit this->edited();
    return true;
}

void CustomAttributesTable::resizeVertically() {
    int height = 0;
    if (this->isEmpty()) {
        // Hide header when table is empty
        this->horizontalHeader()->setVisible(false);
    } else {
        for (int i = 0; i < this->rowCount(); i++)
            height += this->rowHeight(i);

        // Account for header and horizontal scroll bar
        this->horizontalHeader()->setVisible(true);
        height += this->horizontalHeader()->height();
        if (this->horizontalScrollBar()->isVisible())
            height += this->horizontalScrollBar()->height();
        height += 2; // Border
    }

    this->setMinimumHeight(height);
    this->setMaximumHeight(height);
}

void CustomAttributesTable::resizeEvent(QResizeEvent *event) {
    QTableWidget::resizeEvent(event);
    this->resizeVertically();
}

bool CustomAttributesTable::isEmpty() const {
    return this->rowCount() <= 0;
}

bool CustomAttributesTable::isSelectionEmpty() const {
    return this->selectedIndexes().isEmpty();
}
