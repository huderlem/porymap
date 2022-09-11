.. _rme-ref:

*********************
The Region Map Editor
*********************

This is where you edit the region maps for your game.  You are able to edit the
background tilemap, the layout of map sections, and the array of map section entries 
which determines the dimensions of each section.

To open the region map editor, navigate to *Tools -> Region Map Editor* from
porymap's main window. There is also a keyboard shortcut which is by default ``Ctrl+M``.

When you first open the region map editor, you will need to configure porymap to 
read your region map data. There are defaults for every base game project available
which should be sufficient for most users.

.. figure:: images/region-map-editor/new-configure-window.png
    :align: center
    :width: 75%
    :alt: RME Window

    Region Maps Configurator

Porymap supports multiple region maps for any project. 
By default, pokeemerald and pokefirered use this feature.
For a more custom region map, you can use the *Add Region Map...* button to 
create a new region map configuration from scratch.  You can also double-click on any existing
region map in the list to bring this window up to make changes.

.. figure:: images/region-map-editor/rme-config-properties.png
    :align: center
    :width: 50%
    :alt: RME Config Prop

    Region Map Properties Window

This window has many options for users to define:

.. csv-table::
   :header: Field,Explanation,Restrictions
   :widths: 10, 30, 20

   alias,something for porymap to distinguish between your maps,unique & valid json string
   **Tilemap Properties**,,
   format,format of the tiles,Plain *or* 4bpp *or* 8bpp
   width,width *in tiles* of the tilemap,16 *or* 32 *or* 64 *or* 128
   height,height *in tiles* of the tilemap,valid corresponding height based on width
   tileset path,the relative path to the tile image from project root,valid filepath string
   tilemap path,the relative path to the tilemap binary from project root,valid filepath string
   palette path,*optional* relative path to ``.pal`` file from project root,valid filepath string
   **Layout Properties**,*can be unchecked for maps without layouts*,
   format,the format to read the layout file,C array *or* binary
   layout path,the relative path from project root to layout file,valid filepath string
   width,the width of the layout,non-negative integer
   left offset,the position on the tilemap which defines layout x=0,width + left offset < tilemap width
   height,the height of the layout,non-negative integer
   top offset,the position on the tilemap which defines layout y=0,height + top offset < tilemap height

When you are finished configuring your region maps, you can select *OK*.  This will
display the main editor window. 

.. figure:: images/region-map-editor/rme-main-window.png
    :align: center
    :width: 75%
    :alt: RME Config Prop

    Region Map Editor Window

This window has a combobox labeled "Region" which you can use to select the current
region map you want to edit.

You will notice 
that there are three different tabs above the image of the region map 
(:ref:`Background Image <background-image-tab>`,
:ref:`Map Layout <map-layout-tab>`,
:ref:`Map Entries <map-entries-tab>`).  Let's take a look at each tab's 
functionality in more detail...


.. _background-image-tab:

Background Image Tab
--------------------

When this tab is selected, you can draw on the region map.  Select tiles from
the tile selector on the right.  You can single-click or drag your mouse around 
to paint the selected tile onto the region map image.  If you make a mistake, or 
are unhappy with what you have done, you can undo (``Ctrl+Z`` or *Edit -> Undo*)
and redo (``Ctrl+Y`` or *Edit -> Redo*) your changes.  Right-clicking on the map
image will select the tile under your mouse from the tile selector.  

If your tilemap format is not "Plain", then you can also select the palette, 
h-flip, and v-flip of any tile you are painting with.

If you want to clear the background image, *Edit -> Clear Background Image* 
will set all tiles to the first tile in the tile selector.

You can use the sliders to zoom in and out on each of the view panes.

.. _map-layout-tab:

Map Layout Tab
--------------

The layout tab is where map sections are placed on the region map.  When the 
player looks at the region map in-game, the layout determines the map under the
cursor.

.. figure:: images/region-map-editor/rme-new-layout-tab.png
    :align: center
    :width: 75%
    :alt: RME Layout

    RME Layout Tab

To modify the region map layout, select a position by clicking on the map image
and higlighting a single square.  The "Map Section" combobox will be populated
with all of the map sections defined in ``include/constants/region_map_sections.h``.
Select the map section you want to associate with the selected position on the 
region map.

There are a couple of tools which make editing multiple layout squares simultaneously easier.

*Edit -> Clear Map Layout* will set all squares in the layout to ``MAPSEC_NONE``.

*Edit -> Swap Layout Sections...* will exchange two layout sections with each other.

*Edit -> Replace Layout Section...* will replace all instances of one section with another.

The "Delete Square" button simply resets a single layout square to ``MAPSEC_NONE``.

.. _map-entries-tab:

Map Entries Tab
---------------

A region map entry is the area on the region map that spans an entire map section.
This determines, for example, where the player's head appears on the region map
in-game.  Entries are stored in ``src/data/region_map/region_map_sections.json``.

.. figure:: images/region-map-editor/rme-new-entries-tab.png
    :width: 75%
    :align: center
    :alt: RME Entries

    RME Entries Tab

To edit an entry, select a map section from the "Map Section" combobox.  You can
use the "Location" "x" and "y" spinboxes to change the coordinates of the entry.
You can also drag the entry around the map.  The "x" and "y" values correspond to
the position of the entry's top-left square on the region map.  The "Dimensions" 
"width" and "height" spinboxes will change the size of the map entry.


To change the popup name of the map section when you enter the map, type it
into the "Map Name" box.


