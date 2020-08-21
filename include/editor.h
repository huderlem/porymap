#ifndef EDITOR_H
#define EDITOR_H

#include <QGraphicsScene>
#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemAnimation>
#include <QComboBox>
#include <QCheckBox>
#include <QCursor>
#include <QUndoGroup>

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
    void duplicateSelectedEvents();
    void redrawObject(DraggablePixmapItem *item);
    QList<DraggablePixmapItem *> getObjects();

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
    CurrentSelectedMetatilesPixmapItem *current_metatile_selection_item = nullptr;
    MovementPermissionsSelector *movement_permissions_selector_item = nullptr;

    QList<DraggablePixmapItem*> *events = nullptr;
    QList<DraggablePixmapItem*> *selected_events = nullptr;

    QString map_edit_mode = "paint";
    QString obj_edit_mode = "select";

    int scale_exp = 0;
    double scale_base = sqrt(2); // adjust scale factor with this
    qreal collisionOpacity = 0.5;

    void objectsView_onMousePress(QMouseEvent *event);
    void objectsView_onMouseMove(QMouseEvent *event);
    void objectsView_onMouseRelease(QMouseEvent *event);

    int getBorderDrawDistance(int dimension);

    QUndoGroup editGroup; // Manages the undo history for each map

    bool selectingEvent = false;

    void shouldReselectEvents();

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
    QString getMetatileDisplayMessage(uint16_t metatileId);
    bool eventLimitReached(Map *, QString);

private slots:
    void onMapStartPaint(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void onMapEndPaint(QGraphicsSceneMouseEvent *event, MapPixmapItem *item);
    void setSmartPathCursorMode(QGraphicsSceneMouseEvent *event);
    void setStraightPathCursorMode(QGraphicsSceneMouseEvent *event);
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
    void wildMonDataChanged();
    void warpEventDoubleClicked(QString mapName, QString warpNum);
    void currentMetatilesSelectionChanged();
    void wheelZoom(int delta);
};

#endif // EDITOR_H
