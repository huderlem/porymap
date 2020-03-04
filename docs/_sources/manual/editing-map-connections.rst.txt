***********************
Editing Map Connections
***********************

Maps can be connected together so that the player can seamlessly walk between them.  These connections can be edited in the Connections tab.

.. figure:: images/editing-map-connections/map-connections.png
    :alt: Map Connections View

    Map Connections View

A connection has a direction, offset, and destination map.  To add new connection, press the plus button |add-connection-button|.  To delete a connection, select it and press the delete button |remove-connection-button|.

.. |add-connection-button|
   image:: images/editing-map-connections/add-connection-button.png

.. |remove-connection-button|
   image:: images/editing-map-connections/remove-connection-button.png

To change the connection's vertical or horizontal offset, it's easiest to click and drag the connection to the desired offset.

Dive & Emerge Warps
-------------------

Dive & emerge warps are used for the HM move Dive. They don't have offsets or directions--only a destination map.  To add a dive or emerge warp, simply add a value in the Dive Map and/or Emerge Map dropdown menus.


Mirror Connections
------------------

An extremely useful feature is the *Mirror to Connecting Maps* checkbox in the top-right corner.  Connections are one-way, which means that you must keep the two connections in sync between the two maps.  For example, Petalburg City has a west connection to Route 104, and Route 104 has an east connection to Petalburg City.  If *Mirror to Connecting Maps* is enabled, then Porymap will automatically update both sides of the connection.  Be sure to *File -> Save All* (``Ctrl+Shift+S``) when saving, since you will need to save both maps' connections.

Follow Connections
------------------

Double-clicking on a connection will open the destination map.  This is very useful for navigating through your maps, similar to double-clicking on  :ref:`Warp Events <event-warps>`.
