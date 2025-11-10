.. _files:

*************
Project Files
*************

Porymap relies on the user maintaining a certain level of integrity with their project files.
This is a list of files that porymap reads from and writes to. Generally, if porymap writes 
to a file, it probably is not a good idea to edit yourself unless otherwise noted.

The filepath that Porymap expects for each file can be overridden on the ``Files`` tab of ``Options > Project Settings``. A new path can be specified by entering it in the text box or choosing it with the |button-folder| button. Paths are expected to be relative to the root project folder. If no path is specified, or if the file/folder specified does not exist, then the default path will be used instead. The name of each setting in this section is listed in the table below under ``Setting``.

.. |button-folder| image:: images/scripting-capabilities/folder.png
   :width: 24
   :height: 24

.. figure:: images/settings-and-options/tab-files.png
   :alt: Files tab
   :width: 75%
   :align: center

.. csv-table::
   :header: File Name,Read,Write,Setting,Notes
   :widths: 20, 5, 5, 20, 40

   data/maps/, yes, yes, ``data_map_folders``, "expected folder to find/create map data"
   ``data_map_folders``/\*/map.json, yes, yes, ``--``, ""
   ``data_map_folders``/\*/scripts.[inc|pory], yes, no, ``--``, "for finding script labels"
   data/scripts/\*.[inc|pory], yes, no, ``data_scripts_folders``, "for finding script labels"
   data/event_scripts.s, no, yes, ``data_event_scripts``, "only appends new script files to end of file"
   data/maps/map_groups.json, yes, yes, ``json_map_groups``, "for populating the map list"
   data/layouts/layouts.json, yes, yes, ``json_layouts``, "for populating the layout list and to know where layout data is. Files specified with ``border_filepath`` or ``blockdata_filepath`` in this file (normally, ``border.bin`` and ``map.bin``) will also be read/written."
   data/layouts/, yes, yes, ``data_layouts_folders``, "the root folder for creating new layout folders with ``border.bin`` and ``map.bin`` files."
   src/data/tilesets/headers.h, yes, yes, ``tilesets_headers``, "to populate the tileset list, to know where tileset data is, and to add new tileset data."
   src/data/tilesets/graphics.h, yes, yes, ``tilesets_graphics``, "to locate tileset tiles and palettes, and to add new tileset data. The source files specified here (normally, ``tiles.png`` and ``palettes/*.pal``) will also be read/written."
   src/data/tilesets/metatiles.h, yes, yes, ``tilesets_metatiles``, "to locate tileset metatile data, and to add new tileset data. The source files specified here (normally, ``metatiles.bin`` and ``metatile_attributes.bin``) will also be read/written."
   data/tilesets/headers.inc, yes, yes, ``tilesets_headers_asm``, "old assembly format to use if ``tilesets_headers`` can't be found"
   data/tilesets/graphics.inc, yes, yes, ``tilesets_graphics_asm``, "old assembly format to use if ``tilesets_headers`` can't be found"
   data/tilesets/metatiles.inc, yes, yes, ``tilesets_metatiles_asm``, "old assembly format to use if ``tilesets_headers`` can't be found"
   data/tilesets/primary/, yes, yes, ``data_primary_tilesets_folders``, "expected folder to find/create data for primary tilesets"
   data/tilesets/secondary/, yes, yes, ``data_secondary_tilesets_folders``, "expected folder to find/create data for secondary tilesets"
   src/data/wild_encounters.json, yes, yes, ``json_wild_encounters``, "optional (only required to use Wild Pokémon tab)"
   src/data/heal_locations.json, yes, yes, ``json_heal_locations``, ""
   src/data/object_events/object_event_graphics_info_pointers.h, yes, no, ``data_obj_event_gfx_pointers``, "to read ``symbol_obj_event_gfx_pointers``"
   src/data/object_events/object_event_graphics_info.h, yes, no, ``data_obj_event_gfx_info``, "to read data about how to display object event sprites"
   src/data/object_events/object_event_pic_tables.h, yes, no, ``data_obj_event_pic_tables``, "to locate object event sprites using data from ``data_obj_event_gfx_info``"
   src/data/object_events/object_event_graphics.h, yes, no, ``data_obj_event_gfx``, "to locate object event sprites using data from ``data_obj_event_pic_tables``"
   src/data/graphics/pokemon.h, yes, no, ``data_pokemon_gfx``, "if ``symbol_pokemon_icon_table`` is read this file will be searched for filepaths to species icon"
   src/data/region_map/region_map_sections.json, yes, yes, ``json_region_map_entries``, "for populating the locations list and for region map data"
   src/data/region_map/regions.json, yes, yes, ``json_region_entries``, "For populating the regions list and for region map data"
   src/data/region_map/porymap_config.json, yes, yes, ``json_region_porymap_cfg``, "Porymap's config file for the region map editor"
   include/constants/global.h, yes, no, ``constants_global``, "to evaluate ``define_obj_event_count``"
   include/constants/items.h, yes, no, ``constants_items``, "to find ``regex_items`` names"
   include/constants/flags.h, yes, no, ``constants_flags``, "to find ``regex_flags`` names"
   include/constants/vars.h, yes, no, ``constants_vars``, "to find ``regex_vars`` names"
   include/constants/weather.h, yes, no, ``constants_weather``, "to find ``regex_weather`` names"
   include/constants/songs.h, yes, no, ``constants_songs``, "to find ``regex_music`` names"
   include/constants/pokemon.h, yes, no, ``constants_pokemon``, "to evaluate ``define_min_level`` and ``define_max_level``"
   include/constants/map_types.h, yes, no, ``constants_map_types``, "to find ``regex_map_types`` and ``regex_battle_scenes`` names"
   include/constants/trainer_types.h, yes, no, ``constants_trainer_types``, "to find ``regex_trainer_types`` names"
   include/constants/secret_bases.h, yes, no, ``constants_secret_bases``, "to find ``regex_secret_bases`` names"
   include/constants/event_object_movement.h, yes, no, ``constants_obj_event_movement``, "to find ``regex_movement_types`` names"
   include/constants/event_objects.h, yes, no, ``constants_obj_events``, "to evaluate ``regex_obj_event_gfx`` constants"
   include/constants/event_bg.h, yes, no, ``constants_event_bg``, "to find ``regex_sign_facing_directions`` names"
   include/constants/metatile_labels.h, yes, yes, ``constants_metatile_labels``, "to read/write metatile labels using ``define_metatile_label_prefix``"
   include/constants/metatile_behaviors.h, yes, no, ``constants_metatile_behaviors``, "to evaluate ``regex_behaviors`` constants"
   include/constants/species.h, yes, no, ``constants_species``, "to find names using ``define_species_prefix``"
   include/global.fieldmap.h, yes, no, ``global_fieldmap``, "to evaluate map and tileset data masks, and to read ``regex_encounter_types`` / ``regex_terrain_types``"
   include/fieldmap.h, yes, no, ``constants_fieldmap``, "to evaluate a variety of tileset and map constants"
   src/fieldmap.c, yes, no, ``fieldmap``, "to read ``symbol_attribute_table``"
   src/event_object_movement.c, yes, no, ``initial_facing_table``, "to read ``symbol_facing_directions``"
   src/wild_encounter.c, yes, no, ``wild_encounter``, "to evaluate ``define_max_encounter_rate``"
   src/pokemon_icon.c, yes, no, ``pokemon_icon_table``, "to read ``symbol_pokemon_icon_table``"
   graphics/pokemon/, yes, no, ``pokemon_gfx``, "to search for Pokémon ``icon.png`` files if they aren't found via ``symbol_pokemon_icon_table``"


Identifiers
-----------

In addition to these files, there are some specific symbol and macro names that Porymap expects to find in your project. These can be overridden on the ``Identifiers`` tab of ``Options > Project Settings``. The name of each setting in this section is listed in the table below under ``Setting``. Settings with ``regex`` in the name support the `regular expression syntax <https://perldoc.perl.org/perlre>`_ used by Qt.

.. figure:: images/settings-and-options/tab-identifiers.png
   :alt: Identifiers tab
   :width: 75%
   :align: center


.. csv-table::
   :header: Setting,Default,Notes
   :widths: 20, 20, 30

   ``symbol_facing_directions``, ``gInitialMovementTypeFacingDirections``, "to set sprite frame for Object events based on its ``Movement`` type"
   ``symbol_obj_event_gfx_pointers``, ``gObjectEventGraphicsInfoPointers``, "the array mapping ``regex_obj_event_gfx`` constants to their data"
   ``symbol_pokemon_icon_table``, ``gMonIconTable``, "to map species constants to icon images"
   ``symbol_attribute_table``, ``sMetatileAttrMasks``, "optionally read to get settings on ``Tilesets`` tab"
   ``symbol_tilesets_prefix``, ``gTileset_``, "for new tileset names and to extract base tileset names"
   ``symbol_dynamic_map_name``, ``Dynamic``, "reserved map name to display for ``define_map_dynamic``"
   ``define_obj_event_count``, ``OBJECT_EVENT_TEMPLATES_COUNT``, "to limit total Object events"
   ``define_min_level``, ``MIN_LEVEL``, "minimum wild encounters level"
   ``define_max_level``, ``MAX_LEVEL``, "maximum wild encounters level"
   ``define_max_encounter_rate``, ``MAX_ENCOUNTER_RATE``, "this value / 16 will be the maximum encounter rate on the ``Wild Pokémon`` tab"
   ``define_tiles_primary``, ``NUM_TILES_IN_PRIMARY``, ""
   ``define_tiles_total``, ``NUM_TILES_TOTAL``, ""
   ``define_metatiles_primary``, ``NUM_METATILES_IN_PRIMARY``, "total metatiles are calculated using metatile ID mask"
   ``define_pals_primary``, ``NUM_PALS_IN_PRIMARY``, ""
   ``define_pals_total``, ``NUM_PALS_TOTAL``, ""
   ``define_tiles_per_metatile``, ``NUM_TILES_PER_METATILE``, "to determine if triple-layer metatiles are in use. Values other than 8 or 12 are ignored"
   ``define_map_size``, ``MAX_MAP_DATA_SIZE``, "to limit map dimensions"
   ``define_map_offset_width``, ``MAP_OFFSET_W``, "to limit map dimensions"
   ``define_map_offset_height``, ``MAP_OFFSET_H``, "to limit map dimensions"
   ``define_mask_metatile``, ``MAPGRID_METATILE_ID_MASK``, "optionally read to get settings on ``Maps`` tab"
   ``define_mask_collision``, ``MAPGRID_COLLISION_MASK``, "optionally read to get settings on ``Maps`` tab"
   ``define_mask_elevation``, ``MAPGRID_ELEVATION_MASK``, "optionally read to get settings on ``Maps`` tab"
   ``define_mask_behavior``, ``METATILE_ATTR_BEHAVIOR_MASK``, "optionally read to get settings on ``Tilesets`` tab"
   ``define_mask_layer``, ``METATILE_ATTR_LAYER_MASK``, "optionally read to get settings on ``Tilesets`` tab"
   ``define_attribute_behavior``, ``METATILE_ATTRIBUTE_BEHAVIOR``, "name used to extract setting from ``symbol_attribute_table``"
   ``define_attribute_layer``, ``METATILE_ATTRIBUTE_LAYER_TYPE``, "name used to extract setting from ``symbol_attribute_table``"
   ``define_attribute_terrain``, ``METATILE_ATTRIBUTE_TERRAIN``, "name used to extract setting from ``symbol_attribute_table``"
   ``define_attribute_encounter``, ``METATILE_ATTRIBUTE_ENCOUNTER_TYPE``, "name used to extract setting from ``symbol_attribute_table``"
   ``define_metatile_label_prefix``, ``METATILE_``, "expected prefix for metatile label macro names"
   ``define_heal_locations_prefix``, ``HEAL_LOCATION_``, "default prefix for heal location macro names"
   ``define_layout_prefix``, ``LAYOUT_``, "default prefix for layout ID names"
   ``define_map_prefix``, ``MAP_``, "default prefix for map ID names"
   ``define_map_dynamic``, ``MAP_DYNAMIC``, "ID name for Dynamic maps"
   ``define_map_empty``, ``MAP_UNDEFINED``, "ID name for empty maps"
   ``define_map_section_prefix``, ``MAPSEC_``, "expected prefix for location macro names"
   ``define_map_section_empty``, ``NONE``, "macro name after prefix for empty region map sections"
   ``define_species_prefix``, ``SPECIES_``, "expected prefix for species macro names"
   ``define_species_empty``, ``NONE``, "macro name after prefix for the default species"
   ``regex_behaviors``, ``\bMB_``, "regex to find metatile behavior constants to evaluate"
   ``regex_obj_event_gfx``, ``\bOBJ_EVENT_GFX_``, "regex to find Object event graphics ID macro names"
   ``regex_items``, ``\bITEM_(?!(B_)?USE_)``, "regex to populate the ``Item`` dropdown for Hidden Item events"
   ``regex_flags``, ``\bFLAG_``, "regex to populate the ``Event Flag``/``Flag`` dropdowns for Object and Hidden Item events"
   ``regex_vars``, ``\bVAR_``, "regex to populate the ``Var`` dropdown for Trigger events"
   ``regex_movement_types``, ``\bMOVEMENT_TYPE_``, "regex to populate the ``Movement`` dropdown for Object events"
   ``regex_map_types``, ``\bMAP_TYPE_``, "regex to populate the ``Type`` dropdown for maps"
   ``regex_battle_scenes``, ``\bMAP_BATTLE_SCENE_``, "regex to populate the ``Battle Scene`` dropdown for maps"
   ``regex_weather``, ``\bWEATHER_``, "regex to populate the ``Weather`` dropdowns for maps and Weather Trigger events"
   ``regex_coord_event_weather``, ``\bCOORD_EVENT_WEATHER_``, "regex to find weather trigger macro names"
   ``regex_secret_bases``, ``\bSECRET_BASE_[\w]+_[\d]+``, "regex to populate the ``Secret Base`` dropdown for Secret Base events"
   ``regex_sign_facing_directions``, ``\bBG_EVENT_PLAYER_FACING_``, "regex to populate the ``Player Facing Direction`` dropdown for Sign events"
   ``regex_trainer_types``, ``\bTRAINER_TYPE_``, "regex to populate the ``Trainer Type`` dropdown for Object events"
   ``regex_music``, ``\b(SE|MUS)_``, "regex to populate ``Song`` dropdown for maps"
   ``regex_encounter_types``, ``\bTILE_ENCOUNTER_``, "regex to populate the ``Encounter Type`` dropdown for the Tileset Editor"
   ``regex_terrain_types``, ``\bTILE_TERRAIN_``, "regex to populate the ``Terrain Type`` dropdown for the Tileset Editor"
   ``pals_output_extension``, ``.gbapal``, "the file extension to output for a new tileset's palette data files"
   ``tiles_output_extension``, ``.4bpp.lz``, "the file extension to output for a new tileset's tiles image data file"


Global Constants
----------------

In some cases you may want to tell Porymap about a ``#define`` or ``enum`` it wouldn't otherwise know about, or override one that it already reads. For this you can add a global constant, or a global constant file, and Porymap will read and evaluate these before anything else. Let's look at an example of how each might be useful.

Porymap evaluates ``MAX_LEVEL`` in the ``constants_pokemon`` file, but let's say you have defined ``MAX_LEVEL`` to be ``#define MAX_LEVEL  (MY_CONSTANT + 1)``, and ``MY_CONSTANT`` is defined in some other file ``foo.h``. Porymap doesn't read ``foo.h``, so it doesn't know what ``MY_CONSTANT`` is and it fails to evaluate ``MAX_LEVEL``. To fix this, click the |add-global-constants-file| button on the ``Files`` tab and choose your ``foo.h`` file. Now Porymap will read any ``#define`` or ``enum`` in ``foo.h``, and it will know what ``MY_CONSTANT`` is.

Now let's say that you have ``#define MIN_LEVEL 1``. Porymap will read this ``1``, and use it as the lower limit for a Pokémon's level on the ``Wild Pokémon`` tab. But what if you want to use level ``0`` on the ``Wild Pokémon`` tab to mean something special like "match the player's level"? You could redefine ``MIN_LEVEL`` to be ``0``, but that might have consequences in your code. You could instead override ``MIN_LEVEL`` in Porymap by redefining it. Click the |add-global-constant| button on the ``Identifiers`` tab, enter the name ``MIN_LEVEL`` and its value ``0``, and now Porymap will ignore ``#define MIN_LEVEL 1`` because you already told it the value.

.. |add-global-constant| image:: images/project-files/add-global-constant.png
   :height: 24

.. |add-global-constants-file| image:: images/project-files/add-global-constants-file.png
   :height: 24
