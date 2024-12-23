#pragma once
#ifndef MAPLISTMODELS_H
#define MAPLISTMODELS_H

#include <QTreeView>
#include <QFontDatabase>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QMap>



class Project;

enum MapListUserRoles {
    NameRole = Qt::UserRole, // Holds the name of the item in the list
    TypeRole, // Used to differentiate between the different layers of the map list tree view.
};



class MapTree : public QTreeView {
    Q_OBJECT
public:
    MapTree(QWidget *parent) : QTreeView(parent) {
        this->setDropIndicatorShown(true);
        this->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        this->setFocusPolicy(Qt::StrongFocus);
        this->setContextMenuPolicy(Qt::CustomContextMenu);
    }

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;

public slots:
    void removeSelected();
};



class GroupNameDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    GroupNameDelegate(Project *project, QObject *parent = nullptr) : QStyledItemDelegate(parent), project(project) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    Project *project = nullptr;
};


class MapListModel : public QStandardItemModel {
    Q_OBJECT

public:
    MapListModel(Project *project, QObject *parent = nullptr);
    ~MapListModel() { }

    void setActiveItem(const QString &itemName) { this->activeItemName = itemName; }

    virtual QStandardItem *insertMapItem(const QString &mapName, const QString &folderName);
    virtual QStandardItem *insertMapFolderItem(const QString &folderName);

    virtual QModelIndex indexOf(const QString &itemName) const;
    virtual void removeItemAt(const QModelIndex &index);
    virtual QStandardItem *getItem(const QModelIndex &index) const;

    virtual QVariant data(const QModelIndex &index, int role) const override;

protected:
    Project *project;
    QStandardItem *root = nullptr;

    QString activeItemName;
    QString folderTypeName;
    bool sortingEnabled = false;
    bool editable = false;

    QIcon mapGrayIcon;
    QIcon mapIcon;
    QIcon mapEditedIcon;
    QIcon mapOpenedIcon;
    QIcon mapFolderIcon;
    QIcon emptyMapFolderIcon;

    QMap<QString, QStandardItem *> mapFolderItems;
    QMap<QString, QStandardItem *> mapItems;

    virtual QStandardItem *createMapItem(const QString &mapName, QStandardItem *map = nullptr);
    virtual QStandardItem *createMapFolderItem(const QString &groupName, QStandardItem *fromItem = nullptr);
    virtual void removeItem(QStandardItem *item) = 0;   
};

class MapGroupModel : public MapListModel {
    Q_OBJECT

public:
    MapGroupModel(Project *project, QObject *parent = nullptr);
    ~MapGroupModel() { }

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

protected:
    void removeItem(QStandardItem *item) override;

private:
    friend class MapTree;
    void updateProject();

signals:
    void dragMoveCompleted();
};



class MapAreaModel : public MapListModel {
    Q_OBJECT

public:
    MapAreaModel(Project *project, QObject *parent = nullptr);
    ~MapAreaModel() {}

protected:
    void removeItem(QStandardItem *item) override;
};



class LayoutTreeModel : public MapListModel {
    Q_OBJECT

public:
    LayoutTreeModel(Project *project, QObject *parent = nullptr);
    ~LayoutTreeModel() {}

    QVariant data(const QModelIndex &index, int role) const override;
    QStandardItem *createMapFolderItem(const QString &folderName, QStandardItem *folder) override;

protected:
    void removeItem(QStandardItem *item) override;
};

#endif // MAPLISTMODELS_H
