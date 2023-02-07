#include "maplistmodels.h"

#include "project.h"



MapGroupModel::MapGroupModel(Project *project, QObject *parent) : QStandardItemModel(parent) {
    this->project = project;
    this->root = this->invisibleRootItem();

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

QVariant MapGroupModel::data(const QModelIndex &index, int role) const {
    int row = index.row();
    int col = index.column();

    if (role == Qt::DecorationRole) {
        static QIcon mapGrayIcon = QIcon(QStringLiteral(":/icons/map_grayed.ico"));
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
                else {
                    return mapIcon;
                }
            }
            return mapGrayIcon;
        }
    }

    return QStandardItemModel::data(index, role);
}



MapAreaModel::MapAreaModel(Project *project, QObject *parent) : QStandardItemModel(parent) {
    this->project = project;
    this->root = this->invisibleRootItem();

    initialize();
}

QStandardItem *MapAreaModel::createAreaItem(QString mapsecName, int areaIndex) {
    QStandardItem *area = new QStandardItem;
    area->setText(mapsecName);
    area->setEditable(false);
    area->setData(mapsecName, Qt::UserRole);
    area->setData("map_section", MapListRoles::TypeRole);
    area->setData(areaIndex, MapListRoles::GroupRole);
    // group->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->areaItems.insert(mapsecName, area);
    return area;
}

QStandardItem *MapAreaModel::createMapItem(QString mapName, int groupIndex, int mapIndex) {
    QStandardItem *map = new QStandardItem;
    map->setText(QString("[%1.%2] ").arg(groupIndex).arg(mapIndex, 2, 10, QLatin1Char('0')) + mapName);
    map->setEditable(false);
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListRoles::TypeRole);
    // map->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->mapItems.insert(mapName, map);
    return map;
}

void MapAreaModel::initialize() {
    for (int i = 0; i < this->project->mapSectionNameToValue.size(); i++) {
        QString mapsecName = project->mapSectionValueToName.value(i);
        QStandardItem *areaItem = createAreaItem(mapsecName, i);
        this->root->appendRow(areaItem);
    }

    for (int i = 0; i < this->project->groupNames.length(); i++) {
        QStringList names = this->project->groupedMapNames.value(i);
        for (int j = 0; j < names.length(); j++) {
            QString mapName = names.value(j);
            QStandardItem *map = createMapItem(mapName, i, j);
            QString mapsecName = this->project->readMapLocation(mapName);
            if (this->areaItems.contains(mapsecName)) {
                this->areaItems[mapsecName]->appendRow(map);
            }
        }
    }
}

QStandardItem *MapAreaModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        QStandardItem *item = static_cast<QStandardItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return this->root;
}

QModelIndex MapAreaModel::indexOfMap(QString mapName) {
    if (this->mapItems.contains(mapName)) {
        return this->mapItems[mapName]->index();
    }
    return QModelIndex();
}

QVariant MapAreaModel::data(const QModelIndex &index, int role) const {
    int row = index.row();
    int col = index.column();

    if (role == Qt::DecorationRole) {
        static QIcon mapGrayIcon = QIcon(QStringLiteral(":/icons/map_grayed.ico"));
        static QIcon mapIcon = QIcon(QStringLiteral(":/icons/map.ico"));
        static QIcon mapEditedIcon = QIcon(QStringLiteral(":/icons/map_edited.ico"));
        static QIcon mapOpenedIcon = QIcon(QStringLiteral(":/icons/map_opened.ico"));

        static QIcon mapFolderIcon;
        static QIcon folderIcon;
        static bool loaded = false;
        if (!loaded) {
            mapFolderIcon.addFile(QStringLiteral(":/icons/folder_closed_map.ico"), QSize(), QIcon::Normal, QIcon::Off);
            mapFolderIcon.addFile(QStringLiteral(":/icons/folder_map.ico"), QSize(), QIcon::Normal, QIcon::On);
            folderIcon.addFile(QStringLiteral(":/icons/folder_closed.ico"), QSize(), QIcon::Normal, QIcon::Off);
            folderIcon.addFile(QStringLiteral(":/icons/folder.ico"), QSize(), QIcon::Normal, QIcon::On);
            loaded = true;
        }

        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListRoles::TypeRole).toString();

        if (type == "map_section") {
            if (item->hasChildren()) {
                return mapFolderIcon;
            }
            return folderIcon;
        } else if (type == "map_name") {
            QString mapName = item->data(Qt::UserRole).toString();
            if (mapName == this->openMap) {
                return mapOpenedIcon;
            }
            else if (this->project->mapCache.contains(mapName)) {
                if (this->project->mapCache.value(mapName)->hasUnsavedChanges()) {
                    return mapEditedIcon;
                }
                else {
                    return mapIcon;
                }
            }
            return mapGrayIcon;
        }
    }

    return QStandardItemModel::data(index, role);
}



LayoutTreeModel::LayoutTreeModel(Project *project, QObject *parent) : QStandardItemModel(parent) {
    this->project = project;
    this->root = this->invisibleRootItem();

    initialize();
}

QStandardItem *LayoutTreeModel::createLayoutItem(QString layoutId) {
    QStandardItem *layout = new QStandardItem;
    layout->setText(this->project->layoutIdsToNames[layoutId]);
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
    }

    return QStandardItemModel::data(index, role);
}
