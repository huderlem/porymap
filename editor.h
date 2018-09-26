#ifndef EDITOR_H
#define EDITOR_H

#include <QGraphicsScene>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemAnimation>
#include <QComboBox>
#include <QCheckBox>
#include <QCursor>

#include "mapconnection.h"
#include "metatileselector.h"
#include "movementpermissionsselector.h"
#include "project.h"
#include "ui_mainwindow.h"

class DraggablePixmapItem;
class MapPixmapItem;
class CollisionPixmapItem;
class ConnectionPixmapItem;
class MetatilesPixmapItem;
class BorderMetatilesPixmapItem;
class CurrentSelectedMetatilesPixmapItem;

#define SWAP(a, b) do { if (a != b) { a ^= b; b ^= a; a ^= b; } } while (0)

class Editor : public QObject
{
    Q_OBJECT
public:
    Editor(Ui::MainWindow* ui);
public:
    Ui::MainWindow* ui;
    QObject *parent = nullptr;
    Project *project = nullptr;
    Map *map = nullptr;
    void saveProject();
    void save();
    void undo();
    void redo();
    void setMap(QString map_name);
    void displayMap();
    void displayMetatileSelector();
    void displayBorderMetatiles();
    void displayCurrentMetatilesSelection();
    void redrawCurrentMetatilesSelection();
    void displayMovementPermissionSelector();
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
    void toggleBorderVisibility(bool visible);

    DraggablePixmapItem *addMapEvent(Event *event);
    void selectMapEvent(DraggablePixmapItem *object);
    void selectMapEvent(DraggablePixmapItem *object, bool toggle);
    DraggablePixmapItem *addNewEvent(QString event_type);
    Event* createNewEvent(QString event_type);
    void deleteEvent(Event *);
    void updateSelectedEvents();
    void redrawObject(DraggablePixmapItem *item);
    QList<DraggablePixmapItem *> *getObjects();

    QGraphicsScene *scene = nullptr;
    QGraphicsPixmapItem *current_view = nullptr;
    MapPixmapItem *map_item = nullptr;
    ConnectionPixmapItem* selected_connection_item = nullptr;
    QList<QGraphicsPixmapItem*> connection_items;
    QList<ConnectionPixmapItem*> connection_edit_items;
    CollisionPixmapItem *collision_item = nullptr;
    QGraphicsItemGroup *events_group = nullptr;
    QList<QGraphicsPixmapItem*> borderItems;
    QList<QGraphicsLineItem*> gridLines;

    QGraphicsScene *scene_metatiles = nullptr;
    QGraphicsScene *scene_current_metatile_selection = nullptr;
    QGraphicsScene *scene_selected_border_metatiles = nullptr;
    QGraphicsScene *scene_collision_metatiles = nullptr;
    QGraphicsScene *scene_elevation_metatiles = nullptr;
    MetatileSelector *metatile_selector_item = nullptr;

    BorderMetatilesPixmapItem *selected_border_metatiles_item = nullptr;
    CurrentSelectedMetatilesPixmapItem *scene_current_metatile_selection_item = nullptr;
    MovementPermissionsSelector *movement_permissions_selector_item = nullptr;

    QList<DraggablePixmapItem*> *events = nullptr;
    QList<DraggablePixmapItem*> *selected_events = nullptr;

    QString map_edit_mode;
    QString prev_edit_mode;
    QCursor cursor;
    bool smart_paths_enabled = false;

    void objectsView_onMousePress(QMouseEvent *event);
    void objectsView_onMouseMove(QMouseEvent *event);
    void objectsView_onMouseRelease(QMouseEvent *event);

private:
    void setConnectionItemsVisible(bool);
    void setBorderItemsVisible(bool, qreal = 1);
    void setConnectionEditControlValues(MapConnection*);
    void setConnectionEditControlsEnabled(bool);
    void createConnectionItem(MapConnection* connection, bool hide);
    void populateConnectionMapPickers();
    void setDiveEmergeControls();
    void updateDiveEmergeMap(QString mapName, QString direction);
    void onConnectionOffsetChanged(int newOffset);
    void removeMirroredConnection(MapConnection*);
    void updateMirroredConnectionOffset(MapConnection*);
    void updateMirroredConnectionDirection(MapConnection*, QString);
    void updateMirroredConnectionMap(MapConnection*, QString);
    void updateMirroredConnection(MapConnection*, QString, QString, bool isDelete = false);
    Event* createNewObjectEvent();
    Event* createNewWarpEvent();
    Event* createNewHealLocationEvent();
    Event* createNewCoordScriptEvent();
    Event* createNewCoordWeatherEvent();
    Event* createNewSignEvent();
    Event* createNewHiddenItemEvent();
    Event* createNewSecretBaseEvent();
    QString getMovementPermissionText(uint16_t collision, uint16_t elevation);

private slots:
    void mouseEvent_map(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item);
    void onConnectionMoved(MapConnection*);
    void onConnectionItemSelected(ConnectionPixmapItem* connectionItem);
    void onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem);
    void onConnectionDirectionChanged(QString newDirection);
    void onBorderMetatilesChanged();
    void onHoveredMovementPermissionChanged(uint16_t, uint16_t);
    void onHoveredMovementPermissionCleared();
    void onHoveredMetatileSelectionChanged(uint16_t);
    void onHoveredMetatileSelectionCleared();
    void onHoveredMapMetatileChanged(int, int);
    void onHoveredMapMetatileCleared();
    void onHoveredMapMovementPermissionChanged(int, int);
    void onHoveredMapMovementPermissionCleared();
    void onSelectedMetatilesChanged();

signals:
    void objectsChanged();
    void selectedObjectsChanged();
    void loadMapRequested(QString, QString);
    void tilesetChanged(QString);
    void warpEventDoubleClicked(QString mapName, QString warpNum);
    void currentMetatilesSelectionChanged();
};



class DraggablePixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DraggablePixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Editor *editor = nullptr;
    Event *event = nullptr;
    QGraphicsItemAnimation *pos_anim = nullptr;
    DraggablePixmapItem(Event *event_, Editor *editor_) : QGraphicsPixmapItem(event_->pixmap) {
        event = event_;
        editor = editor_;
        updatePosition();
    }
    bool active;
    int last_x;
    int last_y;
    void updatePosition() {
        int x = event->getPixelX();
        int y = event->getPixelY();
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
        this->updatePosition();
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
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
};

class EventGroup : public QGraphicsItemGroup {
};

class MapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    MapPixmapItem(QPixmap pixmap): QGraphicsPixmapItem(pixmap) {
    }
    Map *map = nullptr;
    Editor *editor = nullptr;
    MapPixmapItem(Map *map_, Editor *editor_) {
        map = map_;
        editor = editor_;
        setAcceptHoverEvents(true);
    }
    bool active;
    bool right_click;
    int paint_tile_initial_x;
    int paint_tile_initial_y;
    QPoint selection_origin;
    QList<QPoint> selection;
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    void _floodFill(int x, int y);
    void _floodFillSmartPath(int initialX, int initialY);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void select(QGraphicsSceneMouseEvent*);
    virtual void shift(QGraphicsSceneMouseEvent*);
    virtual void draw(bool ignoreCache = false);
    void updateMetatileSelection(QGraphicsSceneMouseEvent *event);

private:
    void paintNormal(int x, int y);
    void paintSmartPath(int x, int y);
    static QList<int> smartPathTable;

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, MapPixmapItem *);
    void hoveredMapMetatileChanged(int x, int y);
    void hoveredMapMetatileCleared();

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
    CollisionPixmapItem(Map *map_, Editor *editor_): MapPixmapItem(map_, editor_) {
    }
    void updateMovementPermissionSelection(QGraphicsSceneMouseEvent *event);
    virtual void paint(QGraphicsSceneMouseEvent*);
    virtual void floodFill(QGraphicsSceneMouseEvent*);
    virtual void pick(QGraphicsSceneMouseEvent*);
    virtual void draw(bool ignoreCache = false);

signals:
    void mouseEvent(QGraphicsSceneMouseEvent *, CollisionPixmapItem *);
    void hoveredMapMovementPermissionChanged(int, int);
    void hoveredMapMovementPermissionCleared();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent*);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
};

class ConnectionPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    ConnectionPixmapItem(QPixmap pixmap, MapConnection* connection, int x, int y, int baseMapWidth, int baseMapHeight): QGraphicsPixmapItem(pixmap) {
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
            int alpha = static_cast<int>(255 * (1 - opacity));
            painter.fillRect(0, 0, newPixmap.width(), newPixmap.height(), QColor(0, 0, 0, alpha));
            painter.end();
        }
        this->setPixmap(newPixmap);
    }
    int getMinOffset();
    int getMaxOffset();
    QPixmap basePixmap;
    MapConnection* connection;
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
    void connectionMoved(MapConnection*);
};

class BorderMetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    BorderMetatilesPixmapItem(Map *map_, Editor *editor_) {
        map = map_;
        editor = editor_;
        setAcceptHoverEvents(true);
    }
    Editor *editor = nullptr;
    Map* map = nullptr;
    virtual void draw();
signals:
    void borderMetatilesChanged();
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent*);
};

class CurrentSelectedMetatilesPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    CurrentSelectedMetatilesPixmapItem(Map *map_, Editor *editor_) {
        map = map_;
        editor = editor_;
    }
    Map* map = nullptr;
    Editor *editor = nullptr;
    virtual void draw();
};

#endif // EDITOR_H
