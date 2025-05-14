**********
Navigation
**********

Porymap can seem daunting at first because it has many buttons, tabs, panes, and windows.  Let's briefly go over the different parts of the application.

.. _navigation-map-list:

Map List
--------

The map list contains a hierarchical view of all of the maps in your project.  It is situated on the left side of Porymap's main window.  To switch to a different map, simply double-click on the desired map's name.  Larger maps can take a few seconds to load the first time, so be patient.

.. figure:: images/navigation/map-list-pane.png
    :alt: Map List Pane
    :width: 40%
    :align: center

    Map List Pane

Tabs
^^^^

The map list has 3 tabs that each organize your maps a bit differently.

Groups
    Maps are sorted by "map groups". Next to each map name you will see a ``[AA.BB]`` prefix. ``AA`` will be the map group number, and ``BB`` will be the map number. These are the numbers you can get in your project using the ``MAP_GROUP`` and ``MAP_NUM`` macros, respectively.

Locations
    Maps are sorted by their ``Location`` value from the ``Header`` tab. This value may also be referred to as a "region map section" or "mapsec" for short, because it is used to identify maps for the region map. Maps with the same ``Location`` will have the same in-game name, if it appears.

Layouts
    Maps are sorted by their ``Layout``. From this tab you can also open and edit layouts directly. Most layouts are only used by a single map, but layouts like the Pokémon Center are used by many maps. Some maps have multiple layouts that they can switch between, like ``Route111`` using both ``Route111_Layout`` and ``Route111_NoMirageTower_Layout``. 


On any of the tabs, right-clicking on a folder name or map name will bring up some helpful options. For instance, you can right-click on a folder to add a new map to that folder, or right-click on a map to duplicate that map.

Buttons
^^^^^^^

Under each tab you'll also find a set of buttons that operate on the current map list tab.

 - |folder-add| Adds a new folder
 - |folder-eye| Hides empty folders
 - |folder-expand| Expands all of the map folders
 - |folder-collapse| Collapses all of the map folders
 - |lock| Allows you to rename folders and reorganize the map list; this is is only accessible on the ``Groups`` tab.

.. |folder-add| image:: images/navigation/folder-add.png
   :height: 24

.. |folder-eye| image:: images/navigation/folder-eye.png
   :height: 24

.. |folder-expand| image:: images/navigation/folder-expand.png
   :height: 24

.. |folder-collapse| image:: images/navigation/folder-collapse.png
   :height: 24

.. |lock| image:: images/navigation/lock.png
   :height: 24

To the left of the tabs the |button-navigation| buttons will let you go back/forward to previous maps you had open, and to the right of the tabs the |button-add| button will open the dialog to create a new map (see :ref:`Creating New Maps <creating-new-maps>`).

.. |button-add| image:: images/navigation/button-add.png
   :height: 24

.. |button-navigation| image:: images/navigation/button-navigation.png
   :height: 24

Type in the filter to show maps/folders that contain the filter text.

.. figure:: images/navigation/filter-map-list.png
    :alt: Filter Map List
    :width: 40%
    :align: center


    Filter Map List

Status Icons
^^^^^^^^^^^^

Next to each map/layout name you'll see an icon that indicates its current status.

  - |map-icon-unloaded| Has not been loaded
  - |map-icon-loaded| Has been loaded
  - |map-icon-open| Is currently open
  - |map-icon-unsaved| Has unsaved changes
  - |map-icon-error| An error occurred, Porymap cannot use this map elsewhere in the project. See your error log file for more details.

.. |map-icon-unloaded| image:: images/navigation/map-icon-unloaded.png
   :height: 24

.. |map-icon-loaded| image:: images/navigation/map-icon-loaded.png
   :height: 24

.. |map-icon-open| image:: images/navigation/map-icon-open.png
   :height: 24

.. |map-icon-unsaved| image:: images/navigation/map-icon-unsaved.png
   :height: 24

.. |map-icon-error| image:: images/navigation/map-icon-error.png
   :height: 24

Main Window
-----------

Most of the work you do in Porymap is in the center Main Window.  It features 5 tabbed views which each have different purposes, but they all operate within the context of the currently-opened map in the Map List.  Let's quickly summarize what each of these tabs is used for.

.. figure:: images/navigation/main-window-tabs.png
    :alt: Main Window Tabs
    :width: 90%
    :align: center

    Main Window Tabs

Map
    This tab is further divided into 3 tabs, accessible from the panel on the right

    .. figure:: images/navigation/map-tabs.png
        :alt: Map Tabs
        :width: 40%

    - :ref:`Metatiles <editing-map-tiles>`: Change the map's visual appearance by painting metatiles and setting the map's layout and tilesets.
    - :ref:`Collision <editing-map-collisions>`:  Determine how the player can navigate the map by painting collision and elevation values.
    - :ref:`Prefabs <editing-map-tiles-prefabs>`: Save groups of metatiles/collision for more convenient painting.

:ref:`Events <editing-map-events>`
    Edit the interactable events on the map.  This includes things like objects, warps, script triggers, and more.

:ref:`Header <editing-map-headers>`
    Choose various gameplay properties to set for the map. This includes things like background music and weather.

:ref:`Connections <editing-map-connections>`
    Change how the map connects with surrounding maps when the player walks from one to another.

:ref:`Wild Pokémon <editing-wild-encounters>`
    Edit the wild Pokémon available in the map.


.. note::
    If you have a layout open, only the ``Map`` tab will be active. Additionally, the Wild Pokémon tab may be disabled if the data failed to load, or if you disabled it under ``Project Settings``.
