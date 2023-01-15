.. _settings-and-options:

****************
Porymap Settings
****************

Porymap uses config files to read and store user and project settings.
A global settings file is stored in a platform-dependent location for app configuration files 
(``%Appdata%\pret\porymap\porymap.cfg`` on Windows, ``~/Library/Application\ Support/pret/porymap/porymap.cfg`` on macOS).

A config file is also created when opening a project in porymap for the first time. It is stored in
your project root as ``porymap.project.cfg``. There are several project-specific settings that are
determined by this file. You may want to force commit this file so that other users will automatically have access to your project settings.

A second config file is created for user-specific settings. It is stored in
your project root as ``porymap.user.cfg``. You should add this file to your gitignore.

.. csv-table::
   :header: Setting,Default,Location,Can Edit?,Description
   :widths: 10, 3, 5, 5, 20

   ``recent_project``, , global, yes, The project that will be opened on launch
   ``reopen_on_launch``, 1, global, yes, Whether the most recent project should be opened on launch
   ``recent_map``, , user, yes, The map that will be opened on launch
   ``pretty_cursors``, 1, global, yes, Whether to use custom crosshair cursors
   ``map_sort_order``, group, global, yes, The order map list is sorted in
   ``window_geometry``, , global, no, For restoring window sizes
   ``window_state``, , global, no, For restoring window sizes
   ``map_splitter_state``, , global, no, For restoring window sizes
   ``main_splitter_state``, , global, no, For restoring window sizes
   ``collision_opacity``, 50, global, yes, Opacity of collision overlay
   ``metatiles_zoom``, 30, global, yes, Scale of map metatiles
   ``show_player_view``, 0, global, yes, Display a rectangle for the GBA screen radius
   ``show_cursor_tile``, 0, global, yes, Display a rectangle around the hovered metatile(s)
   ``monitor_files``, 1, global, yes, Whether porymap will monitor changes to project files
   ``tileset_checkerboard_fill``, 1, global, yes, Whether new tilesets will be filled with a checkerboard pattern of metatiles.
   ``theme``, default, global, yes, The color theme for porymap windows and widgets
   ``text_editor_goto_line``, , global, yes, The command that will be executed when clicking the button next the ``Script`` combo-box.
   ``text_editor_open_directory``, , global, yes, The command that will be executed when clicking ``Open Project in Text Editor``.
   ``base_game_version``, , project, no, The base pret repo for this project
   ``use_encounter_json``, 1, user, yes, Enables wild encounter table editing
   ``use_poryscript``, 0, project, yes, Whether to open .pory files for scripts
   ``use_custom_border_size``, 0, project, yes, Whether to allow variable border sizes
   ``enable_event_weather_trigger``, 1 if not ``pokefirered``, project, yes, Allows adding Weather Trigger events
   ``enable_event_secret_base``, 1 if not ``pokefirered``, project, yes, Allows adding Secret Base events
   ``enable_event_clone_object``, 1 if ``pokefirered``, project, yes, Allows adding Clone Object events
   ``enable_hidden_item_quantity``, 1 if ``pokefirered``, project, yes, Adds ``Quantity`` to Hidden Item events
   ``enable_hidden_item_requires_itemfinder``, 1 if ``pokefirered``, project, yes, Adds ``Requires Itemfinder`` to Hidden Item events
   ``enable_heal_location_respawn_data``, 1 if ``pokefirered``, project, yes, Adds ``Respawn Map`` and ``Respawn NPC`` to Heal Location events
   ``enable_floor_number``, 1 if ``pokefirered``, project, yes, Adds ``Floor Number`` to map headers
   ``enable_map_allow_flags``, 1 if not ``pokeruby``, project, yes, "Adds ``Allow Running``, ``Allow Biking``, and ``Allow Dig & Escape Rope`` to map headers"
   ``create_map_text_file``, 1 if not ``pokeemerald``, project, yes, A ``text.inc`` or ``text.pory`` file will be created for any new map
   ``enable_triple_layer_metatiles``, 0, project, yes, Enables triple-layer metatiles (See https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles)
   ``new_map_metatile``, 1, project, yes, The metatile id that will be used to fill new maps
   ``new_map_elevation``, 3, project, yes, The elevation that will be used to fill new maps
   ``new_map_border_metatiles``, "``468,469,476,477`` or ``20,21,28,29``", project, yes, The list of metatile ids that will be used to fill the 2x2 border of new maps
   ``default_primary_tileset``, ``gTileset_General``, project, yes, The label of the default primary tileset
   ``default_secondary_tileset``, ``gTileset_Petalburg`` or ``gTileset_PalletTown``, project, yes, The label of the default secondary tileset
   ``custom_scripts``, , user, yes, A list of script files to load into the scripting engine
   ``prefabs_filepath``, ``<project_root>/prefabs.json``, project, yes, The filepath containing prefab JSON data
   ``prefabs_import_prompted``, 0, project, no, Keeps track of whether or not the project was prompted for importing default prefabs
   ``tilesets_have_callback``, 1, project, yes, Whether new tileset headers should have the ``callback`` field
   ``tilesets_have_is_compressed``, 1, project, yes, Whether new tileset headers should have the ``isCompressed`` field
   ``metatile_attributes_size``, 2 or 4, project, yes, The number of attribute bytes each metatile has
   ``metatile_behavior_mask``, ``0xFF`` or ``0x1FF``, project, yes, The mask for the metatile Behavior attribute
   ``metatile_encounter_type_mask``, ``0x0`` or ``0x7000000``, project, yes, The mask for the metatile Encounter Type attribute
   ``metatile_layer_type_mask``, ``0xF000`` or ``0x60000000``, project, yes, The mask for the metatile Layer Type attribute
   ``metatile_terrain_type_mask``, ``0x0`` or ``0x3E00``, project, yes, The mask for the metatile Terrain Type attribute

Some of these settings can be toggled manually in porymap via the *Options* menu.
