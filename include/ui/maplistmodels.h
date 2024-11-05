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
        this->setFocusPolicy(Qt::StrongFocus);
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



class QRegularExpressionValidator;

class MapListModel : public QStandardItemModel {
    Q_OBJECT

public:
    MapListModel(QObject *parent = nullptr) : QStandardItemModel(parent) {};
    ~MapListModel() { }

    virtual QModelIndex indexOf(QString id) const = 0;
    virtual void removeFolder(int index) = 0;
    virtual void removeItem(const QModelIndex &index);
    virtual QStandardItem *getItem(const QModelIndex &index) const = 0;
};

class MapGroupModel : public MapListModel {
    Q_OBJECT

public:
    MapGroupModel(Project *project, QObject *parent = nullptr);
    ~MapGroupModel() { }

    QVariant data(const QModelIndex &index, int role) const override;

    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

public:
    void setMap(QString mapName) { this->openMap = mapName; }

    QStandardItem *createGroupItem(QString groupName, int groupIndex, QStandardItem *fromItem = nullptr);
    QStandardItem *createMapItem(QString mapName, QStandardItem *fromItem = nullptr);

    QStandardItem *insertGroupItem(QString groupName);
    QStandardItem *insertMapItem(QString mapName, QString groupName);
    virtual void removeFolder(int index) override;

    virtual QStandardItem *getItem(const QModelIndex &index) const override;
    virtual QModelIndex indexOf(QString mapName) const override;

    void initialize();

private:
    friend class MapTree;
    void updateProject();

private:
    Project *project;
    QStandardItem *root = nullptr;

    QMap<QString, QStandardItem *> groupItems;
    QMap<QString, QStandardItem *> mapItems;

    QString openMap;

signals:
    void dragMoveCompleted();
};



class MapAreaModel : public MapListModel {
    Q_OBJECT

public:
    MapAreaModel(Project *project, QObject *parent = nullptr);
    ~MapAreaModel() {}

    QVariant data(const QModelIndex &index, int role) const override;

public:
    void setMap(QString mapName) { this->openMap = mapName; }

    QStandardItem *createAreaItem(QString areaName, int areaIndex);
    QStandardItem *createMapItem(QString mapName, int areaIndex, int mapIndex);

    QStandardItem *insertAreaItem(QString areaName);
    QStandardItem *insertMapItem(QString mapName, QString areaName, int groupIndex);
    virtual void removeFolder(int index) override;

    virtual QStandardItem *getItem(const QModelIndex &index) const override;
    virtual QModelIndex indexOf(QString mapName) const override;

    void initialize();

private:
    Project *project;
    QStandardItem *root = nullptr;

    QMap<QString, QStandardItem *> areaItems;
    QMap<QString, QStandardItem *> mapItems;

    QString openMap;

signals:
    void edited();
};



class LayoutTreeModel : public MapListModel {
    Q_OBJECT

public:
    LayoutTreeModel(Project *project, QObject *parent = nullptr);
    ~LayoutTreeModel() {}

    QVariant data(const QModelIndex &index, int role) const override;

public:
    void setLayout(QString layoutId) { this->openLayout = layoutId; }

    QStandardItem *createLayoutItem(QString layoutId);
    QStandardItem *createMapItem(QString mapName);

    QStandardItem *insertLayoutItem(QString layoutId);
    QStandardItem *insertMapItem(QString mapName, QString layoutId);
    virtual void removeFolder(int index) override;

    virtual QStandardItem *getItem(const QModelIndex &index) const override;
    virtual QModelIndex indexOf(QString layoutName) const override;

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
