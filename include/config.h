#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QObject>
#include <QByteArrayList>
#include <QSize>
#include <QKeySequence>
#include <QMultiMap>

// In both versions the default new map border is a generic tree
#define DEFAULT_BORDER_RSE (QList<int>{468, 469, 476, 477})
#define DEFAULT_BORDER_FRLG (QList<int>{20, 21, 28, 29})

#define CONFIG_BACKWARDS_COMPATABILITY

enum MapSortOrder {
    Group   =  0,
    Area    =  1,
    Layout  =  2,
};

class KeyValueConfigBase
{
public:
    void save();
    void load();
    virtual ~KeyValueConfigBase();
    virtual void reset() = 0;
protected:
    virtual QString getConfigFilepath() = 0;
    virtual void parseConfigKeyValue(QString key, QString value) = 0;
    virtual QMap<QString, QString> getKeyValueMap() = 0;
    virtual void onNewConfigFileCreated() = 0;
    virtual void setUnreadKeys() = 0;
    bool getConfigBool(QString key, QString value);
    int getConfigInteger(QString key, QString value, int min, int max, int defaultValue);
    uint32_t getConfigUint32(QString key, QString value, uint32_t min, uint32_t max, uint32_t defaultValue);
};

class PorymapConfig: public KeyValueConfigBase
{
public:
    PorymapConfig() {
        reset();
    }
    virtual void reset() override {
        this->recentProject = "";
        this->reopenOnLaunch = true;
        this->mapSortOrder = MapSortOrder::Group;
        this->prettyCursors = true;
        this->collisionOpacity = 50;
        this->metatilesZoom = 30;
        this->showPlayerView = false;
        this->showCursorTile = true;
        this->showBorder = true;
        this->showGrid = false;
        this->monitorFiles = true;
        this->tilesetCheckerboardFill = true;
        this->theme = "default";
        this->textEditorOpenFolder = "";
        this->textEditorGotoLine = "";
    }
    void setRecentProject(QString project);
    void setReopenOnLaunch(bool enabled);
    void setMapSortOrder(MapSortOrder order);
    void setPrettyCursors(bool enabled);
    void setMainGeometry(QByteArray, QByteArray, QByteArray, QByteArray);
    void setTilesetEditorGeometry(QByteArray, QByteArray);
    void setPaletteEditorGeometry(QByteArray, QByteArray);
    void setRegionMapEditorGeometry(QByteArray, QByteArray);
    void setCollisionOpacity(int opacity);
    void setMetatilesZoom(int zoom);
    void setShowPlayerView(bool enabled);
    void setShowCursorTile(bool enabled);
    void setShowBorder(bool enabled);
    void setShowGrid(bool enabled);
    void setMonitorFiles(bool monitor);
    void setTilesetCheckerboardFill(bool checkerboard);
    void setTheme(QString theme);
    void setTextEditorOpenFolder(const QString &command);
    void setTextEditorGotoLine(const QString &command);
    void setPaletteEditorBitDepth(int bitDepth);
    QString getRecentProject();
    bool getReopenOnLaunch();
    MapSortOrder getMapSortOrder();
    bool getPrettyCursors();
    QMap<QString, QByteArray> getMainGeometry();
    QMap<QString, QByteArray> getTilesetEditorGeometry();
    QMap<QString, QByteArray> getPaletteEditorGeometry();
    QMap<QString, QByteArray> getRegionMapEditorGeometry();
    int getCollisionOpacity();
    int getMetatilesZoom();
    bool getShowPlayerView();
    bool getShowCursorTile();
    bool getShowBorder();
    bool getShowGrid();
    bool getMonitorFiles();
    bool getTilesetCheckerboardFill();
    QString getTheme();
    QString getTextEditorOpenFolder();
    QString getTextEditorGotoLine();
    int getPaletteEditorBitDepth();
protected:
    virtual QString getConfigFilepath() override;
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void onNewConfigFileCreated() override {};
    virtual void setUnreadKeys() override {};
private:
    QString recentProject;
    bool reopenOnLaunch;
    QString stringFromByteArray(QByteArray);
    QByteArray bytesFromString(QString);
    MapSortOrder mapSortOrder;
    bool prettyCursors;
    QByteArray mainWindowGeometry;
    QByteArray mainWindowState;
    QByteArray mapSplitterState;
    QByteArray eventsSlpitterState;
    QByteArray mainSplitterState;
    QByteArray tilesetEditorGeometry;
    QByteArray tilesetEditorState;
    QByteArray paletteEditorGeometry;
    QByteArray paletteEditorState;
    QByteArray regionMapEditorGeometry;
    QByteArray regionMapEditorState;
    int collisionOpacity;
    int metatilesZoom;
    bool showPlayerView;
    bool showCursorTile;
    bool showBorder;
    bool showGrid;
    bool monitorFiles;
    bool tilesetCheckerboardFill;
    QString theme;
    QString textEditorOpenFolder;
    QString textEditorGotoLine;
    int paletteEditorBitDepth;
};

extern PorymapConfig porymapConfig;

enum BaseGameVersion {
    pokeruby,
    pokefirered,
    pokeemerald,
};

enum ProjectFilePath {
    data_map_folders,
    data_scripts_folders,
    data_layouts_folders,
    data_tilesets_folders,
    data_event_scripts,
    json_map_groups,
    json_layouts,
    json_wild_encounters,
    json_region_map_entries,
    json_region_porymap_cfg,
    tilesets_headers,
    tilesets_graphics,
    tilesets_metatiles,
    tilesets_headers_asm,
    tilesets_graphics_asm,
    tilesets_metatiles_asm,
    data_obj_event_gfx_pointers,
    data_obj_event_gfx_info,
    data_obj_event_pic_tables,
    data_obj_event_gfx,
    data_pokemon_gfx,
    data_heal_locations,
    constants_global,
    constants_map_groups,
    constants_items,
    constants_opponents,
    constants_flags,
    constants_vars,
    constants_weather,
    constants_songs,
    constants_heal_locations,
    constants_pokemon,
    constants_map_types,
    constants_trainer_types,
    constants_secret_bases,
    constants_obj_event_movement,
    constants_obj_events,
    constants_event_bg,
    constants_region_map_sections,
    constants_metatile_labels,
    constants_metatile_behaviors,
    constants_fieldmap,
    initial_facing_table,
    pokemon_icon_table,
};

class ProjectConfig: public KeyValueConfigBase
{
public:
    ProjectConfig() {
        reset();
    }
    virtual void reset() override {
        this->baseGameVersion = BaseGameVersion::pokeemerald;
        this->useCustomBorderSize = false;
        this->enableEventWeatherTrigger = true;
        this->enableEventSecretBase = true;
        this->enableHiddenItemQuantity = false;
        this->enableHiddenItemRequiresItemfinder = false;
        this->enableHealLocationRespawnData = false;
        this->enableEventCloneObject = false;
        this->enableFloorNumber = false;
        this->createMapTextFile = false;
        this->usePoryScript = false;
        this->enableTripleLayerMetatiles = false;
        this->newMapMetatileId = 1;
        this->newMapElevation = 3;
        this->newMapBorderMetatileIds = DEFAULT_BORDER_RSE;
        this->defaultPrimaryTileset = "gTileset_General";
        this->prefabFilepath = QString();
        this->prefabImportPrompted = false;
        this->tilesetsHaveCallback = true;
        this->tilesetsHaveIsCompressed = true;
        this->filePaths.clear();
        this->readKeys.clear();
    }
    void setBaseGameVersion(BaseGameVersion baseGameVersion);
    BaseGameVersion getBaseGameVersion();
    QString getBaseGameVersionString();
    void setUsePoryScript(bool usePoryScript);
    bool getUsePoryScript();
    void setProjectDir(QString projectDir);
    QString getProjectDir();
    void setUseCustomBorderSize(bool enable);
    bool getUseCustomBorderSize();
    void setEventWeatherTriggerEnabled(bool enable);
    bool getEventWeatherTriggerEnabled();
    void setEventSecretBaseEnabled(bool enable);
    bool getEventSecretBaseEnabled();
    void setHiddenItemQuantityEnabled(bool enable);
    bool getHiddenItemQuantityEnabled();
    void setHiddenItemRequiresItemfinderEnabled(bool enable);
    bool getHiddenItemRequiresItemfinderEnabled();
    void setHealLocationRespawnDataEnabled(bool enable);
    bool getHealLocationRespawnDataEnabled();
    void setEventCloneObjectEnabled(bool enable);
    bool getEventCloneObjectEnabled();
    void setFloorNumberEnabled(bool enable);
    bool getFloorNumberEnabled();
    void setCreateMapTextFileEnabled(bool enable);
    bool getCreateMapTextFileEnabled();
    void setTripleLayerMetatilesEnabled(bool enable);
    bool getTripleLayerMetatilesEnabled();
    int getNumLayersInMetatile();
    int getNumTilesInMetatile();
    void setNewMapMetatileId(int metatileId);
    int getNewMapMetatileId();
    void setNewMapElevation(int elevation);
    int getNewMapElevation();
    void setNewMapBorderMetatileIds(QList<int> metatileIds);
    QList<int> getNewMapBorderMetatileIds();
    QString getDefaultPrimaryTileset();
    QString getDefaultSecondaryTileset();
    void setFilePath(ProjectFilePath pathId, QString path);
    QString getFilePath(ProjectFilePath pathId);
    void setPrefabFilepath(QString filepath);
    QString getPrefabFilepath(bool setIfEmpty);
    void setPrefabImportPrompted(bool prompted);
    bool getPrefabImportPrompted();
    void setTilesetsHaveCallback(bool has);
    bool getTilesetsHaveCallback();
    void setTilesetsHaveIsCompressed(bool has);
    bool getTilesetsHaveIsCompressed();
    int getMetatileAttributesSize();
    uint32_t getMetatileBehaviorMask();
    uint32_t getMetatileTerrainTypeMask();
    uint32_t getMetatileEncounterTypeMask();
    uint32_t getMetatileLayerTypeMask();
    bool getMapAllowFlagsEnabled();
protected:
    virtual QString getConfigFilepath() override;
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void onNewConfigFileCreated() override;
    virtual void setUnreadKeys() override;
private:
    BaseGameVersion baseGameVersion;
    QString projectDir;
    QMap<ProjectFilePath, QString> filePaths;
    bool usePoryScript;
    bool useCustomBorderSize;
    bool enableEventWeatherTrigger;
    bool enableEventSecretBase;
    bool enableHiddenItemQuantity;
    bool enableHiddenItemRequiresItemfinder;
    bool enableHealLocationRespawnData;
    bool enableEventCloneObject;
    bool enableFloorNumber;
    bool createMapTextFile;
    bool enableTripleLayerMetatiles;
    int newMapMetatileId;
    int newMapElevation;
    QList<int> newMapBorderMetatileIds;
    QString defaultPrimaryTileset;
    QString defaultSecondaryTileset;
    QStringList readKeys;
    QString prefabFilepath;
    bool prefabImportPrompted;
    bool tilesetsHaveCallback;
    bool tilesetsHaveIsCompressed;
    int metatileAttributesSize;
    uint32_t metatileBehaviorMask;
    uint32_t metatileTerrainTypeMask;
    uint32_t metatileEncounterTypeMask;
    uint32_t metatileLayerTypeMask;
    bool enableMapAllowFlags;
};

extern ProjectConfig projectConfig;

class UserConfig: public KeyValueConfigBase
{
public:
    UserConfig() {
        reset();
    }
    virtual void reset() override {
        this->recentMap = QString();
        this->useEncounterJson = true;
        this->customScripts.clear();
        this->readKeys.clear();
    }
    void setRecentMap(const QString &map);
    QString getRecentMap();
    void setEncounterJsonActive(bool active);
    bool getEncounterJsonActive();
    void setProjectDir(QString projectDir);
    QString getProjectDir();
    void setCustomScripts(QList<QString> scripts);
    QList<QString> getCustomScripts();
protected:
    virtual QString getConfigFilepath() override;
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void onNewConfigFileCreated() override;
    virtual void setUnreadKeys() override;
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    friend class ProjectConfig;
#endif    
private:
    QString projectDir;
    QString recentMap;
    bool useEncounterJson;
    QList<QString> customScripts;
    QStringList readKeys;
};

extern UserConfig userConfig;

class QAction;
class Shortcut;

class ShortcutsConfig : public KeyValueConfigBase
{
public:
    ShortcutsConfig() :
        user_shortcuts({ }),
        default_shortcuts({ })
    { }

    virtual void reset() override { user_shortcuts.clear(); }

    // Call this before applying user shortcuts so that the user can restore defaults.
    void setDefaultShortcuts(const QObjectList &objects);
    QList<QKeySequence> defaultShortcuts(const QObject *object) const;

    void setUserShortcuts(const QObjectList &objects);
    void setUserShortcuts(const QMultiMap<const QObject *, QKeySequence> &objects_keySequences);
    QList<QKeySequence> userShortcuts(const QObject *object) const;

protected:
    virtual QString getConfigFilepath() override;
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void onNewConfigFileCreated() override { };
    virtual void setUnreadKeys() override { };

private:
    QMultiMap<QString, QKeySequence> user_shortcuts;
    QMultiMap<QString, QKeySequence> default_shortcuts;

    enum StoreType {
        User,
        Default
    };

    QString cfgKey(const QObject *object) const;
    QList<QKeySequence> currentShortcuts(const QObject *object) const;

    void storeShortcutsFromList(StoreType storeType, const QObjectList &objects);
    void storeShortcuts(
            StoreType storeType,
            const QString &cfgKey,
            const QList<QKeySequence> &keySequences);
};

extern ShortcutsConfig shortcutsConfig;

#endif // CONFIG_H
