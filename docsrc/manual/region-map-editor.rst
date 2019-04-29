*********************
The Region Map Editor
*********************

This is where you edit the region map for your game.  To open the region map 
editor, navigate to *Tools -> Region Map Editor* from porymap's main window.

When you first open the region map editor, your window will look like this:

.. figure:: images/region-map-editor/rme-new-window.png
    :scale: 75%
    :align: center
    :alt: RME Window

    Region Map Editor Window

This window is split vertically--the region map editing is done at the top, 
while the zoomed-in city maps are edited at the bottom.  You can use the 
sliders to zoom in and out on each of the view panes.  You will notice 
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
image will select the tile under your mouse from the tile selector.  If you want
to clear the background image, *Edit -> Clear Background Image* will set all 
tiles to the first tile in the tile selector.

.. figure:: images/region-map-editor/rme-painting-image.gif
    :scale: 75%
    :align: center
    :alt: RME Paint

    Drawing on the Region Map Image

It is likely that you will want to use your own tiles for your region map.  You 
can import a tile image by navigating to *Tools -> Import Region Map Image Tiles*.
There are strict requirements for your region map tile image.  It must (1) be 
indexed with a 256 color palette\*, (2) composed of 8x8 pixel tiles, (3) have 256
or fewer tiles.

\* while the region map tile image requires a 256-color palette, the image only
uses the 32 colors beginning at index 112 in the palette

.. _map-layout-tab:

Map Layout Tab
--------------

The layout tab is where map sections are placed on the region map.  When the 
player looks at the region map, the layout determines the map under the cursor.

.. figure:: images/region-map-editor/rme-layout-tab.png
    :scale: 75%
    :align: center
    :alt: RME Layout

    RME Layout Tab

To modify the region map layout, select a position by clicking on the map image
and higlighting a single square.  The "Map Section" combobox will be populated
with all of the map sections defined in `include/constants/region_map_sections.h`.
Select the map section you want to associate with the selected position on the 
region map.  To change the popup name of the map section when you enter the map,
type it into the "Map Name" box.  The popup name is tied to the map section, so
each layout square with the same map section will share a name.

*Edit -> Clear Map Layout*

When you add new region map sections.
*Edit -> Swap*

.. figure:: images/region-map-editor/rme-layout-swap.gif
    :scale: 75%
    :align: center
    :alt: RME Swap

    Swapping Map Sections

.

.. _map-entries-tab:

Map Entries Tab
---------------

The other thing

.. figure:: images/region-map-editor/rme-entries-tab.png
    :scale: 75%
    :align: center
    :alt: RME Entries

    RME Entries Tab

.

City Maps
---------

city maps

.. figure:: images/region-map-editor/rme-painting-city.gif
    :scale: 60%
    :align: center
    :alt: City Paint

    Drawing on the City Map

*Tools -> Import City Map Image Tiles*

You can add a new city map by pressing the |new-city-map-button| button.





.. |new-city-map-button|
   image:: images/region-map-editor/rme-new-city-map-button.png

