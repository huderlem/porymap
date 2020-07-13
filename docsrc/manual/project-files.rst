*************
Project Files
*************

Porymap relies on the user maintaining a certain level of integrity with their project files.
This is a list of files that porymap reads from and writes to. Generally, if porymap writes 
to a file, it probably is not a good idea to edit yourself unless otherwise noted.


.. csv-table::
   :header: File Name,Read,Write,Notes
   :widths: 20, 5, 5, 30

   data/maps/\*/map.json, yes, yes,
   data/event_scripts.s, no, yes, only appends new script files to end of file
   data/maps/map_groups.json, yes, yes, 
   data/layouts/layouts.json, yes, yes, also reads border and blockdata files listed in this file
   data/tilesets/headers.inc, yes, yes,
   data/tilesets/graphics.inc, yes, yes, also edits palette and tile image files listed in this file
   data/tilesets/metatiles.inc, yes, yes, also edits metatile files listed in this file
   src/data/wild_encounters.json, yes, yes, 
   src/data/object_events/object_event_graphics_info_pointers.h, yes, no, 
   src/data/object_events/object_event_graphics_info.h, yes, no, 
   src/data/object_events/object_event_pic_tables.h, yes, no, 
   src/data/object_events/object_event_graphics.h, yes, no, 
   src/data/graphics/pokemon.h, yes, no, for pokemon sprite icons
   src/data/heal_locations.h, yes, yes, 
   src/data/region_map/region_map_entries.h, yes, yes, 
   include/constants/global.h, yes, no, 
   include/constants/map_groups.h, no, yes, 
   include/constants/items.h, yes, no, 
   include/constants/opponents.h, yes, no, reads max trainers constant
   include/constants/flags.h, yes, no, 
   include/constants/vars.h, yes, no, 
   include/constants/weather.h, yes, no, 
   include/constants/heal_locations.h, no, yes, 
   include/constants/pokemon.h, yes, no, reads min and max level constants
   include/constants/map_types.h, yes, no, 
   include/constants/trainer_types.h, yes, no, 
   include/constants/secret_bases.h, yes, no, pokeemerald and pokeruby only
   include/constants/event_object_movement.h, yes, no, 
   include/constants/event_bg.h, yes, no, 
   include/constants/region_map_sections.h, yes, no, 
   include/constants/metatile_labels.h, yes, yes, 
   include/constants/metatile_behaviors.h, yes, no, 
   include/fieldmap.h, yes, no, reads tileset related constants


