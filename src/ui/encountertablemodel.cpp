#include "encountertablemodel.h"
#include "editor.h"



EncounterTableModel::EncounterTableModel(WildMonInfo info, QString fieldName, QObject *parent) : QAbstractTableModel(parent) {
    this->fieldName = fieldName;
    this->monInfo = info;
    this->resize(this->monInfo.wildPokemon.size(), ColumnType::Count);
    this->updateRows();
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

        case ColumnType::Species:
            return this->monInfo.wildPokemon[row].species;

        case ColumnType::MinLevel:
            return this->monInfo.wildPokemon[row].minLevel;

        case ColumnType::MaxLevel:
            return this->monInfo.wildPokemon[row].maxLevel;

        case ColumnType::EncounterChanceDay:
            return QString::number(this->monInfo.wildPokemon[row].encounterChanceDay * (100.0 / 255.0), 'f', 2) + "%";

        case ColumnType::EncounterChanceNight:
            return QString::number(this->monInfo.wildPokemon[row].encounterChanceNight * (100.0 / 255.0), 'f', 2) + "%";

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

        case ColumnType::EncounterChanceDay:
            return this->monInfo.wildPokemon[row].encounterChanceDay * (100.0 / 255.0);

        case ColumnType::EncounterChanceNight:
            return this->monInfo.wildPokemon[row].encounterChanceNight * (100.0 / 255.0);

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
        case ColumnType::Species:
            return QString("Species");
        case ColumnType::MinLevel:
            return QString("Min Level");
        case ColumnType::MaxLevel:
            return QString("Max Level");
        case ColumnType::EncounterChanceDay:
            return QString("Encounter Chance - Day");
        case ColumnType::EncounterChanceNight:
            return QString("Encounter Chance - Night");
        case ColumnType::EncounterRate:
            return QString("Encounter Rate");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool EncounterTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::EditRole) {
        if (!checkIndex(index))
            return false;

        int row = index.row();
        int col = index.column();

        switch (col) {
        case ColumnType::Species:
            this->monInfo.wildPokemon[row].species = value.toString();
            this->updateRows();
            break;

        case ColumnType::MinLevel:
            this->monInfo.wildPokemon[row].minLevel = value.toInt();
            break;

        case ColumnType::MaxLevel:
            this->monInfo.wildPokemon[row].maxLevel = value.toInt();
            break;

        case ColumnType::EncounterChanceDay:
            this->monInfo.wildPokemon[row].encounterChanceDay = std::round(value.toDouble() * (255.0 / 100.0));
            this->updateEncounterChances(row);
            break;

        case ColumnType::EncounterChanceNight:
            this->monInfo.wildPokemon[row].encounterChanceNight = std::round(value.toDouble() * (255.0 / 100.0));
            this->updateEncounterChances(row);
            break;

        case ColumnType::EncounterRate:
            this->monInfo.encounterRate = value.toInt();
            break;

        default:
            return false;
        }
        emit edited();
        return true;
    }
    return false;
}

Qt::ItemFlags EncounterTableModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags flags = Qt::NoItemFlags;
    switch (index.column()) {
        case ColumnType::Species:
        case ColumnType::MinLevel:
        case ColumnType::MaxLevel:
        case ColumnType::EncounterChanceDay:
        case ColumnType::EncounterChanceNight:
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

WildMonInfo EncounterTableModel::encounterData() {
    return this->monInfo;
}

void EncounterTableModel::updateRows() {
    bool resized = false;
    // Remove rows besides the last which have no species
    int i = 0;
    while (i < this->monInfo.wildPokemon.size() - 1) {
        if (this->monInfo.wildPokemon[i].species.isEmpty() || this->monInfo.wildPokemon[i].species == "SPECIES_NONE") {
            this->monInfo.wildPokemon.removeAt(i);
            resized = true;
        } else {
            i++;
        }
    }

    // Add a new row if the last row has a species
    if (this->monInfo.wildPokemon.isEmpty() || !(this->monInfo.wildPokemon.last().species.isEmpty() || this->monInfo.wildPokemon.last().species == "SPECIES_NONE")) {
        this->monInfo.wildPokemon.append(WildPokemon());
        resized = true;
    }

    if (resized) {
        // Update the encounter chances
        this->updateEncounterChances(-1);
        this->resize(this->monInfo.wildPokemon.size(), ColumnType::Count);
        
        emit layoutChanged();
    }
}

void EncounterTableModel::updateEncounterChances(int updatedRow) {
    if (this->monInfo.wildPokemon.empty()) {
        return;
    }

    // Ensure that the encounter chance adds up to 255
    int totalEncounterChanceDay = 0;
    int dayRows = 0;
    int totalEncounterChanceNight = 0;
    int nightRows = 0;
    for (WildPokemon mon : this->monInfo.wildPokemon) {
        totalEncounterChanceDay += mon.encounterChanceDay;
        totalEncounterChanceNight += mon.encounterChanceNight;
        if (mon.encounterChanceDay > 0) dayRows++;
        if (mon.encounterChanceNight > 0) nightRows++;
    }

    if (totalEncounterChanceDay > 230) {
        // Multiply to get near 255
        float dayRatio = 255.0 / totalEncounterChanceDay;
        totalEncounterChanceDay = 0;
        for (int i = 0; i < this->monInfo.wildPokemon.size(); i++) {
            if (i != updatedRow) {
                this->monInfo.wildPokemon[i].encounterChanceDay = std::round(this->monInfo.wildPokemon[i].encounterChanceDay * dayRatio);
            } else {
                this->monInfo.wildPokemon[i].encounterChanceDay = std::min(this->monInfo.wildPokemon[i].encounterChanceDay, 256 - dayRows);
            }
            totalEncounterChanceDay += this->monInfo.wildPokemon[i].encounterChanceDay;
        }
        // Adjust up and down to get exactly 255
        for (int i = 0; totalEncounterChanceDay < 255; i = (i + 1) % this->monInfo.wildPokemon.size()) {
            if (this->monInfo.wildPokemon[i].encounterChanceDay > 0 && (dayRows <= 1 || i != updatedRow)) {
                this->monInfo.wildPokemon[i].encounterChanceDay++;
                totalEncounterChanceDay++;
            }
        }
        for (int i = 0; totalEncounterChanceDay > 255; i = (i + 1) % this->monInfo.wildPokemon.size()) {
            if (this->monInfo.wildPokemon[i].encounterChanceDay > 1 && (dayRows <= 1 || i != updatedRow)) {
                this->monInfo.wildPokemon[i].encounterChanceDay--;
                totalEncounterChanceDay--;
            }
        }
    }

    if (totalEncounterChanceNight > 230) {
        // Multiply to get near 255
        float nightRatio = 255.0 / totalEncounterChanceNight;
        totalEncounterChanceNight = 0;
        for (int i = 0; i < this->monInfo.wildPokemon.size(); i++) {
            if (nightRows <= 1 || i != updatedRow) {
                this->monInfo.wildPokemon[i].encounterChanceNight = std::round(this->monInfo.wildPokemon[i].encounterChanceNight * nightRatio);
            } else {
                this->monInfo.wildPokemon[i].encounterChanceNight = std::min(this->monInfo.wildPokemon[i].encounterChanceNight, 256 - nightRows);
            }
            totalEncounterChanceNight += this->monInfo.wildPokemon[i].encounterChanceNight;
        }
        // Adjust up and down to get exactly 255
        for (int i = 0; totalEncounterChanceNight < 255; i = (i + 1) % this->monInfo.wildPokemon.size()) {
            if (this->monInfo.wildPokemon[i].encounterChanceNight > 0 && (nightRows <= 1 || i != updatedRow)) {
                this->monInfo.wildPokemon[i].encounterChanceNight++;
                totalEncounterChanceNight++;
            }
        }
        for (int i = 0; totalEncounterChanceNight > 255; i = (i + 1) % this->monInfo.wildPokemon.size()) {
            if (this->monInfo.wildPokemon[i].encounterChanceNight > 1 && (nightRows <= 1 || i != updatedRow)) {
                this->monInfo.wildPokemon[i].encounterChanceNight--;
                totalEncounterChanceNight--;
            }
        }
    }
}
