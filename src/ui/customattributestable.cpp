#include "customattributestable.h"
#include "customattributesdialog.h"
#include "parseutil.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QSpinBox>

enum Column {
    Key,
    Value,
    Count
};

CustomAttributesTable::CustomAttributesTable(QWidget *parent) :
    QTableWidget(parent)
{
    this->setColumnCount(Column::Count);
    this->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setHorizontalHeaderLabels(QStringList({"Key", "Value"}));
    this->horizontalHeader()->setStretchLastSection(true);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    //this->horizontalHeader()->setMaximumSectionSize(this->horizontalHeader()->length()); // TODO
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    connect(this, &QTableWidget::cellChanged, this, &CustomAttributesTable::edited);

    // Key cells are uneditable, but users should be allowed to select one and press delete to remove the row.
    // Adding the "Selectable" flag to the Key cell changes its appearance to match the Value cell, which
    // makes it confusing that you can't edit the Key cell. To keep the uneditable appearance and allow
    // deleting rows by selecting Key cells, we select the full row when a Key cell is selected.
    connect(this, &QTableWidget::cellClicked, [this](int row, int column) {
        if (column == Column::Key) {
            this->selectRow(row);
        }
    });

    // Double clicking the Key cell will bring up the dialog window to edit all the attribute's properties
    connect(this, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        if (column == Column::Key) {
            // Get cell data
            auto keyValuePair = this->getAttribute(row);
            auto key = keyValuePair.first;

            // TODO: This dialog is confusing if the name is changed
            // Create dialog
            CustomAttributesDialog dialog(this);
            dialog.setInput(key, keyValuePair.second, this->m_defaultKeys.contains(key));
            if (dialog.exec() == QDialog::Accepted) {
                emit this->edited();
            }
        }
    });
}

QMap<QString, QJsonValue> CustomAttributesTable::getAttributes() const {
    QMap<QString, QJsonValue> fields;
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
        return QPair<QString, QJsonValue>();

    // Read from the table data which JSON type to save the value as
    QJsonValue::Type type = static_cast<QJsonValue::Type>(keyItem->data(Qt::UserRole).toInt());

    QJsonValue value;
    if (type == QJsonValue::String) {
        value = QJsonValue(this->item(row, Column::Value)->text());
    } else if (type == QJsonValue::Double) {
        auto spinBox = static_cast<QSpinBox*>(this->cellWidget(row, Column::Value));
        value = QJsonValue(spinBox->value());
    } else if (type == QJsonValue::Bool) {
        value = QJsonValue(this->item(row, Column::Value)->checkState() == Qt::Checked);
    } else {
        // All other types will just be preserved
        value = this->item(row, Column::Value)->data(Qt::UserRole).toJsonValue();
    }

    return QPair<QString, QJsonValue>(keyItem->text(), value);
}

int CustomAttributesTable::addAttribute(const QString &key, QJsonValue value) {
    const QSignalBlocker blocker(this);

    // Certain key names cannot be used (if they would overwrite a field used outside this table)
    if (this->m_restrictedKeys.contains(key))
        return -1;

    // Overwrite existing key (if present)
    if (this->m_keys.contains(key))
        this->removeAttribute(key);

    // Add new row
    int rowIndex = this->rowCount();
    this->insertRow(rowIndex);

    QJsonValue::Type type = value.type();

    // Add key name to table
    auto keyItem = new QTableWidgetItem(key);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setData(Qt::UserRole, type); // Record the type for writing to the file
    keyItem->setTextAlignment(Qt::AlignCenter);
    keyItem->setToolTip(key); // Display name as tool tip in case it's too long to see in the cell
    this->setItem(rowIndex, Column::Key, keyItem);

    // Add value to table
    switch (type) {
    case QJsonValue::String: {
        // Add a regular cell item for editing text
        this->setItem(rowIndex, Column::Value, new QTableWidgetItem(ParseUtil::jsonToQString(value)));
        break;
    } case QJsonValue::Double: {
        // Add a spin box for editing number values
        auto spinBox = new QSpinBox(this);
        spinBox->setMinimum(INT_MIN);
        spinBox->setMaximum(INT_MAX);
        spinBox->setValue(ParseUtil::jsonToInt(value));
        // This connection will be handled by QTableWidget::cellChanged for other cell types
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this]() { emit this->edited(); });
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
        valueItem->setData(Qt::UserRole, value); // Preserve the value for writing to the file
        this->setItem(rowIndex, Column::Value, valueItem);
        break;
    }}
    this->m_keys.insert(key);

    return rowIndex;
}

// For the user adding an attribute by interacting with the table
void CustomAttributesTable::addNewAttribute(const QString &key, QJsonValue value, bool setAsDefault) {
    int row = this->addAttribute(key, value);
    if (row < 0) return;
    if (setAsDefault) this->setDefaultAttribute(key, value);
    this->resizeVertically();
    this->selectRow(row);
    emit this->edited();
}

// For programmatically populating the table
void CustomAttributesTable::setAttributes(const QMap<QString, QJsonValue> attributes) {
    this->m_keys.clear();
    this->setRowCount(0); // Clear old values
    for (auto it = attributes.cbegin(); it != attributes.cend(); it++)
        this->addAttribute(it.key(), it.value());
    this->resizeVertically();
}

void CustomAttributesTable::setDefaultAttribute(const QString &key, QJsonValue value) {
    this->m_defaultKeys.insert(key);
    emit this->defaultSet(key, value);
}

void CustomAttributesTable::unsetDefaultAttribute(const QString &key) {
    if (this->m_defaultKeys.remove(key))
        emit this->defaultRemoved(key);
}

void CustomAttributesTable::removeAttribute(const QString &key) {
    for (int row = 0; row < this->rowCount(); row++) {
        auto keyItem = this->item(row, Column::Key);
        if (keyItem && keyItem->text() == key) {
            this->m_keys.remove(key);
            this->removeRow(row);
            break;
        }
    }
}

bool CustomAttributesTable::deleteSelectedAttributes() {
    int rowCount = this->rowCount();
    if (rowCount <= 0)
        return false;

    QModelIndexList indexList = this->selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> persistentIndexes;
    for (QModelIndex index : indexList) {
        QPersistentModelIndex persistentIndex(index);
        persistentIndexes.append(persistentIndex);
    }

    if (persistentIndexes.isEmpty())
        return false;

    for (QPersistentModelIndex index : persistentIndexes) {
        auto row = index.row();
        auto item = this->item(row, Column::Key);
        if (item) this->m_keys.remove(item->text());
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
    if (this->rowCount() == 0) {
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
    this->resizeVertically();
    QAbstractItemView::resizeEvent(event);
}

QSet<QString> CustomAttributesTable::keys() const {
    return this->m_keys;
}

QSet<QString> CustomAttributesTable::defaultKeys() const {
    return this->m_defaultKeys;
}

QSet<QString> CustomAttributesTable::restrictedKeys() const {
    return this->m_restrictedKeys;
}

void CustomAttributesTable::setDefaultKeys(const QSet<QString> keys) {
    this->m_defaultKeys = keys;
}

void CustomAttributesTable::setRestrictedKeys(const QSet<QString> keys) {
    this->m_restrictedKeys = keys;
}
