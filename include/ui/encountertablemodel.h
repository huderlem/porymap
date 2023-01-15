#pragma once
#ifndef ENCOUNTERTABLEMODEL_H
#define ENCOUNTERTABLEMODEL_H

#include "wildmoninfo.h"

#include <QAbstractTableModel>



class Editor;

class EncounterTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    EncounterTableModel(WildMonInfo monInfo, EncounterFields allFields, int fieldIndex, QObject *parent = nullptr);

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

    WildMonInfo encounterData();
    void resize(int rows, int cols);

private:
    WildMonInfo monInfo;
    EncounterFields encounterFields;
    int fieldIndex;

    int numRows = 0;
    int numCols = 0;

    QList<int> slotRatios;
    QList<QString> groupNames;
    QList<double> slotPercentages;

signals:
    void edited();
};

#endif // ENCOUNTERTABLEMODEL_H
