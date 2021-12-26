*********
Shortcuts
*********

Porymap has many keyboard shortcuts set by default, and even more that can be customized yourself.  You can view and customize your shortcuts by going to *Options -> Edit Shortcuts*.  Shortcut actions are grouped together by the window that they are used in (Main Window, Tileset Editor...).  You can set up to 2 shortcuts per action, but you cannot have duplicate shortcuts set within the same window.  If you enter a shortcut that is already in use, Porymap will prompt you cancel or overwrite the old shortcut.  You can also restore your shortcuts to the defaults.

.. figure:: images/shortcuts/edit-shortcuts.gif
    :alt: Edit Shortcuts

    Edit Shortcuts

Your shortcuts are stored at ``%Appdata%\pret\porymap\porymap.shortcuts.cfg`` on Windows and ``~/Library/Application\ Support/pret/porymap/porymap.shortcuts.cfg`` on macOS).

For reference, here is a comprehensive list of the default shortcuts set in Porymap.

Main Window
-----------

.. csv-table::
   :header: Toolbuttons
   :widths: 20, 20

   Pencil, ``N``
   Pointer, ``P`` 
   Bucket Fill, ``B``
   Eyedropper, ``E``
   Move, ``M``
   Shift, ``S``

.. csv-table::
   :header: Actions
   :widths: 20, 20
   :escape: \

   Save Current Map, ``Ctrl+S``
   Save All Maps, ``Shift+Ctrl+S``
   Open Project, ``Ctrl+O``
   Undo, ``Ctrl+Z``
   Redo, ``Ctrl+Y`` `or` ``Ctrl+Shift+Z``
   Show Edit History Window, ``Ctrl+E``
   Open New Map Dialog, ``Ctrl+N``
   Open New Tileset Dialog, ``Ctrl+Shift+N``
   Open Tileset Editor, ``Ctrl+T``
   Open Region Map Editor, ``Ctrl+M``
   Edit Preferences, ``Ctrl+\,``

.. csv-table::
   :header: Map Editing
   :widths: 20, 20

   Select Metatile, Right-click in "paint" or "fill" mode
   Select Multiple Metatiles, Hold Right-click while dragging
   Bucket Fill Metatiles, Middle-click from "paint" or "fill" mode
   Magic Fill Metatiles, ``Ctrl+`` Middle-click from "paint" or "fill" mode
   Zoom In, ``Ctrl++`` `or` ``Ctrl+=``
   Zoom Out, ``Ctrl+-``
   Reset View Scale, ``Ctrl+0``
   Toggle Grid, ``Ctrl+G``
   Toggle Cursor Outline, ``C``
   Toggle Player View, ``V``
   Toggle Draw Smart Paths, Hold ``Shift``
   Draw Straight Paths, Hold ``Ctrl``
   Duplicate Event(s), ``Ctrl+D`` while selected
   Delete Event(s), ``DEL`` while selected
   Pointer, Right-click in "paint" mode
   Select Event, Left-click in "pointer" mode or Right-click in "paint" mode
   Select Multiple Events, `Ctrl+` click



Tileset Editor
--------------

.. csv-table::
   :header: General,
   :widths: 20, 20

   Save, ``Ctrl+S``
   Undo, ``Ctrl+Z``
   Redo, ``Ctrl+Y`` `or` ``Ctrl+Shift+Z``



Region Map Editor
-----------------

.. csv-table::
   :header: General,
   :widths: 20, 20

   Save, ``Ctrl+S``
   Undo, ``Ctrl+Z``
   Redo, ``Ctrl+Y`` `or` ``Ctrl+Shift+Z``

.. note::
    If using macOS, ``Ctrl`` refers to the ``Command`` key
