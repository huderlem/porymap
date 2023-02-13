#pragma once
#ifndef MAPLISTMODELS_H
#define MAPLISTMODELS_H

#include <QTreeView>
#include <QFontDatabase>
#include <QStandardItemModel>
#include <QMap>



class Project;

enum MapListRoles {
    GroupRole = Qt::UserRole + 1, // Used to hold the map group number.
    TypeRole,  // Used to differentiate between the different layers of the map list tree view.
    TypeRole2, // Used for various extra data needed.
};



class MapTree : public QTreeView {
    Q_OBJECT
public:
    MapTree(QWidget *parent) : QTreeView(parent) {
        this->setDropIndicatorShown(true);
        this->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    }

public slots:
    void removeSelected();
};



class MapGroupModel : public QStandardItemModel {
    Q_OBJECT

public:
    MapGroupModel(Project *project, QObject *parent = nullptr);
    ~MapGroupModel() {}

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

public:
    void setMap(QString mapName) { this->openMap = mapName; }

    QStandardItem *createGroupItem(QString groupName, int groupIndex);
    QStandardItem *createMapItem(QString mapName, QStandardItem *fromItem = nullptr);

    QStandardItem *insertMapItem(QString mapName, QString groupName);

    QStandardItem *getItem(const QModelIndex &index) const;
    QModelIndex indexOfMap(QString mapName);

    void initialize();

private:
    void updateProject();

private:
    Project *project;
    QStandardItem *root = nullptr;

    QMap<QString, QStandardItem *> groupItems;
    QMap<QString, QStandardItem *> mapItems;
    // TODO: if reordering, will the item be the same?

    QString openMap;

signals:
    void dragMoveCompleted();
};



class MapAreaModel : public QStandardItemModel {
    Q_OBJECT

public:
    MapAreaModel(Project *project, QObject *parent = nullptr);
    ~MapAreaModel() {}

    QVariant data(const QModelIndex &index, int role) const override;

public:
    void setMap(QString mapName) { this->openMap = mapName; }

    QStandardItem *createAreaItem(QString areaName, int areaIndex);
    QStandardItem *createMapItem(QString mapName, int areaIndex, int mapIndex);

    QStandardItem *insertMapItem(QString mapName, QString areaName, int groupIndex);

    QStandardItem *getItem(const QModelIndex &index) const;
    QModelIndex indexOfMap(QString mapName);

    void initialize();

private:
    Project *project;
    QStandardItem *root = nullptr;

    QMap<QString, QStandardItem *> areaItems;
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
    void setLayout(QString layoutId) { this->openLayout = layoutId; }

    QStandardItem *createLayoutItem(QString layoutId);
    QStandardItem *createMapItem(QString mapName);

    QStandardItem *insertMapItem(QString mapName, QString layoutId);

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
