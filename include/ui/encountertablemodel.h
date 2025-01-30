#pragma once
#ifndef ENCOUNTERTABLEMODEL_H
#define ENCOUNTERTABLEMODEL_H

#include "wildmoninfo.h"

#include <QAbstractTableModel>



class Editor;

class EncounterTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    EncounterTableModel(const WildMonInfo &monInfo, const EncounterField &field, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

public:
    enum ColumnType {
        Slot, Group, Species, MinLevel, MaxLevel, EncounterChance, SlotRatio, EncounterRate, Count
    };

    WildMonInfo encounterData() const { return m_monInfo; }
    EncounterField encounterField() const { return m_encounterField; }
    QList<double> percentages() const { return m_slotPercentages; }

private:
    int m_numRows = 0;
    int m_numCols = 0;
    WildMonInfo m_monInfo;
    EncounterField m_encounterField;
    QMap<int,QString> m_groupNames;
    QList<double> m_slotPercentages;

signals:
    void edited();
};

#endif // ENCOUNTERTABLEMODEL_H
