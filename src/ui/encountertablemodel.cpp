#include "encountertablemodel.h"
#include "editor.h"



EncounterTableModel::EncounterTableModel(WildMonInfo info, EncounterFields fields, int index, QObject *parent) : QAbstractTableModel(parent) {
    this->fieldIndex = index;
    this->encounterFields = fields;
    this->monInfo = info;

    this->resize(this->monInfo.wildPokemon.size(), ColumnType::Count);

    for (int r = 0; r < this->numRows; r++) {
        this->groupNames.append(QString());
        this->slotPercentages.append(0.0);
        this->slotRatios.append(fields[fieldIndex].encounterRates.value(r, 0));
    }

    if (!this->encounterFields[this->fieldIndex].groups.empty()) {
        for (auto groupKeyPair : fields[fieldIndex].groups) {
            int groupTotal = 0;
            for (int i : groupKeyPair.second) {
                this->groupNames[i] = groupKeyPair.first;
                groupTotal += this->slotRatios[i];
            }
            for (int i : groupKeyPair.second) {
                this->slotPercentages[i] = static_cast<double>(this->slotRatios[i]) / static_cast<double>(groupTotal);
            }
        }
    } else {
        int groupTotal = 0;
        for (int chance : this->encounterFields[this->fieldIndex].encounterRates) {
            groupTotal += chance;
        }
        for (int i = 0; i < this->slotPercentages.count(); i++) {
            this->slotPercentages[i] = static_cast<double>(this->slotRatios[i]) / static_cast<double>(groupTotal);
        }
    }
}

void EncounterTableModel::resize(int rows, int cols) {
    this->numRows = rows;
    this->numCols = cols;
}

int EncounterTableModel::rowCount(const QModelIndex &) const {
    return this->numRows;
}

int EncounterTableModel::columnCount(const QModelIndex &) const {
    return this->numCols;
}

QVariant EncounterTableModel::data(const QModelIndex &index, int role) const {
    int row = index.row();
    int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColumnType::Slot:
            return row;

        case ColumnType::Group:
            return this->groupNames[row];

        case ColumnType::Species:
            return this->monInfo.wildPokemon[row].species;

        case ColumnType::MinLevel:
            return this->monInfo.wildPokemon[row].minLevel;

        case ColumnType::MaxLevel:
            return this->monInfo.wildPokemon[row].maxLevel;

        case ColumnType::EncounterChance:
            return QString::number(this->slotPercentages[row] * 100.0, 'f', 2) + "%";

        case ColumnType::SlotRatio:
            return this->slotRatios[row];

        case ColumnType::EncounterRate:
            if (row == 0) {
                return this->monInfo.encounterRate;
            } else {
                return QVariant();
            }

        default:
            return QString();
        }
    }
    else if (role == Qt::EditRole) {
        switch (col) {
        case ColumnType::Species:
            return this->monInfo.wildPokemon[row].species;

        case ColumnType::MinLevel:
            return this->monInfo.wildPokemon[row].minLevel;

        case ColumnType::MaxLevel:
            return this->monInfo.wildPokemon[row].maxLevel;

        case ColumnType::EncounterRate:
            if (row == 0) {
                return this->monInfo.encounterRate;
            } else {
                return QVariant();
            }

        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant EncounterTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case ColumnType::Slot:
            return QString("Slot");
        case ColumnType::Group:
            return QString("Group");
        case ColumnType::Species:
            return QString("Species");
        case ColumnType::MinLevel:
            return QString("Min Level");
        case ColumnType::MaxLevel:
            return QString("Max Level");
        case ColumnType::EncounterChance:
            return QString("Encounter Chance");
        case ColumnType::SlotRatio:
            return QString("Slot Ratio");
        case ColumnType::EncounterRate:
            return QString("Encounter Rate");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool EncounterTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role != Qt::EditRole)
        return false;
    if (!checkIndex(index))
        return false;

    int row = index.row();
    int col = index.column();

    switch (col) {
    case ColumnType::Species: {
        QString species = value.toString();
        if (this->monInfo.wildPokemon[row].species != species) {
            this->monInfo.wildPokemon[row].species = species;
            emit edited();
        }
        break;
    }

    case ColumnType::MinLevel: {
        int minLevel = value.toInt();
        if (this->monInfo.wildPokemon[row].minLevel != minLevel) {
            this->monInfo.wildPokemon[row].minLevel = minLevel;
            this->monInfo.wildPokemon[row].maxLevel = qMax(minLevel, this->monInfo.wildPokemon[row].maxLevel);
            emit edited();
        }
        break;
    }

    case ColumnType::MaxLevel: {
        int maxLevel = value.toInt();
        if (this->monInfo.wildPokemon[row].maxLevel != maxLevel) {
            this->monInfo.wildPokemon[row].maxLevel = maxLevel;
            this->monInfo.wildPokemon[row].minLevel = qMin(maxLevel, this->monInfo.wildPokemon[row].minLevel);
            emit edited();
        }
        break;
    }

    case ColumnType::EncounterRate: {
        int encounterRate = value.toInt();
        if (this->monInfo.encounterRate != encounterRate) {
            this->monInfo.encounterRate = encounterRate;
            emit edited();
        }
        break;
    }

    default:
        return false;
    }
    return true;
}

Qt::ItemFlags EncounterTableModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags flags = Qt::NoItemFlags;
    switch (index.column()) {
        case ColumnType::Species:
        case ColumnType::MinLevel:
        case ColumnType::MaxLevel:
            flags |= Qt::ItemIsEditable;
            break;
        case ColumnType::EncounterRate:
            if (index.row() == 0) flags |= Qt::ItemIsEditable;
            break;
        default:
            break;
    }
    return flags | QAbstractTableModel::flags(index);
}
