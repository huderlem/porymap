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
class BorderMetatilesPixmapItem;
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
    void displayBorderMetatiles();
    void displayCollisionMetatiles();
    void displayElevationMetatiles();
    void displayMapEvents();
    void displayMapConnections();
    void displayMapBorder();
    void displayMapGrid();

    void setEditingMap();
    void setEditingCollision();
    void setEditingObjects();
    void setEditingConnections();
    void setCurrentConnectionDirection(QString curDirection);
    void updateCurrentConnectionDirection(QString curDirection);
    void setConnectionsVisibility(bool visible);
    void updateConnectionOffset(int offset);
    void setConnectionMap(QString mapName);
    void addNewConnection();
    void removeCurrentConnection();
    void updateDiveMap(QString mapName);
    void updateEmergeMap(QString mapName);
    void setSelectedConnectionFromMap(QString mapName);
    void updatePrimaryTileset(QString tilesetLabel);
    void updateSecondaryTileset(QString tilesetLabel);

    DraggablePixmapItem *addMapEvent(Event *event);
    void selectMapEvent(DraggablePixmapItem *object);
    void selectMapEvent(DraggablePixmapItem *object, bool toggle);
    DraggablePixmapItem *addNewEvent(QString event_type);
    Event* createNewEvent(QString event_type);
    void deleteEvent(Event *);
    void updateSelectedEvents();
    void redrawObject(DraggablePixmapItem *item);
    QList<DraggablePixmapItem *> *getObjects();

    QGraphicsScene *scene = NULL;
    QGraphicsPixmapItem *current_view = NULL;
    MapPixmapItem *map_item = NULL;
    ConnectionPixmapItem* selected_connection_item = NULL;
    QList<QGraphicsPixmapItem*> connection_items;
    QList<ConnectionPixmapItem*> connection_edit_items;
    CollisionPixmapItem *collision_item = NULL;
    QGraphicsItemGroup *events_group = NULL;
    QList<QGraphicsPixmapItem*> borderItems;
    QList<QGraphicsLineItem*> gridLines;

    QGraphicsScene *scene_metatiles = NULL;
    QGraphicsScene *scene_selected_border_metatiles = NULL;
    QGraphicsScene *scene_collision_metatiles = NULL;
    QGraphicsScene *scene_elevation_metatiles = NULL;
    MetatilesPixmapItem *metatiles_item = NULL;
    BorderMetatilesPixmapItem *selected_border_metatiles_item = NULL;
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
    void onConnectionOffsetChanged(int newOffset);
    void removeMirroredConnection(Connection*);
    void updateMirroredConnectionOffset(Connection*);
    void updateMirroredConnectionDirection(Connection*, QString);
    void updateMirroredConnectionMap(Connection*, QString);
    void updateMirroredConnection(Connection*, QString, QString, bool isDelete = false);
    Event* createNewObjectEvent();
    Event* createNewWarpEvent();
    Event* createNewCoordScriptEvent();
    Event* createNewCoordWeatherEvent();
    Event* createNewSignEvent();
    Event* createNewHiddenItemEvent();
    Event* createNewSecretBaseEvent();

private slots:
    void mouseEvent_map(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item);
    void onConnectionMoved(Connection*);
    void onConnectionItemSelected(ConnectionPixmapItem* connectionItem);
    void onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem);
    void onConnectionDirectionChanged(QString newDirection);
    void onBorderMetatilesChanged();

signals:
    void objectsChanged();
    void selectedObjectsChanged();
    void loadMapRequested(QString, QString);
    void tilesetChanged(QString);
};



class DraggablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DraggablePixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Editor *editor = NULL;
    Event *event = NULL;
    QGraphicsItemAnimation *pos_anim = NULL;
    DraggablePixmapItem(Event *event_, Editor *editor_) : QGraphicsPixmapItem(event_->pixmap) {
        event = event_;
        editor = editor_;
        updatePosition();
    }
    bool active;
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
        editor->project->loadEventPixmaps(objects);
        editor->redrawObject(this);
        emit spriteChanged(event->pixmap);
    }
    void bind(QComboBox *combo, QString key) {
        connect(combo, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentTextChanged),
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
    void _floodFill(int x, int y);
    void _floodFillSmartPath(int initialX, int initialY);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void select(QGraphicsSceneMouseEvent*);
    virtual void draw(bool ignoreCache = false);

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
    virtual void draw(bool ignoreCache = false);

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
    void connectionMoved(Connection*);
};

class MetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
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

class BorderMetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    BorderMetatilesPixmapItem(Map *map_) {
        map = map_;
        setAcceptHoverEvents(true);
    }
    Map* map = NULL;
    virtual void draw();
signals:
    void borderMetatilesChanged();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
};

class MovementPermissionsPixmapItem : public MetatilesPixmapItem {
    Q_OBJECT
public:
    MovementPermissionsPixmapItem(Map *map_): MetatilesPixmapItem(map_) {}
    virtual void pick(uint collision) {
        map->paint_collision = collision;
        draw();
    }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class CollisionMetatilesPixmapItem : public MovementPermissionsPixmapItem {
    Q_OBJECT
public:
    CollisionMetatilesPixmapItem(Map *map_): MovementPermissionsPixmapItem(map_) {
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

class ElevationMetatilesPixmapItem : public MovementPermissionsPixmapItem {
    Q_OBJECT
public:
    ElevationMetatilesPixmapItem(Map *map_): MovementPermissionsPixmapItem(map_) {
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
