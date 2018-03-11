#ifndef EDITOR_H
#define EDITOR_H

#include <QGraphicsScene>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemAnimation>
#include <QComboBox>
#include <QCheckBox>

#include "project.h"
#include "ui_mainwindow.h"

class DraggablePixmapItem;
class MapPixmapItem;
class CollisionPixmapItem;
class ConnectionPixmapItem;
class MetatilesPixmapItem;
class CollisionMetatilesPixmapItem;
class ElevationMetatilesPixmapItem;

class Editor : public QObject
{
    Q_OBJECT
public:
    Editor(Ui::MainWindow* ui);
public:
    Ui::MainWindow* ui;
    QObject *parent = NULL;
    Project *project = NULL;
    Map *map = NULL;
    void saveProject();
    void save();
    void undo();
    void redo();
    void setMap(QString map_name);
    void displayMap();
    void displayMetatiles();
    void displayCollisionMetatiles();
    void displayElevationMetatiles();
    void displayMapObjects();
    void displayMapConnections();
    void displayMapBorder();
    void displayMapGrid();

    void setEditingMap();
    void setEditingCollision();
    void setEditingObjects();
    void setEditingConnections();
    void setCurrentConnectionDirection(QString curDirection);
    void setConnectionsVisibility(bool visible);
    void updateConnectionOffset(int offset);
    void updateConnectionMap(QString mapName, QString direction);
    void addNewConnection();
    void removeCurrentConnection();
    void updateDiveMap(QString mapName);
    void updateEmergeMap(QString mapName);
    void setSelectedConnectionFromMap(QString mapName);

    DraggablePixmapItem *addMapObject(Event *event);
    void selectMapObject(DraggablePixmapItem *object);
    void selectMapObject(DraggablePixmapItem *object, bool toggle);
    DraggablePixmapItem *addNewEvent();
    DraggablePixmapItem *addNewEvent(QString event_type);
    void deleteEvent(Event *);
    void updateSelectedObjects();
    void redrawObject(DraggablePixmapItem *item);
    QList<DraggablePixmapItem *> *getObjects();

    QGraphicsScene *scene = NULL;
    QGraphicsPixmapItem *current_view = NULL;
    MapPixmapItem *map_item = NULL;
    ConnectionPixmapItem* current_connection_edit_item = NULL;
    QList<ConnectionPixmapItem*> connection_edit_items;
    CollisionPixmapItem *collision_item = NULL;
    QGraphicsItemGroup *objects_group = NULL;
    QList<QGraphicsPixmapItem*> borderItems;

    QGraphicsScene *scene_metatiles = NULL;
    QGraphicsScene *scene_collision_metatiles = NULL;
    QGraphicsScene *scene_elevation_metatiles = NULL;
    MetatilesPixmapItem *metatiles_item = NULL;
    CollisionMetatilesPixmapItem *collision_metatiles_item = NULL;
    ElevationMetatilesPixmapItem *elevation_metatiles_item = NULL;

    QList<DraggablePixmapItem*> *events = NULL;
    QList<DraggablePixmapItem*> *selected_events = NULL;

    QString map_edit_mode;

    void objectsView_onMousePress(QMouseEvent *event);
    void objectsView_onMouseMove(QMouseEvent *event);
    void objectsView_onMouseRelease(QMouseEvent *event);

private:
    void setConnectionItemsVisible(bool);
    void setBorderItemsVisible(bool, qreal = 1);
    void setConnectionEditControlValues(Connection*);
    void setConnectionEditControlsEnabled(bool);
    void createConnectionItem(Connection* connection, bool hide);
    void populateConnectionMapPickers();
    void setDiveEmergeControls();
    void updateDiveEmergeMap(QString mapName, QString direction);

private slots:
    void mouseEvent_map(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item);
    void onConnectionOffsetChanged(int newOffset);
    void onConnectionItemSelected(ConnectionPixmapItem* connectionItem);
    void onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem);
    void onConnectionDirectionChanged(QString newDirection);

signals:
    void objectsChanged();
    void selectedObjectsChanged();
    void loadMapRequested(QString, QString);
};



class DraggablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DraggablePixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Editor *editor = NULL;
    Event *event = NULL;
    QGraphicsItemAnimation *pos_anim = NULL;
    DraggablePixmapItem(Event *event_) : QGraphicsPixmapItem(event_->pixmap) {
        event = event_;
        updatePosition();
    }
    bool active;
    bool right_click;
    bool clicking;
    int last_x;
    int last_y;
    void updatePosition() {
        int x = event->x() * 16;
        int y = event->y() * 16;
        x -= pixmap().width() / 32 * 16;
        y -= pixmap().height() - 16;
        setX(x);
        setY(y);
        setZValue(event->y());
    }
    void move(int x, int y);
    void emitPositionChanged() {
        emit xChanged(event->x());
        emit yChanged(event->y());
        emit elevationChanged(event->elevation());
    }
    void updatePixmap() {
        QList<Event*> objects;
        objects.append(event);
        event->pixmap = QPixmap();
        editor->project->loadObjectPixmaps(objects);
        editor->redrawObject(this);
        emit spriteChanged(event->pixmap);
    }
    void bind(QComboBox *combo, QString key) {
        connect(combo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                this, [this, key](QString value){
            this->event->put(key, value);
        });
        connect(this, &DraggablePixmapItem::onPropertyChanged,
                this, [combo, key](QString key2, QString value){
            if (key2 == key) {
                combo->addItem(value);
                combo->setCurrentText(value);
            }
        });
    }

signals:
    void positionChanged(Event *event);
    void xChanged(int);
    void yChanged(int);
    void elevationChanged(int);
    void spriteChanged(QPixmap pixmap);
    void onPropertyChanged(QString key, QString value);

public slots:
    void set_x(const QString &text) {
        event->put("x", text);
        updatePosition();
    }
    void set_y(const QString &text) {
        event->put("y", text);
        updatePosition();
    }
    void set_elevation(const QString &text) {
        event->put("elevation", text);
        updatePosition();
    }
    void set_sprite(const QString &text) {
        event->put("sprite", text);
        updatePixmap();
    }
    void set_script(const QString &text) {
        event->put("script_label", text);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class EventGroup : public QGraphicsItemGroup {
};

class MapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    MapPixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Map *map = NULL;
    MapPixmapItem(Map *map_) {
        map = map_;
        setAcceptHoverEvents(true);
    }
    bool active;
    bool right_click;
    QPoint selection_origin;
    QList<QPoint> selection;
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void select(QGraphicsSceneMouseEvent*);
    virtual void undo();
    virtual void redo();
    virtual void draw();

private:
    void updateCurHoveredTile(QPointF pos);
    void paintNormal(int x, int y);
    void paintSmartPath(int x, int y);
    static QList<int> smartPathTable;

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, MapPixmapItem *);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class CollisionPixmapItem : public MapPixmapItem {
    Q_OBJECT
public:
    CollisionPixmapItem(QPixmap pixmap): MapPixmapItem(pixmap) {
    }
    CollisionPixmapItem(Map *map_): MapPixmapItem(map_) {
    }
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void draw();

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, CollisionPixmapItem *);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class ConnectionPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ConnectionPixmapItem(QPixmap pixmap, Connection* connection, int x, int y, int baseMapWidth, int baseMapHeight): QGraphicsPixmapItem(pixmap) {
        this->basePixmap = pixmap;
        this->connection = connection;
        setFlag(ItemIsMovable);
        setFlag(ItemSendsGeometryChanges);
        this->initialX = x;
        this->initialY = y;
        this->initialOffset = connection->offset.toInt();
        this->baseMapWidth = baseMapWidth;
        this->baseMapHeight = baseMapHeight;
    }
    void render(qreal opacity = 1) {
        QPixmap newPixmap = basePixmap.copy(0, 0, basePixmap.width(), basePixmap.height());
        if (opacity < 1) {
            QPainter painter(&newPixmap);
            int alpha = (int)(255 * (1 - opacity));
            painter.fillRect(0, 0, newPixmap.width(), newPixmap.height(), QColor(0, 0, 0, alpha));
            painter.end();
        }
        this->setPixmap(newPixmap);
    }
    int getMinOffset();
    int getMaxOffset();
    QPixmap basePixmap;
    Connection* connection;
    int initialX;
    int initialY;
    int initialOffset;
    int baseMapWidth;
    int baseMapHeight;
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
signals:
    void connectionItemSelected(ConnectionPixmapItem* connectionItem);
    void connectionItemDoubleClicked(ConnectionPixmapItem* connectionItem);
    void connectionMoved(int offset);
};

class MetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    MetatilesPixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    MetatilesPixmapItem(Map *map_) {
        map = map_;
        setAcceptHoverEvents(true);
        connect(map, SIGNAL(paintTileChanged(Map*)), this, SLOT(paintTileChanged(Map *)));
    }
    Map* map = NULL;
    virtual void draw();
private:
    void updateSelection(QPointF pos, Qt::MouseButton button);
protected:
    virtual void updateCurHoveredMetatile(QPointF pos);
private slots:
    void paintTileChanged(Map *map);
protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class CollisionMetatilesPixmapItem : public MetatilesPixmapItem {
    Q_OBJECT
public:
    CollisionMetatilesPixmapItem(Map *map_): MetatilesPixmapItem(map_) {
        connect(map, SIGNAL(paintCollisionChanged(Map*)), this, SLOT(paintCollisionChanged(Map *)));
    }
    virtual void pick(uint collision) {
        map->paint_collision = collision;
        draw();
    }
    virtual void draw() {
        setPixmap(map->renderCollisionMetatiles());
    }
protected:
    virtual void updateCurHoveredMetatile(QPointF pos);
private slots:
    void paintCollisionChanged(Map *map) {
        draw();
    }
};

class ElevationMetatilesPixmapItem : public MetatilesPixmapItem {
    Q_OBJECT
public:
    ElevationMetatilesPixmapItem(Map *map_): MetatilesPixmapItem(map_) {
        connect(map, SIGNAL(paintCollisionChanged(Map*)), this, SLOT(paintCollisionChanged(Map *)));
    }
    virtual void pick(uint elevation) {
        map->paint_elevation = elevation;
        draw();
    }
    virtual void draw() {
        setPixmap(map->renderElevationMetatiles());
    }
protected:
    virtual void updateCurHoveredMetatile(QPointF pos);
private slots:
    void paintCollisionChanged(Map *map) {
        draw();
    }
};


#endif // EDITOR_H
