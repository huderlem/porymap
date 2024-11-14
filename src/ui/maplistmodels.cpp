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

void MapTree::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Delete selected items in the tree
        auto selectionModel = this->selectionModel();
        if (!selectionModel->hasSelection())
            return;

        auto model = static_cast<FilterChildrenProxyModel*>(this->model());
        auto sourceModel = static_cast<MapListModel*>(model->sourceModel());

        QModelIndexList selectedIndexes = selectionModel->selectedRows();
        QList<QPersistentModelIndex> persistentIndexes;
        for (const auto &index : selectedIndexes) {
            persistentIndexes.append(model->mapToSource(index));
        }
        for (const auto &index : persistentIndexes) {
            sourceModel->removeItemAt(index);
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

void MapListModel::removeItemAt(const QModelIndex &index) {
    QStandardItem *item = this->getItem(index)->child(index.row(), index.column());
    if (!item)
        return;

    const QString type = item->data(MapListUserRoles::TypeRole).toString();
    if (type == "map_name") {
        // TODO: No support for deleting maps
    } else {
        // TODO: Because there's no support for deleting maps we can only delete empty folders
        if (!item->hasChildren()) {
            this->removeItem(item);
        }
    }
}



QWidget *GroupNameDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    QLineEdit *editor = new QLineEdit(parent);
    static const QRegularExpression expression("[A-Za-z_]+[\\w]*");
    editor->setPlaceholderText("gMapGroup_");
    editor->setValidator(new QRegularExpressionValidator(expression, parent));
    editor->setFrame(false);
    return editor;
}

void GroupNameDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    QString groupName = index.data(MapListUserRoles::NameRole).toString();
    QLineEdit *le = static_cast<QLineEdit *>(editor);
    le->setText(groupName);
}

void GroupNameDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QLineEdit *le = static_cast<QLineEdit *>(editor);
    QString groupName = le->text();
    model->setData(index, groupName, MapListUserRoles::NameRole);
}

void GroupNameDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const {
    editor->setGeometry(option.rect);
}



MapGroupModel::MapGroupModel(Project *project, QObject *parent) : MapListModel(parent) {
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
          << "application/porymap.mapgroupmodel.group"
          << "application/porymap.mapgroupmodel.source.row"
          << "application/porymap.mapgroupmodel.source.column";
    return types;
}

QMimeData *MapGroupModel::mimeData(const QModelIndexList &indexes) const {
    QMimeData *mimeData = QStandardItemModel::mimeData(indexes);
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    // if dropping a selection containing a group(s) and map(s), clear all selection but first group.
    for (const QModelIndex &index : indexes) {
        if (index.isValid() && data(index, MapListUserRoles::TypeRole).toString() == "map_group") {
            QString groupName = data(index, MapListUserRoles::NameRole).toString();
            stream << groupName;
            mimeData->setData("application/porymap.mapgroupmodel.group", encodedData);
            mimeData->setData("application/porymap.mapgroupmodel.source.row", QByteArray::number(index.row()));
            return mimeData;
        }
    }

    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            QString mapName = data(index, MapListUserRoles::NameRole).toString();
            stream << mapName;
        }
    }

    mimeData->setData("application/porymap.mapgroupmodel.map", encodedData);
    return mimeData;
}

bool MapGroupModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int, const QModelIndex &parentIndex) {
    if (action == Qt::IgnoreAction)
        return true;

    if (!parentIndex.isValid() && !data->hasFormat("application/porymap.mapgroupmodel.group"))
        return false;

    int firstRow = 0;
 
    if (row != -1) {
        firstRow = row;
    }
    else if (parentIndex.isValid()) {
        firstRow = rowCount(parentIndex);
    }

    if (data->hasFormat("application/porymap.mapgroupmodel.group")) {
        if (parentIndex.row() != -1 || parentIndex.column() != -1) {
            return false;
        }
        QByteArray encodedData = data->data("application/porymap.mapgroupmodel.group");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QString groupName;

        while (!stream.atEnd()) {
            stream >> groupName;
        }

        this->insertRow(row, parentIndex);

        // copy children to new node
        int sourceRow = data->data("application/porymap.mapgroupmodel.source.row").toInt();
        QModelIndex originIndex = this->index(sourceRow, 0);
        QModelIndexList children;
        QStringList mapsToMove;
        for (int i = 0; i < this->rowCount(originIndex); ++i ) {
            children << this->index( i, 0, originIndex);
            mapsToMove << this->index( i, 0 , originIndex).data(MapListUserRoles::NameRole).toString();
        }

        QModelIndex groupIndex = index(row, 0, parentIndex);
        QStandardItem *groupItem = this->itemFromIndex(groupIndex);
        createGroupItem(groupName, groupItem);

        for (QString mapName : mapsToMove) {
            QStandardItem *mapItem = createMapItem(mapName);
            groupItem->appendRow(mapItem);
        }
    }
    else if (data->hasFormat("application/porymap.mapgroupmodel.map")) {
        QByteArray encodedData = data->data("application/porymap.mapgroupmodel.map");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QStringList droppedMaps;
        int rowCount = 0;

        while (!stream.atEnd()) {
            QString mapName;
            stream >> mapName;
            droppedMaps << mapName;
            rowCount++;
        }

        QStandardItem *groupItem = this->itemFromIndex(parentIndex);
        if (groupItem->hasChildren()) {
            this->insertRows(firstRow, rowCount, parentIndex);
            for (QString mapName : droppedMaps) {
                QModelIndex mapIndex = index(firstRow, 0, parentIndex);
                QStandardItem *mapItem = this->itemFromIndex(mapIndex);
                createMapItem(mapName, mapItem);
                firstRow++;
            }
        }
        // for whatever reason insertRows doesn't work as I expected with childless items
        // so just append all the new maps instead
        else {
            for (QString mapName : droppedMaps) {
                QStandardItem *mapItem = createMapItem(mapName);
                groupItem->appendRow(mapItem);
                firstRow++;
            }
        }

    }

    emit dragMoveCompleted();
    updateProject();

    return true;
}

void MapGroupModel::updateProject() {
    if (!this->project) return;

    // Temporary objects in case of failure, so we won't modify the project unless it succeeds.
    QStringList mapNames;
    QStringList groupNames;
    QMap<QString, QStringList> groupNameToMapNames;

    for (int g = 0; g < this->root->rowCount(); g++) {
        const QStandardItem *groupItem = this->item(g);
        QString groupName = groupItem->data(MapListUserRoles::NameRole).toString();
        groupNames.append(groupName);
        for (int m = 0; m < groupItem->rowCount(); m++) {
            const QStandardItem *mapItem = groupItem->child(m);
            if (!mapItem) {
                logError("An error occured while trying to apply updates to map group structure.");
                return;
            }
            QString mapName = mapItem->data(MapListUserRoles::NameRole).toString();
            groupNameToMapNames[groupName].append(mapName);
            mapNames.append(mapName);
        }
    }

    this->project->mapNames = mapNames;
    this->project->groupNames = groupNames;
    this->project->groupNameToMapNames = groupNameToMapNames;
    this->project->hasUnsavedDataChanges = true;
}

QStandardItem *MapGroupModel::createGroupItem(QString groupName, QStandardItem *group) {
    if (!group) group = new QStandardItem;
    group->setText(groupName);
    group->setData(groupName, MapListUserRoles::NameRole);
    group->setData("map_group", MapListUserRoles::TypeRole);
    group->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsSelectable);
    this->groupItems.insert(groupName, group);
    return group;
}

QStandardItem *MapGroupModel::createMapItem(QString mapName, QStandardItem *map) {
    if (!map) map = new QStandardItem;
    map->setData(mapName, MapListUserRoles::NameRole);
    map->setData("map_name", MapListUserRoles::TypeRole);
    map->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->mapItems[mapName] = map;
    return map;
}

QStandardItem *MapGroupModel::insertGroupItem(QString groupName) {
    QStandardItem *group = createGroupItem(groupName);
    this->root->appendRow(group);
    return group;
}

void MapGroupModel::removeItem(QStandardItem *item) {
    this->removeRow(item->row());
    this->updateProject();
}

QStandardItem *MapGroupModel::insertMapItem(QString mapName, QString groupName) {
    QStandardItem *group = this->groupItems[groupName];
    if (!group) {
        group = insertGroupItem(groupName);
    }
    QStandardItem *map = createMapItem(mapName);
    group->appendRow(map);
    return map;
}

void MapGroupModel::initialize() {
    this->groupItems.clear();
    this->mapItems.clear();


    for (const auto &groupName : this->project->groupNames) {
        QStandardItem *group = createGroupItem(groupName);
        root->appendRow(group);
        for (const auto &mapName : this->project->groupNameToMapNames.value(groupName)) {
            group->appendRow(createMapItem(mapName));
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

QModelIndex MapGroupModel::indexOf(QString mapName) const {
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
        QString type = item->data(MapListUserRoles::TypeRole).toString();

        if (type == "map_group") {
            if (!item->hasChildren()) {
                return folderIcon;
            }
            return mapFolderIcon;
        } else if (type == "map_name") {
            QString mapName = item->data(MapListUserRoles::NameRole).toString();
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
        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListUserRoles::TypeRole).toString();

        if (type == "map_name") {
            return QString("[%1.%2] ").arg(this->getItem(index)->row()).arg(row, 2, 10, QLatin1Char('0')) + item->data(MapListUserRoles::NameRole).toString();
        }
        else if (type == "map_group") {
            return item->data(MapListUserRoles::NameRole).toString();
        }
    }

    return QStandardItemModel::data(index, role);
}

bool MapGroupModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == MapListUserRoles::NameRole && data(index, MapListUserRoles::TypeRole).toString() == "map_group") {
        // verify uniqueness of new group name
        // TODO: Check that the name is a valid symbol name (i.e. only word characters, not starting with a number)
        if (this->project->groupNames.contains(value.toString())) {
            return false;
        }
    }
    if (QStandardItemModel::setData(index, value, role)) {
        this->updateProject();
    }
    return true;
}



MapAreaModel::MapAreaModel(Project *project, QObject *parent) : MapListModel(parent) {
    this->project = project;
    this->root = this->invisibleRootItem();

    initialize();
}

QStandardItem *MapAreaModel::createAreaItem(QString mapsecName) {
    QStandardItem *area = new QStandardItem;
    area->setText(mapsecName);
    area->setEditable(false);
    area->setData(mapsecName, MapListUserRoles::NameRole);
    area->setData("map_section", MapListUserRoles::TypeRole);
    // group->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->areaItems.insert(mapsecName, area);
    return area;
}

QStandardItem *MapAreaModel::createMapItem(QString mapName) {
    QStandardItem *map = new QStandardItem;
    map->setText(mapName);
    map->setEditable(false);
    map->setData(mapName, MapListUserRoles::NameRole);
    map->setData("map_name", MapListUserRoles::TypeRole);
    // map->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->mapItems.insert(mapName, map);
    return map;
}

QStandardItem *MapAreaModel::insertAreaItem(QString areaName) {
    this->project->addNewMapsec(areaName);
    QStandardItem *item = createAreaItem(areaName);
    this->root->appendRow(item);
    this->sort(0, Qt::AscendingOrder);
    return item;
}

QStandardItem *MapAreaModel::insertMapItem(QString mapName, QString areaName) {
    QStandardItem *area = this->areaItems[areaName];
    if (!area) {
        return nullptr;
    }
    QStandardItem *map = createMapItem(mapName);
    area->appendRow(map);
    return map;
}

void MapAreaModel::removeItem(QStandardItem *item) {
    this->project->removeMapsec(item->data(MapListUserRoles::NameRole).toString());
    this->removeRow(item->row());
}

void MapAreaModel::initialize() {
    this->areaItems.clear();
    this->mapItems.clear();

    for (const auto &idName : this->project->mapSectionIdNames) {
        this->root->appendRow(createAreaItem(idName));
    }

    for (const auto &mapName : this->project->mapNames) {
        const QString mapsecName = this->project->mapNameToMapSectionName.value(mapName);
        if (this->areaItems.contains(mapsecName))
            this->areaItems[mapsecName]->appendRow(createMapItem(mapName));
    }

    this->sort(0, Qt::AscendingOrder);
}

QStandardItem *MapAreaModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        QStandardItem *item = static_cast<QStandardItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return this->root;
}

QModelIndex MapAreaModel::indexOf(QString mapName) const {
    if (this->mapItems.contains(mapName)) {
        return this->mapItems[mapName]->index();
    }
    return QModelIndex();
}

QVariant MapAreaModel::data(const QModelIndex &index, int role) const {
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
        QString type = item->data(MapListUserRoles::TypeRole).toString();

        if (type == "map_section") {
            if (item->hasChildren()) {
                return mapFolderIcon;
            }
            return folderIcon;
        } else if (type == "map_name") {
            QString mapName = item->data(MapListUserRoles::NameRole).toString();
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
        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListUserRoles::TypeRole).toString();

        if (type == "map_section") {
            return item->data(MapListUserRoles::NameRole).toString();
        }
    }

    return QStandardItemModel::data(index, role);
}



LayoutTreeModel::LayoutTreeModel(Project *project, QObject *parent) : MapListModel(parent) {
    this->project = project;
    this->root = this->invisibleRootItem();

    initialize();
}

QStandardItem *LayoutTreeModel::createLayoutItem(QString layoutId) {
    QStandardItem *layout = new QStandardItem;
    layout->setText(this->project->mapLayouts[layoutId]->name);
    layout->setEditable(false);
    layout->setData(layoutId, MapListUserRoles::NameRole);
    layout->setData("map_layout", MapListUserRoles::TypeRole);
    // // group->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    this->layoutItems.insert(layoutId, layout);
    return layout;
}

QStandardItem *LayoutTreeModel::createMapItem(QString mapName) {
    QStandardItem *map = new QStandardItem;
    map->setText(mapName);
    map->setEditable(false);
    map->setData(mapName, MapListUserRoles::NameRole);
    map->setData("map_name", MapListUserRoles::TypeRole);
    map->setFlags(Qt::NoItemFlags | Qt::ItemNeverHasChildren);
    this->mapItems.insert(mapName, map);
    return map;
}

QStandardItem *LayoutTreeModel::insertLayoutItem(QString layoutId) {
    QStandardItem *layoutItem = this->createLayoutItem(layoutId);
    this->root->appendRow(layoutItem);
    this->sort(0, Qt::AscendingOrder);
    return layoutItem;
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

void LayoutTreeModel::removeItem(QStandardItem *) {
    // TODO: Deleting layouts not supported
}


void LayoutTreeModel::initialize() {
    this->layoutItems.clear();
    this->mapItems.clear();

    for (const auto &layoutId : this->project->layoutIds) {
        this->root->appendRow(createLayoutItem(layoutId));
    }

    for (const auto &mapName : this->project->mapNames) {
        QString layoutId = project->mapNameToLayoutId.value(mapName);
        if (this->layoutItems.contains(layoutId))
            this->layoutItems[layoutId]->appendRow(createMapItem(mapName));
    }

    this->sort(0, Qt::AscendingOrder);
}

QStandardItem *LayoutTreeModel::getItem(const QModelIndex &index) const {
    if (index.isValid()) {
        QStandardItem *item = static_cast<QStandardItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return this->root;
}

QModelIndex LayoutTreeModel::indexOf(QString layoutName) const {
    if (this->layoutItems.contains(layoutName)) {
        return this->layoutItems[layoutName]->index();
    }
    return QModelIndex();
}

QVariant LayoutTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    int row = index.row();
    int col = index.column();

    if (role == Qt::DecorationRole) {
        static QIcon mapGrayIcon = QIcon(QStringLiteral(":/icons/map_grayed.ico"));
        static QIcon mapIcon = QIcon(QStringLiteral(":/icons/map.ico"));
        static QIcon mapEditedIcon = QIcon(QStringLiteral(":/icons/map_edited.ico"));
        static QIcon mapOpenedIcon = QIcon(QStringLiteral(":/icons/map_opened.ico"));

        QStandardItem *item = this->getItem(index)->child(row, col);
        QString type = item->data(MapListUserRoles::TypeRole).toString();

        if (type == "map_layout") {
            QString layoutId = item->data(MapListUserRoles::NameRole).toString();
            if (layoutId == this->openLayout) {
                return mapOpenedIcon;
            }
            else if (this->project->mapLayouts.contains(layoutId)) {
                if (this->project->mapLayouts.value(layoutId)->hasUnsavedChanges()) {
                    return mapEditedIcon;
                }
                else if (!this->project->mapLayouts[layoutId]->loaded) {
                    return mapGrayIcon;
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
