#pragma once
#ifndef ENCOUNTERTABLEMODEL_H
#define ENCOUNTERTABLEMODEL_H

#include "wildmoninfo.h"

#include <QAbstractTableModel>



class Editor;

class EncounterTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    EncounterTableModel(WildMonInfo monInfo, QString fieldName, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

public:
    enum ColumnType {
        Slot, Species, MinLevel, MaxLevel, EncounterChanceDay, EncounterChanceNight, EncounterRate, Count
    };

    WildMonInfo encounterData();
    void resize(int rows, int cols);

private:
    WildMonInfo monInfo;
    QVector<QString> encounterFieldTypes;
    QString fieldName;

    int numRows = 0;
    int numCols = 0;

    void updateEncounterChances(int updatedRow);
    void updateRows();

signals:
    void edited();
};

#endif // ENCOUNTERTABLEMODEL_H
