#include "maplistmodels.h"

#include <QMouseEvent>
#include <QLineEdit>

#include "project.h"
#include "filterchildrenproxymodel.h"



void MapTree::removeSelected() {
    while (!this->selectedIndexes().isEmpty()) {
        QModelIndex i = this->selectedIndexes().takeLast();
        this->model()->removeRow(i.row(), i.parent());
    }
}



QWidget *GroupNameDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    QLineEdit *editor = new QLineEdit(parent);
    static const QRegularExpression expression("gMapGroup_[A-Za-z0-9_]+");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression, parent);
    editor->setValidator(validator);
    editor->setFrame(false);
    return editor;
}

void GroupNameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    QString groupName = index.data(Qt::UserRole).toString();
    QLineEdit *le = static_cast<QLineEdit *>(editor);
    le->setText(groupName);
}

void GroupNameDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QLineEdit *le = static_cast<QLineEdit *>(editor);
    QString groupName = le->text();
    model->setData(index, groupName, Qt::UserRole);
}

void GroupNameDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
    editor->setGeometry(option.rect);
}



MapGroupModel::MapGroupModel(Project *project, QObject *parent) : QStandardItemModel(parent) {
    this->project = project;
    this->root = this->invisibleRootItem();

    initialize();
}

Qt::DropActions MapGroupModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QStringList MapGroupModel::mimeTypes() const {
    QStringList types;
    types << "application/porymap.mapgroupmodel.map"
          << "application/porymap.mapgroupmodel.group";
    return types;
}

QMimeData *MapGroupModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData *mimeData = QStandardItemModel::mimeData(indexes);
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            QString mapName = data(index, Qt::UserRole).toString();
            stream << mapName;
        }
    }

    mimeData->setData("application/porymap.mapgroupmodel.map", encodedData);
    return mimeData;
}

bool MapGroupModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parentIndex) {
    if (action == Qt::IgnoreAction)
        return true;
 
    if (!data->hasFormat("application/porymap.mapgroupmodel.map"))
        return false;

    if (!parentIndex.isValid())
        return false;

    int firstRow = 0;
 
    if (row != -1) {
        firstRow = row;
    }
    else if (parentIndex.isValid()) {
        firstRow = rowCount(parentIndex);
    }

    QByteArray encodedData = data->data("application/porymap.mapgroupmodel.map");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList droppedMaps;
    int rowCount = 0;

    QList<QStandardItem *> newItems;

    while (!stream.atEnd()) {
        QString mapName;
        stream >> mapName;
        droppedMaps << mapName;
        rowCount++;
    }

    this->insertRows(firstRow, rowCount, parentIndex);

    int newItemIndex = 0;
    for (QString mapName : droppedMaps) {
        QModelIndex mapIndex = index(firstRow, 0, parentIndex);
        QStandardItem *mapItem = this->itemFromIndex(mapIndex);
        createMapItem(mapName, mapItem);
        firstRow++;
    }

    emit dragMoveCompleted();
    updateProject();

    return false;
}

void MapGroupModel::updateProject() {
    if (!this->project) return;

    QStringList groupNames;
    QMap<QString, int> mapGroups;
    QList<QStringList> groupedMapNames;
    QStringList mapNames;

    for (int g = 0; g < this->root->rowCount(); g++) {
        QStandardItem *groupItem = this->item(g);
        QString groupName = groupItem->data(Qt::UserRole).toString();
        groupNames.append(groupName);
        mapGroups[groupName] = g;
        QStringList mapsInGroup;
        for (int m = 0; m < groupItem->rowCount(); m++) {
            QStandardItem *mapItem = groupItem->child(m);
            QString mapName = mapItem->data(Qt::UserRole).toString();
            mapsInGroup.append(mapName);
            mapNames.append(mapName);
        }
        groupedMapNames.append(mapsInGroup);
    }

    this->project->groupNames = groupNames;
    this->project->mapGroups = mapGroups;
    this->project->groupedMapNames = groupedMapNames;
    this->project->mapNames = mapNames;
}

QStandardItem *MapGroupModel::createGroupItem(QString groupName, int groupIndex) {
    QStandardItem *group = new QStandardItem;
    group->setText(groupName);
    group->setData(groupName, Qt::UserRole);
    group->setData("map_group", MapListRoles::TypeRole);
    group->setData(groupIndex, MapListRoles::GroupRole);
    group->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable);
    this->groupItems.insert(groupName, group);
    return group;
}

QStandardItem *MapGroupModel::createMapItem(QString mapName, QStandardItem *map) {
    if (!map) map = new QStandardItem;
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListRoles::TypeRole);
    map->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->mapItems[mapName] = map;
    return map;
}

QStandardItem *MapGroupModel::insertMapItem(QString mapName, QString groupName) {
    QStandardItem *group = this->groupItems[groupName];
    if (!group) {
        return nullptr;
    }
    QStandardItem *map = createMapItem(mapName);
    group->appendRow(map);
    return map;
}

void MapGroupModel::initialize() {
    this->groupItems.clear();
    this->mapItems.clear();
    for (int i = 0; i < this->project->groupNames.length(); i++) {
        QString group_name = this->project->groupNames.value(i);
        QStandardItem *group = createGroupItem(group_name, i);
        root->appendRow(group);
        QStringList names = this->project->groupedMapNames.value(i);
        for (int j = 0; j < names.length(); j++) {
            QString map_name = names.value(j);
            QStandardItem *map = createMapItem(map_name);
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
    if (!index.isValid()) return QVariant();

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

        if (type == "map_group") {
            if (!item->hasChildren()) {
                return folderIcon;
            }
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
    else if (role == Qt::DisplayRole) {
        //
        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListRoles::TypeRole).toString();

        if (type == "map_name") {
            return QString("[%1.%2] ").arg(this->getItem(index)->row()).arg(row, 2, 10, QLatin1Char('0')) + item->data(Qt::UserRole).toString();
        }
        else if (type == "map_group") {
            return item->data(Qt::UserRole).toString();
        }
    }

    return QStandardItemModel::data(index, role);
}

bool MapGroupModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::UserRole && data(index, MapListRoles::TypeRole).toString() == "map_group") {
        // verify uniqueness of new group name
        if (this->project->groupNames.contains(value.toString())) {
            return false;
        }
    }
    if (QStandardItemModel::setData(index, value, role)) {
        this->updateProject();
    }
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
    map->setText(mapName);
    map->setEditable(false);
    map->setData(mapName, Qt::UserRole);
    map->setData("map_name", MapListRoles::TypeRole);
    // map->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->mapItems.insert(mapName, map);
    return map;
}

QStandardItem *MapAreaModel::insertMapItem(QString mapName, QString areaName, int groupIndex) {
    // int areaIndex = this->project->mapSectionNameToValue[areaName];
    QStandardItem *area = this->areaItems[areaName];
    if (!area) {
        return nullptr;
    }
    int mapIndex = area->rowCount();
    QStandardItem *map = createMapItem(mapName, groupIndex, mapIndex);
    area->appendRow(map);
    return map;
}

void MapAreaModel::initialize() {
    this->areaItems.clear();
    this->mapItems.clear();
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

QStandardItem *LayoutTreeModel::insertMapItem(QString mapName, QString layoutId) {
    QStandardItem *layout = nullptr;
    if (this->layoutItems.contains(layoutId)) {
        layout = this->layoutItems[layoutId];
    }
    else {
        layout = createLayoutItem(layoutId);
        this->root->appendRow(layout);
    }
    if (!layout) {
        return nullptr;
    }
    QStandardItem *map = createMapItem(mapName);
    layout->appendRow(map);
    return map;
}

void LayoutTreeModel::initialize() {
    this->layoutItems.clear();
    this->mapItems.clear();
    for (int i = 0; i < this->project->mapLayoutsTable.length(); i++) {
        QString layoutId = project->mapLayoutsTable.value(i);
        QStandardItem *layoutItem = createLayoutItem(layoutId);
        this->root->appendRow(layoutItem);
    }

    for (auto mapList : this->project->groupedMapNames) {
        for (auto mapName : mapList) {
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
