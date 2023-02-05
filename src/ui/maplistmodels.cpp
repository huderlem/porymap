#include "maplistmodels.h"

#include "project.h"



/*

    // QIcon mapFolderIcon;
    // mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    // mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);

    // QIcon folderIcon;
    // folderIcon.addFile(QStringLiteral(":/icons/folder_closed.ico"), QSize(), QIcon::Normal, QIcon::Off);
    // //folderIcon.addFile(QStringLiteral(":/icons/folder.ico"), QSize(), QIcon::Normal, QIcon::On);

    // ui->mapList->setUpdatesEnabled(false);
    // mapListModel->clear();
    // mapGroupItemsList->clear();
    // QStandardItem *root = mapListModel->invisibleRootItem();

    // switch (mapSortOrder)
    // {
    //     case MapSortOrder::Group:
    //         for (int i = 0; i < project->groupNames.length(); i++) {
    //             QString group_name = project->groupNames.value(i);
    //             QStandardItem *group = new QStandardItem;
    //             group->setText(group_name);
    //             group->setIcon(mapFolderIcon);
    //             group->setEditable(false);
    //             group->setData(group_name, Qt::UserRole);
    //             group->setData("map_group", MapListUserRoles::TypeRole);
    //             group->setData(i, MapListUserRoles::GroupRole);
    //             root->appendRow(group);
    //             mapGroupItemsList->append(group);
    //             QStringList names = project->groupedMapNames.value(i);
    //             for (int j = 0; j < names.length(); j++) {
    //                 QString map_name = names.value(j);
    //                 QStandardItem *map = createMapItem(map_name, i, j);
    //                 group->appendRow(map);
    //                 mapListIndexes.insert(map_name, map->index());
    //             }
    //         }
    //         break;

    // mapListModel = new QStandardItemModel;
    // mapGroupItemsList = new QList<QStandardItem*>;
    // mapListProxyModel = new FilterChildrenProxyModel;

    // mapListProxyModel->setSourceModel(mapListModel);
    // ui->mapList->setModel(mapListProxyModel);
    
    // createMapItem:
    // QStandardItem *map = new QStandardItem;
    // map->setText(QString("[%1.%2] ").arg(groupNum).arg(inGroupNum, 2, 10, QLatin1Char('0')) + mapName);
    // map->setIcon(*mapIcon);
    // map->setEditable(false);
    // map->setData(mapName, Qt::UserRole);
    // map->setData("map_name", MapListUserRoles::TypeRole);
    // return map;

    // scrolling:
    if (scrollTreeView) {
        // Make sure we clear the filter first so we actually have a scroll target
        /// !TODO
        // mapListProxyModel->setFilterRegularExpression(QString());
        // ui->mapList->setCurrentIndex(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)));
        // ui->mapList->scrollTo(ui->mapList->currentIndex(), QAbstractItemView::PositionAtCenter);
    }

    // ui->mapList->setExpanded(mapListProxyModel->mapFromSource(mapListIndexes.value(map_name)), true);

*/
MapGroupModel::MapGroupModel(Project *project, QObject *parent) : QStandardItemModel(parent) {
    //

    this->project = project;
    this->root = this->invisibleRootItem();

    // mapIcon = new QIcon(QStringLiteral(":/icons/map.ico"));
    // mapEditedIcon = new QIcon(QStringLiteral(":/icons/map_edited.ico"));
    // mapOpenedIcon = new QIcon(QStringLiteral(":/icons/map_opened.ico"));

    // mapFolderIcon = new QIcon(QStringLiteral(":/icons/folder_closed_map.ico"));

    //mapFolderIcon = new QIcon;
    //mapFolderIcon->addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
    //mapFolderIcon->addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);

    initialize();
}

QStandardItem *MapGroupModel::createGroupItem(QString groupName, int groupIndex) {
    QStandardItem *group = new QStandardItem;
    group->setText(groupName);
    group->setEditable(true);
    group->setData(groupName, Qt::UserRole);
    group->setData("map_group", MapListRoles::TypeRole);
    group->setData(groupIndex, MapListRoles::GroupRole);
    // group->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->groupItems.insert(groupName, group);
    return group;
}

QStandardItem *MapGroupModel::createMapItem(QString mapName, int groupIndex, int mapIndex) {
    QStandardItem *map = new QStandardItem;
    map->setText(QString("[%1.%2] ").arg(groupIndex).arg(mapIndex, 2, 10, QLatin1Char('0')) + mapName);
    map->setEditable(false);
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListRoles::TypeRole);
    // map->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->mapItems.insert(mapName, map);
    return map;
}

void MapGroupModel::initialize() {
    for (int i = 0; i < this->project->groupNames.length(); i++) {
        QString group_name = this->project->groupNames.value(i);
        QStandardItem *group = createGroupItem(group_name, i);
        root->appendRow(group);
        QList<QStandardItem *> groupItems;
        QMap<QString, QStandardItem *> inGroupItems;
        //mapGroupItemsList->append(group);
        QStringList names = this->project->groupedMapNames.value(i);
        for (int j = 0; j < names.length(); j++) {
            QString map_name = names.value(j);
            QStandardItem *map = createMapItem(map_name, i, j);
            group->appendRow(map);
        }
    }
}

QStandardItem *MapGroupModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        QStandardItem *item = static_cast<QStandardItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return this->root;
}

QModelIndex MapGroupModel::indexOfMap(QString mapName) {
    if (this->mapItems.contains(mapName)) {
        return this->mapItems[mapName]->index();
    }
    return QModelIndex();
}

    // projectHasUnsavedChanges = false;
    // QList<QModelIndex> list;
    // list.append(QModelIndex());
    // while (list.length()) {
    //     QModelIndex parent = list.takeFirst();
    //     for (int i = 0; i < model->rowCount(parent); i++) {
    //         QModelIndex index = model->index(i, 0, parent);
    //         if (model->hasChildren(index)) {
    //             list.append(index);
    //         }
    //         QVariant data = index.data(Qt::UserRole);
    //         if (!data.isNull()) {
    //             QString map_name = data.toString();
    //             if (editor->project && editor->project->mapCache.contains(map_name)) {
    //                 QStandardItem *map = mapListModel->itemFromIndex(mapListIndexes.value(map_name));
    //                 map->setIcon(*mapIcon);
    //                 if (editor->project->mapCache.value(map_name)->hasUnsavedChanges()) {
    //                     map->setIcon(*mapEditedIcon);
    //                     projectHasUnsavedChanges = true;
    //                 }
    //                 if (editor->map->name == map_name) {
    //                     map->setIcon(*mapOpenedIcon);
    //                 }
    //             }
    //         }
    //     }
    // }

#include <QDebug>
QVariant MapGroupModel::data(const QModelIndex &index, int role) const {
    int row = index.row();
    int col = index.column();

    if (role == Qt::DecorationRole) {
        static QIcon mapIcon = QIcon(QStringLiteral(":/icons/map.ico"));
        static QIcon mapEditedIcon = QIcon(QStringLiteral(":/icons/map_edited.ico"));
        static QIcon mapOpenedIcon = QIcon(QStringLiteral(":/icons/map_opened.ico"));

        static QIcon mapFolderIcon;
        static bool loaded = false;
        if (!loaded) {
            mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
            mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);
            loaded = true;
        }

        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListRoles::TypeRole).toString();

        if (type == "map_group") {
            return mapFolderIcon;
        } else if (type == "map_name") {
            QString mapName = item->data(Qt::UserRole).toString();
            if (mapName == this->openMap) {
                return mapOpenedIcon;
            }
            else if (this->project->mapCache.contains(mapName)) {
                if (this->project->mapCache.value(mapName)->hasUnsavedChanges()) {
                    return mapEditedIcon;
                }
            }
            return mapIcon;
        }

        // check if map or group
        // if map, check if edited or open
        //return QIcon(":/icons/porymap-icon-2.ico");
    }

    return QStandardItemModel::data(index, role);
}














    //     case MapSortOrder::Layout:
    //     {
    //         QMap<QString, int> layoutIndices;
    //         for (int i = 0; i < project->mapLayoutsTable.length(); i++) {
    //             QString layoutId = project->mapLayoutsTable.value(i);
    //             MapLayout *layout = project->mapLayouts.value(layoutId);
    //             QStandardItem *layoutItem = new QStandardItem;
    //             layoutItem->setText(layout->name);
    //             layoutItem->setIcon(folderIcon);
    //             layoutItem->setEditable(false);
    //             layoutItem->setData(layout->name, Qt::UserRole);
    //             layoutItem->setData("map_layout", MapListUserRoles::TypeRole);
    //             layoutItem->setData(layout->id, MapListUserRoles::TypeRole2);
    //             layoutItem->setData(i, MapListUserRoles::GroupRole);
    //             root->appendRow(layoutItem);
    //             mapGroupItemsList->append(layoutItem);
    //             layoutIndices[layoutId] = i;
    //         }
    //         for (int i = 0; i < project->groupNames.length(); i++) {
    //             QStringList names = project->groupedMapNames.value(i);
    //             for (int j = 0; j < names.length(); j++) {
    //                 QString map_name = names.value(j);
    //                 QStandardItem *map = createMapItem(map_name, i, j);
    //                 QString layoutId = project->readMapLayoutId(map_name);
    //                 QStandardItem *layoutItem = mapGroupItemsList->at(layoutIndices.value(layoutId));
    //                 layoutItem->setIcon(mapFolderIcon);
    //                 layoutItem->appendRow(map);
    //                 mapListIndexes.insert(map_name, map->index());
    //             }
    //         }
    //         break;
    //     }
LayoutTreeModel::LayoutTreeModel(Project *project, QObject *parent) : QStandardItemModel(parent) {
    //

    this->project = project;
    this->root = this->invisibleRootItem();

    initialize();
}

QStandardItem *LayoutTreeModel::createLayoutItem(QString layoutId) {
    QStandardItem *layout = new QStandardItem;
    layout->setText(this->project->layoutIdsToNames[layoutId]);
    //layout->setText(layoutId);
    layout->setEditable(false);
    layout->setData(layoutId, Qt::UserRole);
    layout->setData("map_layout", MapListRoles::TypeRole);
    // // group->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->layoutItems.insert(layoutId, layout);
    return layout;
}

QStandardItem *LayoutTreeModel::createMapItem(QString mapName) {
    QStandardItem *map = new QStandardItem;
    map->setText(mapName);
    map->setEditable(false);
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListRoles::TypeRole);
    map->setFlags(Qt::NoItemFlags | Qt::ItemNeverHasChildren);
    this->mapItems.insert(mapName, map);
    return map;
}

void LayoutTreeModel::initialize() {
    for (int i = 0; i < this->project->mapLayoutsTable.length(); i++) {
        //
        QString layoutId = project->mapLayoutsTable.value(i);
        QStandardItem *layoutItem = createLayoutItem(layoutId);
        this->root->appendRow(layoutItem);
    }

    for (auto mapList : this->project->groupedMapNames) {
        for (auto mapName : mapList) {
            //
            QString layoutId = project->readMapLayoutId(mapName);
            QStandardItem *map = createMapItem(mapName);
            this->layoutItems[layoutId]->appendRow(map);
        }
    }

    // // project->readMapLayoutName
}

QStandardItem *LayoutTreeModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        QStandardItem *item = static_cast<QStandardItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return this->root;
}

QModelIndex LayoutTreeModel::indexOfLayout(QString layoutName) {
    if (this->layoutItems.contains(layoutName)) {
        return this->layoutItems[layoutName]->index();
    }
    return QModelIndex();
}

QVariant LayoutTreeModel::data(const QModelIndex &index, int role) const {
    int row = index.row();
    int col = index.column();

    if (role == Qt::DecorationRole) {
        static QIcon mapIcon = QIcon(QStringLiteral(":/icons/map.ico"));
        static QIcon mapEditedIcon = QIcon(QStringLiteral(":/icons/map_edited.ico"));
        static QIcon mapOpenedIcon = QIcon(QStringLiteral(":/icons/map_opened.ico"));

        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListRoles::TypeRole).toString();

        if (type == "map_layout") {
            QString layoutId = item->data(Qt::UserRole).toString();
            if (layoutId == this->openLayout) {
                return mapOpenedIcon;
            }
            else if (this->project->mapLayouts.contains(layoutId)) {
                if (this->project->mapLayouts.value(layoutId)->hasUnsavedChanges()) {
                    return mapEditedIcon;
                }
            }
            return mapIcon;
        }
        else if (type == "map_name") {
            return QVariant();
        }

        return QVariant();

        // check if map or group
        // if map, check if edited or open
        //return QIcon(":/icons/porymap-icon-2.ico");
    }

    return QStandardItemModel::data(index, role);
}

