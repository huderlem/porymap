.. _settings-and-options:

****************
Porymap Settings
****************

Porymap uses config files to read and store user and project settings.
A global settings file is stored in a platform-dependent location for app configuration files 
(``%Appdata%\pret\porymap\porymap.cfg`` on Windows, ``~/Library/Application\ Support/pret/porymap/porymap.cfg`` on macOS).

A config file is also created when opening a project in porymap for the first time. It is stored in
your project root as ``porymap.project.cfg``. There are several project-specific settings that are
determined by this file.

.. csv-table::
   :header: Setting,Default,Location,Can Edit?,Description
   :widths: 10, 3, 5, 5, 20

   ``recent_project``, , global, yes, The project that will be opened on launch
   ``recent_map``, , global, yes, The map that will be opened on launch
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
   ``region_map_dimensions``, 32x20, global, yes, The dimensions of the region map tilemap
   ``theme``, default, global, yes, The color theme for porymap windows and widgets
   ``text_editor_goto_line``, , global, yes, The command that will be executed when clicking the button next the ``Script`` combo-box.
   ``text_editor_open_directory``, , global, yes, The command that will be executed when clicking ``Open Project in Text Editor``.
   ``base_game_version``, , project, no, The base pret repo for this project
   ``use_encounter_json``, 1, project, yes, Enables wild encounter table editing
   ``use_poryscript``, 0, project, yes, Whether to open .pory files for scripts
   ``use_custom_border_size``, 0, project, yes, Whether to allow variable border sizes
   ``enable_event_weather_trigger``, 1 if not ``pokefirered``, project, yes, Allows adding Weather Trigger events
   ``enable_event_secret_base``, 1 if not ``pokefirered``, project, yes, Allows adding Secret Base events
   ``enable_hidden_item_quantity``, 1 if ``pokefirered``, project, yes, Adds ``Quantity`` to Hidden Item events
   ``enable_hidden_item_requires_itemfinder``, 1 if ``pokefirered``, project, yes, Adds ``Requires Itemfinder`` to Hidden Item events
   ``enable_heal_location_respawn_data``, 1 if ``pokefirered``, project, yes, Adds ``Respawn Map`` and ``Respawn NPC`` to Heal Location events
   ``enable_object_event_in_connection``, 1 if ``pokefirered``, project, yes, Adds ``In Connection`` to Object events
   ``enable_floor_number``, 1 if ``pokefirered``, project, yes, Adds ``Floor Number`` to map headers
   ``create_map_text_file``, 1 if not ``pokeemerald``, project, yes, A ``text.inc`` or ``text.pory`` file will be created for any new map
   ``enable_triple_layer_metatiles``, 0, project, yes, Enables triple-layer metatiles (See https://github.com/pret/pokeemerald/wiki/Triple-layer-metatiles)
   ``custom_scripts``, , project, yes, A list of script files to load into the scripting engine

Some of these settings can be toggled manually in porymap via the *Options* menu.
