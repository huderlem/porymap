#pragma once
#ifndef MAPLISTMODELS_H
#define MAPLISTMODELS_H

#include <QStandardItemModel>
#include <QMap>



class Project;

enum MapListRoles {
    GroupRole = Qt::UserRole + 1, // Used to hold the map group number.
    TypeRole,  // Used to differentiate between the different layers of the map list tree view.
    TypeRole2, // Used for various extra data needed.
};

// or QStandardItemModel??
class MapGroupModel : public QStandardItemModel {
    Q_OBJECT

public:
    MapGroupModel(Project *project, QObject *parent = nullptr);
    ~MapGroupModel() {}

    QVariant data(const QModelIndex &index, int role) const override;

public:
    void setMap(QString mapName) { this->openMap = mapName; }

    QStandardItem *createGroupItem(QString groupName, int groupIndex);
    QStandardItem *createMapItem(QString mapName, int groupIndex, int mapIndex);

    QStandardItem *getItem(const QModelIndex &index) const;
    QModelIndex indexOfMap(QString mapName);

    void initialize();

private:
    Project *project;
    QStandardItem *root = nullptr;

    QMap<QString, QStandardItem *> groupItems;
    QMap<QString, QStandardItem *> mapItems;
    // TODO: if reordering, will the item be the same?

    QString openMap;

signals:
    void edited();
};



class LayoutTreeModel : public QStandardItemModel {
    Q_OBJECT

public:
    LayoutTreeModel(Project *project, QObject *parent = nullptr);
    ~LayoutTreeModel() {}

    QVariant data(const QModelIndex &index, int role) const override;

public:
    void setLayout(QString layoutName) { this->openLayout = layoutName; }

    QStandardItem *createLayoutItem(QString layoutName);
    QStandardItem *createMapItem(QString mapName);

    QStandardItem *getItem(const QModelIndex &index) const;
    QModelIndex indexOfLayout(QString layoutName);

    void initialize();

private:
    Project *project;
    QStandardItem *root = nullptr;

    QMap<QString, QStandardItem *> layoutItems;
    QMap<QString, QStandardItem *> mapItems;

    QString openLayout;

signals:
    void edited();
};

#endif // MAPLISTMODELS_H
