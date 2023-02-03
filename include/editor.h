#pragma once
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
#include "layoutpixmapitem.h"
#include "settings.h"
#include "movablerect.h"
#include "cursortilerect.h"
#include "mapruler.h"

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
    Layout *layout = nullptr; /* NEW */

    QUndoGroup editGroup; // Manages the undo history for each map

    Settings *settings;

    void save();
    void saveProject();
    void saveUiFields();
    void saveEncounterTabData();

    void closeProject();

    bool setMap(QString map_name);
    bool setLayout(QString layoutName);
    void unsetMap();

    Tileset *getCurrentMapPrimaryTileset();

    bool displayMap();
    bool displayLayout();

    void displayMetatileSelector();
    void displayMapMetatiles();
    void displayMapMovementPermissions();
    void displayBorderMetatiles();
    void displayCurrentMetatilesSelection();
    void redrawCurrentMetatilesSelection();
    void displayMovementPermissionSelector();
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

    void addNewWildMonGroup(QWidget *window);
    void deleteWildMonGroup();
    void configureEncounterJSON(QWidget *);

    void updatePrimaryTileset(QString tilesetLabel, bool forceLoad = false);
    void updateSecondaryTileset(QString tilesetLabel, bool forceLoad = false);
    void toggleBorderVisibility(bool visible, bool enableScriptCallback = true);
    void updateCustomMapHeaderValues(QTableWidget *);

    DraggablePixmapItem *addMapEvent(Event *event);
    bool eventLimitReached(Map *, Event::Type);
    void selectMapEvent(DraggablePixmapItem *object);
    void selectMapEvent(DraggablePixmapItem *object, bool toggle);
    DraggablePixmapItem *addNewEvent(Event::Type type);
    void updateSelectedEvents();
    void duplicateSelectedEvents();
    void redrawObject(DraggablePixmapItem *item);
    QList<DraggablePixmapItem *> getObjects();

    void updateCursorRectPos(int x, int y);
    void setCursorRectVisible(bool visible);



    QGraphicsScene *scene = nullptr;
    QGraphicsPixmapItem *current_view = nullptr;
    LayoutPixmapItem *map_item = nullptr;
    ConnectionPixmapItem* selected_connection_item = nullptr;
    QList<ConnectionPixmapItem*> connection_items;
    QGraphicsPathItem *connection_mask = nullptr;
    CollisionPixmapItem *collision_item = nullptr;
    QGraphicsItemGroup *events_group = nullptr;

    QList<QGraphicsPixmapItem*> borderItems;
    QList<QGraphicsLineItem*> gridLines;
    MapRuler *map_ruler = nullptr;

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

    QList<DraggablePixmapItem *> *selected_events = nullptr;

    enum class EditAction { None, Paint, Select, Fill, Shift, Pick, Move };
    EditAction mapEditAction = EditAction::Paint;
    EditAction objectEditAction = EditAction::Select;

    /// !TODO this
    enum class EditMode { None, Map, Layout };
    EditMode editMode = EditMode::Map;

    int scaleIndex = 2;
    qreal collisionOpacity = 0.5;

    void objectsView_onMousePress(QMouseEvent *event);

    int getBorderDrawDistance(int dimension);

    bool selectingEvent = false;

    void shouldReselectEvents();
    void scaleMapView(int);
    static void openInTextEditor(const QString &path, int lineNum = 0);
    bool eventLimitReached(Event::Type type);

public slots:
    void openMapScripts() const;
    void openScript(const QString &scriptLabel) const;
    void openProjectInTextEditor() const;
    void maskNonVisibleConnectionTiles();
    void onBorderMetatilesChanged();
    void selectedEventIndexChanged(int index, Event::Group eventGroup);

private:
    void setConnectionItemsVisible(bool);
    void setBorderItemsVisible(bool, qreal = 1);
    void setConnectionEditControlValues(MapConnection*);
    void setConnectionEditControlsEnabled(bool);
    void setConnectionsEditable(bool);
    void createConnectionItem(MapConnection* connection);
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
    QString getMovementPermissionText(uint16_t collision, uint16_t elevation);
    QString getMetatileDisplayMessage(uint16_t metatileId);
    static bool startDetachedProcess(const QString &command,
                                    const QString &workingDirectory = QString(),
                                    qint64 *pid = nullptr);

private slots:
    void onMapStartPaint(QGraphicsSceneMouseEvent *event, LayoutPixmapItem *item);
    void onMapEndPaint(QGraphicsSceneMouseEvent *event, LayoutPixmapItem *item);
    void setSmartPathCursorMode(QGraphicsSceneMouseEvent *event);
    void setStraightPathCursorMode(QGraphicsSceneMouseEvent *event);
    void mouseEvent_map(QGraphicsSceneMouseEvent *event, LayoutPixmapItem *item);
    void mouseEvent_collision(QGraphicsSceneMouseEvent *event, CollisionPixmapItem *item);
    void onConnectionMoved(MapConnection*);
    void onConnectionItemSelected(ConnectionPixmapItem* connectionItem);
    void onConnectionItemDoubleClicked(ConnectionPixmapItem* connectionItem);
    void onConnectionDirectionChanged(QString newDirection);
    void onHoveredMovementPermissionChanged(uint16_t, uint16_t);
    void onHoveredMovementPermissionCleared();
    void onHoveredMetatileSelectionChanged(uint16_t);
    void onHoveredMetatileSelectionCleared();
    void onHoveredMapMetatileChanged(const QPoint &pos);
    void onHoveredMapMetatileCleared();
    void onHoveredMapMovementPermissionChanged(int, int);
    void onHoveredMapMovementPermissionCleared();
    void onSelectedMetatilesChanged();
    void onWheelZoom(int);
    void onToggleGridClicked(bool);

signals:
    void objectsChanged();
    void loadMapRequested(QString, QString);
    void wildMonDataChanged();
    void warpEventDoubleClicked(QString, int, Event::Group);
    void currentMetatilesSelectionChanged();
    void mapRulerStatusChanged(const QString &);
    void editedMapData();
    void tilesetUpdated(QString);
};

#endif // EDITOR_H
