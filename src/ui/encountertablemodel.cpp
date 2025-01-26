#include "encountertablemodel.h"
#include "editor.h"



EncounterTableModel::EncounterTableModel(const WildMonInfo &info, const EncounterField &field, QObject *parent)
    : QAbstractTableModel(parent),
      m_numRows(info.wildPokemon.size()),
      m_numCols(ColumnType::Count),
      m_monInfo(info),
      m_encounterField(field),
      m_slotPercentages(getWildEncounterPercentages(field))
{
    for (const auto &groupKeyPair : m_encounterField.groups) {
        for (const auto &slot : groupKeyPair.second) {
            m_groupNames.insert(slot, groupKeyPair.first);
        }
    }
}

int EncounterTableModel::rowCount(const QModelIndex &) const {
    return m_numRows;
}

int EncounterTableModel::columnCount(const QModelIndex &) const {
    return m_numCols;
}

QVariant EncounterTableModel::data(const QModelIndex &index, int role) const {
    int row = index.row();
    int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
        case ColumnType::Slot:
            return row;

        case ColumnType::Group:
            return m_groupNames.value(row);

        case ColumnType::Species:
            return m_monInfo.wildPokemon.value(row).species;

        case ColumnType::MinLevel:
            return m_monInfo.wildPokemon.value(row).minLevel;

        case ColumnType::MaxLevel:
            return m_monInfo.wildPokemon.value(row).maxLevel;

        case ColumnType::EncounterChance:
            return QString::number(m_slotPercentages.value(row, 0) * 100, 'f', 2) + "%";

        case ColumnType::SlotRatio:
            return m_encounterField.encounterRates.value(row);

        case ColumnType::EncounterRate:
            if (row == 0) {
                return m_monInfo.encounterRate;
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
            return m_monInfo.wildPokemon.value(row).species;

        case ColumnType::MinLevel:
            return m_monInfo.wildPokemon.value(row).minLevel;

        case ColumnType::MaxLevel:
            return m_monInfo.wildPokemon.value(row).maxLevel;

        case ColumnType::EncounterRate:
            if (row == 0) {
                return m_monInfo.encounterRate;
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
    auto wildMon = &m_monInfo.wildPokemon[row];

    switch (col) {
    case ColumnType::Species: {
        QString species = value.toString();
        if (wildMon->species != species) {
            wildMon->species = species;
            emit edited();
        }
        break;
    }

    case ColumnType::MinLevel: {
        int minLevel = value.toInt();
        if (wildMon->minLevel != minLevel) {
            wildMon->minLevel = minLevel;
            wildMon->maxLevel = qMax(minLevel, wildMon->maxLevel);
            emit edited();
        }
        break;
    }

    case ColumnType::MaxLevel: {
        int maxLevel = value.toInt();
        if (wildMon->maxLevel != maxLevel) {
            wildMon->maxLevel = maxLevel;
            wildMon->minLevel = qMin(maxLevel, wildMon->minLevel);
            emit edited();
        }
        break;
    }

    case ColumnType::EncounterRate: {
        int encounterRate = value.toInt();
        if (m_monInfo.encounterRate != encounterRate) {
            m_monInfo.encounterRate = encounterRate;
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
