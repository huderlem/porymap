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
#include "bordermetatilespixmapitem.h"
#include "connectionpixmapitem.h"
#include "currentselectedmetatilespixmapitem.h"
#include "collisionpixmapitem.h"
#include "mappixmapitem.h"
#include "settings.h"
#include "movablerect.h"
#include "cursortilerect.h"

class DraggablePixmapItem;
class MetatilesPixmapItem;

class Editor : public QObject
{
    Q_OBJECT
public:
    Editor(Ui::MainWindow* ui);
    ~Editor();

    Editor() = delete;
    Editor(const Editor &) = delete;
    Editor & operator = (const Editor &) = delete;

public:
    Ui::MainWindow* ui;
    QObject *parent = nullptr;
    Project *project = nullptr;
    Map *map = nullptr;
    Settings *settings;
    void saveProject();
    void save();
    void undo();
    void redo();
    void closeProject();
    bool setMap(QString map_name);
    void saveUiFields();
    void saveEncounterTabData();
    bool displayMap();
    void displayMetatileSelector();
    void displayMapMetatiles();
    void displayMapMovementPermissions();
    void displayBorderMetatiles();
    void displayCurrentMetatilesSelection();
    void redrawCurrentMetatilesSelection();
    void displayMovementPermissionSelector();
    void displayElevationMetatiles();
    void displayMapEvents();
    void displayMapConnections();
    void displayMapBorder();
    void displayMapGrid();
    void displayWildMonTables();

    void updateMapBorder();
    void updateMapConnections();

    void setEditingMap();
    void setEditingCollision();
    void setEditingObjects();
    void setEditingConnections();
    void setMapEditingButtonsEnabled(bool enabled);
    void clearWildMonTabWidgets();
    void setCurrentConnectionDirection(QString curDirection);
    void updateCurrentConnectionDirection(QString curDirection);
    void setConnectionsVisibility(bool visible);
    void updateConnectionOffset(int offset);
    void setConnectionMap(QString mapName);
    void addNewConnection();
    void removeCurrentConnection();
    void addNewWildMonGroup(QWidget *window);
    void updateDiveMap(QString mapName);
    void updateEmergeMap(QString mapName);
    void setSelectedConnectionFromMap(QString mapName);
    void updatePrimaryTileset(QString tilesetLabel, bool forceLoad = false);
    void updateSecondaryTileset(QString tilesetLabel, bool forceLoad = false);
    void toggleBorderVisibility(bool visible);
    void updateCustomMapHeaderValues(QTableWidget *);
    void configureEncounterJSON(QWidget *);
    Tileset *getCurrentMapPrimaryTileset();

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
    MovableRect *playerViewRect = nullptr;
    CursorTileRect *cursorMapTileRect = nullptr;

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

    int scale_exp = 0;
    double scale_base = sqrt(2); // adjust scale factor with this
    qreal collisionOpacity = 0.5;

    void objectsView_onMousePress(QMouseEvent *event);
    void objectsView_onMouseMove(QMouseEvent *event);
    void objectsView_onMouseRelease(QMouseEvent *event);

    int getBorderDrawDistance(int dimension);

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
    void updateEncounterFields(EncounterFields newFields);
    Event* createNewObjectEvent();
    Event* createNewWarpEvent();
    Event* createNewHealLocationEvent();
    Event* createNewTriggerEvent();
    Event* createNewWeatherTriggerEvent();
    Event* createNewSignEvent();
    Event* createNewHiddenItemEvent();
    Event* createNewSecretBaseEvent();
    QString getMovementPermissionText(uint16_t collision, uint16_t elevation);

private slots:
    void onMapStartPaint(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void onMapEndPaint(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void setSmartPathCursorMode(QGraphicsSceneMouseEvent *event);
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
    void wildMonDataChanged();
    void warpEventDoubleClicked(QString mapName, QString warpNum);
    void currentMetatilesSelectionChanged();
    void wheelZoom(int delta);
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
        if (editor->selected_events && editor->selected_events->contains(this)) {
            setZValue(event->y() + 1);
        } else {
            setZValue(event->y());
        }
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
    void bindToUserData(QComboBox *combo, QString key) {
        connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, [this, combo, key](int index) {
            this->event->put(key, combo->itemData(index).toString());
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

#endif // EDITOR_H
