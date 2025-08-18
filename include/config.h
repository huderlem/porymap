#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QObject>
#include <QByteArrayList>
#include <QSize>
#include <QKeySequence>
#include <QMultiMap>
#include <QDateTime>
#include <QUrl>
#include <QVersionNumber>
#include <QGraphicsPixmapItem>
#include <QFontDatabase>
#include <QStandardPaths>
#include <set>

#include "events.h"
#include "gridsettings.h"

extern const QVersionNumber porymapVersion;

// Distance in pixels from the edge of a GBA screen (240x160) to the center 16x16 pixels.
#define GBA_H_DIST_TO_CENTER ((240-16)/2)
#define GBA_V_DIST_TO_CENTER ((160-16)/2)

#define CONFIG_BACKWARDS_COMPATABILITY

enum ScriptAutocompleteMode {
    MapOnly,
    MapAndCommon,
    All,
};

class KeyValueConfigBase
{
public:
    bool save();
    bool load(const QString &dir = QString());

    void setRoot(const QString &dir);
    QString root() const { return m_root; }
    QString filepath() const { return m_filepath; }
    QString filename() const { return m_filename; }

    explicit KeyValueConfigBase(const QString &filename)
        : m_root(QString()),
          m_filename(filename),
          m_filepath(filename)
    { };
    virtual ~KeyValueConfigBase() {};
    virtual void reset() = 0;
protected:
    virtual void parseConfigKeyValue(QString key, QString value) = 0;
    virtual QMap<QString, QString> getKeyValueMap() = 0;
    virtual void init() = 0;
    virtual void setUnreadKeys() = 0;

    static bool getConfigBool(const QString &key, const QString &value);
    static int getConfigInteger(const QString &key, const QString &value, int min = INT_MIN, int max = INT_MAX, int defaultValue = 0);
    static uint32_t getConfigUint32(const QString &key, const QString &value, uint32_t min = 0, uint32_t max = UINT_MAX, uint32_t defaultValue = 0);
    static QColor getConfigColor(const QString &key, const QString &value, const QColor &defaultValue = QColor(Qt::black));
    static QString toConfigColor(const QColor &color);

    QString m_root;
    QString m_filename;
    QString m_filepath;
};

class PorymapConfig: public KeyValueConfigBase
{
public:
    PorymapConfig();
    virtual void reset() override;
    void addRecentProject(QString project);
    void setRecentProjects(QStringList projects);
    QString getRecentProject();
    QStringList getRecentProjects();
    void setMainGeometry(QByteArray, QByteArray, QByteArray, QByteArray, QByteArray);
    void setTilesetEditorGeometry(QByteArray, QByteArray, QByteArray);
    void setPaletteEditorGeometry(QByteArray, QByteArray);
    void setRegionMapEditorGeometry(QByteArray, QByteArray);
    void setProjectSettingsEditorGeometry(QByteArray, QByteArray);
    void setCustomScriptsEditorGeometry(QByteArray, QByteArray);
    QMap<QString, QByteArray> getMainGeometry();
    QMap<QString, QByteArray> getTilesetEditorGeometry();
    QMap<QString, QByteArray> getPaletteEditorGeometry();
    QMap<QString, QByteArray> getRegionMapEditorGeometry();
    QMap<QString, QByteArray> getProjectSettingsEditorGeometry();
    QMap<QString, QByteArray> getCustomScriptsEditorGeometry();

    static QFont defaultMapListFont() { return QFontDatabase::systemFont(QFontDatabase::FixedFont); }

    bool reopenOnLaunch;
    bool projectManuallyClosed;
    int mapListTab;
    bool mapListEditGroupsEnabled;
    QMap<int, bool> mapListHideEmptyEnabled;
    bool mapListLayoutsSorted;
    bool mapListLocationsSorted;
    bool prettyCursors;
    bool mirrorConnectingMaps;
    bool showDiveEmergeMaps;
    int diveEmergeMapOpacity;
    int diveMapOpacity;
    int emergeMapOpacity;
    int collisionOpacity;
    int collisionZoom;
    int metatilesZoom;
    int tilesetEditorMetatilesZoom;
    int tilesetEditorTilesZoom;
    Qt::Orientation tilesetEditorLayerOrientation;
    bool showPlayerView;
    bool showCursorTile;
    bool showBorder;
    bool showGrid;
    bool showTilesetEditorMetatileGrid;
    bool showTilesetEditorLayerGrid;
    bool showTilesetEditorDivider;
    bool showTilesetEditorRawAttributes;
    bool showPaletteEditorUnusedColors;
    bool monitorFiles;
    bool tilesetCheckerboardFill;
    bool newMapHeaderSectionExpanded;
    QString theme;
    QString wildMonChartTheme;
    QString textEditorOpenFolder;
    QString textEditorGotoLine;
    int paletteEditorBitDepth;
    int projectSettingsTab;
    ScriptAutocompleteMode scriptAutocompleteMode;
    bool warpBehaviorWarningDisabled;
    bool eventDeleteWarningDisabled;
    bool eventOverlayEnabled;
    bool checkForUpdates;
    QDateTime lastUpdateCheckTime;
    QVersionNumber lastUpdateCheckVersion;
    QMap<QUrl, QDateTime> rateLimitTimes;
    QGraphicsPixmapItem::ShapeMode eventSelectionShapeMode;
    QByteArray wildMonChartGeometry;
    QByteArray newMapDialogGeometry;
    QByteArray newLayoutDialogGeometry;
    bool shownInGameReloadMessage;
    GridSettings gridSettings;
    // Prefer over QSet to prevent shuffling elements when writing the config file.
    std::set<LogType> statusBarLogTypes;
    QFont applicationFont;
    QFont mapListFont;

protected:
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void init() override {};
    virtual void setUnreadKeys() override {};

private:
    QString stringFromByteArray(const QByteArray&);
    QByteArray bytesFromString(const QString&);

    QStringList recentProjects;
    QByteArray mainWindowGeometry;
    QByteArray mainWindowState;
    QByteArray mapSplitterState;
    QByteArray mainSplitterState;
    QByteArray metatilesSplitterState;
    QByteArray tilesetEditorGeometry;
    QByteArray tilesetEditorState;
    QByteArray tilesetEditorSplitterState;
    QByteArray paletteEditorGeometry;
    QByteArray paletteEditorState;
    QByteArray regionMapEditorGeometry;
    QByteArray regionMapEditorState;
    QByteArray projectSettingsEditorGeometry;
    QByteArray projectSettingsEditorState;
    QByteArray customScriptsEditorGeometry;
    QByteArray customScriptsEditorState;
};

extern PorymapConfig porymapConfig;

enum BaseGameVersion {
    none,
    pokeruby,
    pokefirered,
    pokeemerald,
};

enum ProjectIdentifier {
    symbol_facing_directions,
    symbol_obj_event_gfx_pointers,
    symbol_pokemon_icon_table,
    symbol_attribute_table,
    symbol_tilesets_prefix,
    symbol_dynamic_map_name,
    define_obj_event_count,
    define_min_level,
    define_max_level,
    define_max_encounter_rate,
    define_tiles_primary,
    define_tiles_total,
    define_metatiles_primary,
    define_pals_primary,
    define_pals_total,
    define_tiles_per_metatile,
    define_map_size,
    define_map_offset_width,
    define_map_offset_height,
    define_mask_metatile,
    define_mask_collision,
    define_mask_elevation,
    define_mask_behavior,
    define_mask_layer,
    define_attribute_behavior,
    define_attribute_layer,
    define_attribute_terrain,
    define_attribute_encounter,
    define_metatile_label_prefix,
    define_heal_locations_prefix,
    define_layout_prefix,
    define_map_prefix,
    define_map_dynamic,
    define_map_empty,
    define_map_section_prefix,
    define_map_section_empty,
    define_species_prefix,
    define_species_empty,
    regex_behaviors,
    regex_obj_event_gfx,
    regex_items,
    regex_flags,
    regex_vars,
    regex_movement_types,
    regex_map_types,
    regex_battle_scenes,
    regex_weather,
    regex_coord_event_weather,
    regex_secret_bases,
    regex_sign_facing_directions,
    regex_trainer_types,
    regex_music,
    regex_encounter_types,
    regex_terrain_types,
    pals_output_extension,
    tiles_output_extension,
};

enum ProjectFilePath {
    data_map_folders,
    data_scripts_folders,
    data_layouts_folders,
    data_primary_tilesets_folders,
    data_secondary_tilesets_folders,
    data_event_scripts,
    json_map_groups,
    json_layouts,
    json_wild_encounters,
    json_heal_locations,
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
    constants_global,
    constants_items,
    constants_flags,
    constants_vars,
    constants_weather,
    constants_songs,
    constants_pokemon,
    constants_map_types,
    constants_trainer_types,
    constants_secret_bases,
    constants_obj_event_movement,
    constants_obj_events,
    constants_event_bg,
    constants_metatile_labels,
    constants_metatile_behaviors,
    constants_species,
    constants_fieldmap,
    global_fieldmap,
    fieldmap,
    initial_facing_table,
    wild_encounter,
    pokemon_icon_table,
    pokemon_gfx,
};

class ProjectConfig: public KeyValueConfigBase
{
public:
    ProjectConfig();
    virtual void reset() override {
        this->baseGameVersion = BaseGameVersion::pokeemerald;
        // Reset non-version-specific settings
        this->usePoryScript = false;
        this->tripleLayerMetatilesEnabled = false;
        this->defaultMetatileId = 1;
        this->defaultElevation = 3;
        this->defaultCollision = 0;
        this->defaultMapSize = QSize(20,20);
        this->defaultPrimaryTileset = "gTileset_General";
        this->prefabFilepath = QString();
        this->prefabImportPrompted = false;
        this->tilesetsHaveCallback = true;
        this->tilesetsHaveIsCompressed = true;
        this->transparencyColor = QColor(Qt::black);
        this->preserveMatchingOnlyData = false;
        this->filePaths.clear();
        this->eventIconPaths.clear();
        this->pokemonIconPaths.clear();
        this->eventsTabIconPath = QString();
        this->collisionSheetPath = QString();
        this->collisionSheetSize = QSize(2, 16);
        this->playerViewDistance = QMargins(GBA_H_DIST_TO_CENTER, GBA_V_DIST_TO_CENTER, GBA_H_DIST_TO_CENTER, GBA_V_DIST_TO_CENTER);
        this->blockMetatileIdMask = 0x03FF;
        this->blockCollisionMask = 0x0C00;
        this->blockElevationMask = 0xF000;
        this->unusedTileNormal = 0x3014;
        this->unusedTileCovered = 0x0000;
        this->unusedTileSplit = 0x0000;
        this->maxEventsPerGroup = 255;
        this->forcedMajorVersion = 0;
        this->metatileSelectorWidth = 8;
        this->globalConstantsFilepaths.clear();
        this->globalConstants.clear();
        this->identifiers.clear();
        this->readKeys.clear();
    }
    static const QMap<ProjectIdentifier, QPair<QString, QString>> defaultIdentifiers;
    static const QMap<ProjectFilePath, QPair<QString, QString>> defaultPaths;
    static const QStringList versionStrings;
    static BaseGameVersion stringToBaseGameVersion(const QString &string);

    static QString getPlayerIconPath(BaseGameVersion baseGameVersion, int character);
    static QIcon getPlayerIcon(BaseGameVersion baseGameVersion, int character);

    QString projectDir() const { return m_root; } // Alias for root()
    void reset(BaseGameVersion baseGameVersion);
    void setFilePath(ProjectFilePath pathId, const QString &path);
    void setFilePath(const QString &pathId, const QString &path);
    QString getCustomFilePath(ProjectFilePath pathId);
    QString getCustomFilePath(const QString &pathId);
    QString getFilePath(ProjectFilePath pathId);
    void setIdentifier(ProjectIdentifier id, QString text);
    void setIdentifier(const QString &id, const QString &text);
    QString getCustomIdentifier(ProjectIdentifier id);
    QString getCustomIdentifier(const QString &id);
    QString getIdentifier(ProjectIdentifier id);
    QString getBaseGameVersionString(BaseGameVersion version);
    QString getBaseGameVersionString();
    int getNumLayersInMetatile();
    int getNumTilesInMetatile();
    void setEventIconPath(Event::Group group, const QString &path);
    QString getEventIconPath(Event::Group group);
    void setPokemonIconPath(const QString &species, const QString &path);
    QString getPokemonIconPath(const QString &species);
    QMap<QString, QString> getPokemonIconPaths();

    BaseGameVersion baseGameVersion;
    bool usePoryScript;
    bool useCustomBorderSize;
    bool eventWeatherTriggerEnabled;
    bool eventSecretBaseEnabled;
    bool hiddenItemQuantityEnabled;
    bool hiddenItemRequiresItemfinderEnabled;
    bool healLocationRespawnDataEnabled;
    bool eventCloneObjectEnabled;
    bool floorNumberEnabled;
    bool createMapTextFileEnabled;
    bool tripleLayerMetatilesEnabled;
    uint16_t defaultMetatileId;
    uint16_t defaultElevation;
    uint16_t defaultCollision;
    QSize defaultMapSize;
    QList<uint16_t> newMapBorderMetatileIds;
    QString defaultPrimaryTileset;
    QString defaultSecondaryTileset;
    QString prefabFilepath;
    bool prefabImportPrompted;
    bool tilesetsHaveCallback;
    bool tilesetsHaveIsCompressed;
    QColor transparencyColor;
    bool preserveMatchingOnlyData;
    int metatileAttributesSize;
    uint32_t metatileBehaviorMask;
    uint32_t metatileTerrainTypeMask;
    uint32_t metatileEncounterTypeMask;
    uint32_t metatileLayerTypeMask;
    uint16_t blockMetatileIdMask;
    uint16_t blockCollisionMask;
    uint16_t blockElevationMask;
    uint16_t unusedTileNormal;
    uint16_t unusedTileCovered;
    uint16_t unusedTileSplit;
    bool mapAllowFlagsEnabled;
    QString eventsTabIconPath;
    QString collisionSheetPath;
    QSize collisionSheetSize;
    QMargins playerViewDistance;
    QList<uint32_t> warpBehaviors;
    int maxEventsPerGroup;
    int forcedMajorVersion;
    int metatileSelectorWidth;
    QStringList globalConstantsFilepaths;
    QMap<QString,QString> globalConstants;

protected:
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void init() override;
    virtual void setUnreadKeys() override;

private:
    QStringList readKeys;
    QMap<ProjectIdentifier, QString> identifiers;
    QMap<ProjectFilePath, QString> filePaths;
    QMap<Event::Group, QString> eventIconPaths;
    QMap<QString, QString> pokemonIconPaths;
};

extern ProjectConfig projectConfig;

class UserConfig: public KeyValueConfigBase
{
public:
    UserConfig();
    virtual void reset() override {
        this->recentMapOrLayout = QString();
        this->useEncounterJson = true;
        this->customScripts.clear();
        this->readKeys.clear();
    }

    QString projectDir() const { return m_root; } // Alias for root()
    void parseCustomScripts(QString input);
    QString outputCustomScripts();
    void setCustomScripts(QStringList scripts, QList<bool> enabled);
    QStringList getCustomScriptPaths();
    QList<bool> getCustomScriptsEnabled();

    QString recentMapOrLayout;
    bool useEncounterJson;

protected:
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void init() override;
    virtual void setUnreadKeys() override;
#ifdef CONFIG_BACKWARDS_COMPATABILITY
    friend class ProjectConfig;
#endif

private:
    QStringList readKeys;
    QMap<QString, bool> customScripts;
};

extern UserConfig userConfig;

class QAction;
class Shortcut;

class ShortcutsConfig : public KeyValueConfigBase
{
public:
    ShortcutsConfig();

    virtual void reset() override {
        setRoot(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        user_shortcuts.clear();
    }

    // Call this before applying user shortcuts so that the user can restore defaults.
    void setDefaultShortcuts(const QObjectList &objects);
    QList<QKeySequence> defaultShortcuts(const QObject *object) const;

    void setUserShortcuts(const QObjectList &objects);
    void setUserShortcuts(const QMultiMap<const QObject *, QKeySequence> &objects_keySequences);
    QList<QKeySequence> userShortcuts(const QObject *object) const;

protected:
    virtual void parseConfigKeyValue(QString key, QString value) override;
    virtual QMap<QString, QString> getKeyValueMap() override;
    virtual void init() override { };
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
