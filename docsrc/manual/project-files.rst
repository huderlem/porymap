*************
Project Files
*************

Porymap relies on the user maintaining a certain level of integrity with their project files.
This is a list of files that porymap reads from and writes to. Generally, if porymap writes 
to a file, it probably is not a good idea to edit yourself unless otherwise noted.

The filepath that Porymap expects for each file can be overridden under the ``Project Files`` section of ``Options -> Project Settings``. A new path can be specified by entering it in the text box or choosing it with the folder button. Paths are expected to be relative to the root project folder. If no path is specified, or if the file/folder specified does not exist, then the default path will be used instead. The name of each setting in this section is listed in the table below under ``Override``.

.. figure:: images/project-files/settings.png
    :align: center
    :width: 75%
    :alt: Settings


.. csv-table::
   :header: File Name,Read,Write,Override,Notes
   :widths: 20, 5, 5, 20, 30

   data/maps/\*/map.json, yes, yes, ``data_map_folders``,
   data/maps/\*/scripts.[inc|pory], yes, no, ``data_map_folders``, for finding script labels
   data/scripts/\*.[inc|pory], yes, no, ``data_scripts_folders``, for finding script labels
   data/event_scripts.s, no, yes, ``data_event_scripts``, only appends new script files to end of file
   data/maps/map_groups.json, yes, yes, ``json_map_groups``,
   data/layouts/layouts.json, yes, yes, ``json_layouts``,
   data/layouts/\*/[border|map].bin, yes, yes, ``data_layouts_folders``,
   src/data/tilesets/headers.h, yes, yes, ``tilesets_headers``,
   src/data/tilesets/graphics.h, yes, yes, ``tilesets_graphics``, also edits palette and tile image files listed in this file
   src/data/tilesets/metatiles.h, yes, yes, ``tilesets_metatiles``, also edits metatile files listed in this file
   data/tilesets/headers.inc, yes, yes, ``tilesets_headers_asm``, only if ``tilesets_headers`` can't be found
   data/tilesets/graphics.inc, yes, yes, ``tilesets_graphics_asm``, only if ``tilesets_headers`` can't be found
   data/tilesets/metatiles.inc, yes, yes, ``tilesets_metatiles_asm``, only if ``tilesets_headers`` can't be found
   data/tilesets/[primary|secondary]/\*, yes, yes, ``data_tilesets_folders``, default tileset data location
   src/data/wild_encounters.json, yes, yes, ``json_wild_encounters``,
   src/data/object_events/object_event_graphics_info_pointers.h, yes, no, ``data_obj_event_gfx_pointers``,
   src/data/object_events/object_event_graphics_info.h, yes, no, ``data_obj_event_gfx_info``,
   src/data/object_events/object_event_pic_tables.h, yes, no, ``data_obj_event_pic_tables``,
   src/data/object_events/object_event_graphics.h, yes, no, ``data_obj_event_gfx``,
   src/data/graphics/pokemon.h, yes, no, ``data_pokemon_gfx``, for pokemon sprite icons
   src/data/heal_locations.h, yes, yes, ``data_heal_locations``,
   src/data/region_map/region_map_sections.json, yes, yes, ``json_region_map_entries``,
   src/data/region_map/porymap_config.json, yes, yes, ``json_region_porymap_cfg``,
   include/constants/global.h, yes, no, ``constants_global``, reads ``OBJECT_EVENT_TEMPLATES_COUNT``
   include/constants/map_groups.h, no, yes, ``constants_map_groups``,
   include/constants/items.h, yes, no, ``constants_items``,
   include/constants/opponents.h, yes, no, ``constants_opponents``, reads max trainers constant
   include/constants/flags.h, yes, no, ``constants_flags``,
   include/constants/vars.h, yes, no, ``constants_vars``,
   include/constants/weather.h, yes, no, ``constants_weather``,
   include/constants/songs.h, yes, no, ``constants_songs``,
   include/constants/heal_locations.h, yes, yes, ``constants_heal_locations``,
   include/constants/pokemon.h, yes, no, ``constants_pokemon``, reads min and max level constants
   include/constants/map_types.h, yes, no, ``constants_map_types``,
   include/constants/trainer_types.h, yes, no, ``constants_trainer_types``,
   include/constants/secret_bases.h, yes, no, ``constants_secret_bases``, pokeemerald and pokeruby only
   include/constants/event_object_movement.h, yes, no, ``constants_obj_event_movement``,
   include/constants/event_objects.h, yes, no, ``constants_obj_events``,
   include/constants/event_bg.h, yes, no, ``constants_event_bg``,
   include/constants/region_map_sections.h, yes, no, ``constants_region_map_sections``,
   include/constants/metatile_labels.h, yes, yes, ``constants_metatile_labels``,
   include/constants/metatile_behaviors.h, yes, no, ``constants_metatile_behaviors``,
   include/fieldmap.h, yes, no, ``constants_fieldmap``, reads tileset related constants
   src/event_object_movement.c, yes, no, ``initial_facing_table``, reads ``gInitialMovementTypeFacingDirections``
   src/pokemon_icon.c, yes, no, ``pokemon_icon_table``, reads files in ``gMonIconTable``


