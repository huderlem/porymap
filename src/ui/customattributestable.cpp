#include "customattributestable.h"
#include "customattributesdialog.h"
#include "parseutil.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QScrollBar>
#include <QSpinBox>

enum Column {
    Key,
    Value,
    Count
};

CustomAttributesTable::CustomAttributesTable(QWidget *parent) :
    QFrame(parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);

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
    this->table->setColumnCount(Column::Count);
    this->table->setHorizontalHeaderLabels(QStringList({"Key", "Value"}));
    this->table->horizontalHeader()->setStretchLastSection(true);
    this->table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->table->horizontalHeader()->setVisible(false);
    this->table->verticalHeader()->setVisible(false);
    layout->addWidget(this->table);
    layout->addStretch(1);

    // Connect the "Add" button
    connect(addButton, &QPushButton::clicked, [this]() {
        CustomAttributesDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            emit this->edited();
        }
    });

    // Connect the "Delete" button
    connect(deleteButton, &QPushButton::clicked, [this]() {
        if (this->deleteSelectedAttributes()) {
            emit this->edited();
        }
    });

    connect(this->table, &QTableWidget::cellChanged, [this]() {
        emit this->edited();
    });

    // Key cells are uneditable, but users should be allowed to select one and press delete to remove the row.
    // Adding the "Selectable" flag to the Key cell changes its appearance to match the Value cell, which
    // makes it confusing that you can't edit the Key cell. To keep the uneditable appearance and allow
    // deleting rows by selecting Key cells, we select the full row when a Key cell is selected.
    // Double clicking the Key cell will begin editing the value cell, as it would for double-clicking the value cell.
    connect(this->table, &QTableWidget::cellClicked, [this](int row, int column) {
        if (column == Column::Key) {
            this->table->selectRow(row);
        }
    });
    connect(this->table, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        if (column == Column::Key) {
            auto index = this->table->model()->index(row, Column::Value);
            this->table->edit(index);
        }
    });
    // TODO: Right-click for context menu to set a default?
}

CustomAttributesTable::~CustomAttributesTable()
{
}

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
        auto keyItem = this->table->item(row, Column::Key);
        if (keyItem) key = keyItem->text();
        if (key.isEmpty())
            continue;

        // Read from the table data which JSON type to save the value as
        QJsonValue::Type type = static_cast<QJsonValue::Type>(keyItem->data(Qt::UserRole).toInt());

        QJsonValue value;
        switch (type) {
        case QJsonValue::String: {
            value = QJsonValue(this->table->item(row, Column::Value)->text());
            break;
        } case QJsonValue::Double: {
            auto spinBox = static_cast<QSpinBox*>(this->table->cellWidget(row, Column::Value));
            value = QJsonValue(spinBox->value());
            break;
        } case QJsonValue::Bool: {
            value = QJsonValue(this->table->item(row, Column::Value)->checkState() == Qt::Checked);
            break;
        } default: {
            // All other types will just be preserved
            value = this->table->item(row, Column::Value)->data(Qt::UserRole).toJsonValue();
            break;
        }}
        fields[key] = value;
    }
    return fields;
}

int CustomAttributesTable::addAttribute(const QString &key, QJsonValue value) {
    const QSignalBlocker blocker(this->table);

    // Certain key names cannot be used (if they would overwrite a field used outside this table)
    if (this->m_restrictedKeys.contains(key))
        return -1;

    // Overwrite existing key (if present)
    if (this->m_keys.contains(key))
        this->removeAttribute(key);

    // Add new row
    int rowIndex = this->table->rowCount();
    this->table->insertRow(rowIndex);

    QJsonValue::Type type = value.type();

    // Add key name to table
    auto keyItem = new QTableWidgetItem(key);
    keyItem->setFlags(Qt::ItemIsEnabled);
    keyItem->setData(Qt::UserRole, type); // Record the type for writing to the file
    keyItem->setTextAlignment(Qt::AlignCenter);
    this->table->setItem(rowIndex, Column::Key, keyItem);

    // Add value to table
    switch (type) {
    case QJsonValue::String: {
        // Add a regular cell item for editing text
        this->table->setItem(rowIndex, Column::Value, new QTableWidgetItem(ParseUtil::jsonToQString(value)));
        break;
    } case QJsonValue::Double: {
        // Add a spin box for editing number values
        auto spinBox = new QSpinBox(this->table);
        spinBox->setMinimum(INT_MIN);
        spinBox->setMaximum(INT_MAX);
        spinBox->setValue(ParseUtil::jsonToInt(value));
        // This connection will be handled by QTableWidget::cellChanged for other cell types
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this]() { emit this->edited(); });
        this->table->setCellWidget(rowIndex, Column::Value, spinBox);
        break;
    } case QJsonValue::Bool: {
        // Add a checkable cell item for editing bools
        auto valueItem = new QTableWidgetItem("");
        valueItem->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
        valueItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        this->table->setItem(rowIndex, Column::Value, valueItem);
        break;
    } default: {
        // Arrays, objects, or null/undefined values cannot be edited
        auto valueItem = new QTableWidgetItem("This value cannot be edited from this table");
        valueItem->setFlags(Qt::NoItemFlags);
        valueItem->setData(Qt::UserRole, value); // Preserve the value for writing to the file
        this->table->setItem(rowIndex, Column::Value, valueItem);
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
    this->table->selectRow(row);
    this->resizeVertically();
}

// For programmatically populating the table
void CustomAttributesTable::setAttributes(const QMap<QString, QJsonValue> attributes) {
    this->table->setRowCount(0); // Clear old values
    for (auto it = attributes.cbegin(); it != attributes.cend(); it++)
        this->addAttribute(it.key(), it.value());
    this->resizeVertically();
}

void CustomAttributesTable::setDefaultAttribute(const QString &key, QJsonValue value) {
    m_defaultKeys.insert(key);
    emit this->defaultSet(key, value);
}

void CustomAttributesTable::unsetDefaultAttribute(const QString &key) {
    if (m_defaultKeys.remove(key))
        emit this->defaultRemoved(key);
}

void CustomAttributesTable::removeAttribute(const QString &key) {
    for (int row = 0; row < this->table->rowCount(); row++) {
        auto keyItem = this->table->item(row, Column::Key);
        if (keyItem && keyItem->text() == key) {
            this->m_keys.remove(key);
            this->table->removeRow(row);
            break;
        }
    }
    // No need to adjust size because this is (at the moment) only used for replacement
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
        auto row = index.row();
        auto item = this->table->item(row, Column::Key);
        if (item) this->m_keys.remove(item->text());
        this->table->removeRow(row);
    }

    if (this->table->rowCount() > 0) {
        this->table->selectRow(0);
    }
    this->resizeVertically();
    return true;
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
