.. _creating-new-maps:

*****************
Creating New Maps
*****************

Creating a new map in porymap is easy! Just click ``File > New Map...`` and you'll be greeted with a dialog that has all the options for your new map. These options are described below. Change them however you like, hit ``OK``, and you should see your new map!

Your settings will remain the same for the next time you open this dialog. If you'd like to restore them to their default values, hit ``Reset``.

.. figure:: images/creating-new-maps/new-map-options-window.png
    :alt: New Map Options Window
    :width: 60%
    :align: center

    New Map Options Window

Map Name
	A unique name for your new map. This name won't appear in-game. It will appear in Porymap's map list and various dropdowns, and will be used to create a unique ``MAP_NAME`` ID for your new map. For the in-game map name, see ``Location Name`` under ``Header Data``.

Map Group
	Which map group the new map will belong to. You can either select an existing map group, or enter the name of a new map group that you'd like Porymap to create. In Porymap, the only place you'll see the map group is on the ``Groups`` tab of the map list. The map group you choose is mostly organizational, but you may have some features in your game that rely on maps being grouped together. For more on map groups, see :ref:`Map List <navigation-map-list>`.

Layout ID
	The ID name for your new map's layout. This ID name will be updated automatically as you enter the name of your new map. If you're creating an entirely new map, this ID name should be unique. If you'd to like to create a new map using an existing layout, select the ID from the dropdown. Selecting an existing layout ID will disable the dimensions and tilesets settings, because these are determined by the layout.

Map Dimensions
	The width and height (in metatiles) of the map.

Border Dimensions
	The width and height (in metatiles) of the map's border blocks. You will only see this setting if you have ``Enable Custom Border Size`` checked under ``Options > Project Settings``

Tilesets
	The map's primary and secondary tileset.

Can Fly To
	Whether a Heal Location event will be created for this map. This is provided for historical convenience. This used to be the only way to add a Heal Location event to a map, but since Porymap v6.0.0 you can create a new one at any time from the ``Events`` tab.

Header Data
	This collapsible section contains some additional information about your map. You can click on the arrow to the left of ``Header Data`` to expand or collapse this section. This section is provided for convenience only, you don't need to worry about it right now! All this information can be edited later on the ``Header`` tab. See `Editing Map Headers <https://huderlem.github.io/porymap/manual/editing-map-header.html>`_.

