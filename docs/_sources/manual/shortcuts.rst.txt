*********
Shortcuts
*********

Porymap has many keyboard shortcuts set by default, and even more that can be customized yourself.  You can view and customize your shortcuts by going to ``Options > Edit Shortcuts``.  Shortcut actions are grouped together by the window that they are used in (Main Window, Tileset Editor...).  You can set up to 2 shortcuts per action, but you cannot have duplicate shortcuts set within the same window.  If you enter a shortcut that is already in use, Porymap will prompt you to cancel or overwrite the old shortcut.  You can also restore your shortcuts to the defaults.

.. figure:: images/shortcuts/shortcuts-editor.png
    :alt: Shortcuts Editor
    :align: center

    Shortcuts Editor

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
   Close Project, ``Ctrl+W``
   Undo, ``Ctrl+Z``
   Redo, ``Ctrl+Y`` `or` ``Ctrl+Shift+Z``
   Back, ``Alt+←``
   Forward, ``Alt+→``
   Show Edit History Window, ``Ctrl+E``
   Open New Map Dialog, ``Ctrl+N``
   Open New Tileset Dialog, ``Ctrl+Shift+N``
   Open Tileset Editor, ``Ctrl+T``
   Open Region Map Editor, ``Ctrl+M``
   Edit Preferences, ``Ctrl+\,``
   Exit, ``Ctrl+Q``

.. csv-table::
   :header: Map Editing
   :widths: 20, 20

   Zoom In, ``Ctrl++`` `or` ``Ctrl+=``
   Zoom Out, ``Ctrl+-``
   Reset View Scale, ``Ctrl+0``
   Toggle Grid, ``Ctrl+G``
   Toggle Cursor Outline, ``C``
   Toggle Player View, ``V``
   Duplicate Event(s), ``Ctrl+D`` while selected
   Delete Event(s), ``DEL`` while selected



Tileset Editor
--------------

.. csv-table::
   :header: General,
   :widths: 20, 20

   Save, ``Ctrl+S``
   Undo, ``Ctrl+Z``
   Redo, ``Ctrl+Y`` `or` ``Ctrl+Shift+Z``
   Copy, ``Ctrl+C``
   Cut, ``Ctrl+X``
   Paste, ``Ctrl+V``
   Toggle Grid, ``Ctrl+G``



Region Map Editor
-----------------

.. csv-table::
   :header: General,
   :widths: 20, 20

   Save, ``Ctrl+S``
   Undo, ``Ctrl+Z``
   Redo, ``Ctrl+Y`` `or` ``Ctrl+Shift+Z``
   Delete layout square, ``DEL`` `or` ``Backspace``
   Resize tilemap, ``Ctrl+R``



Custom Scripts Editor
---------------------

.. csv-table::
   :header: General,
   :widths: 20, 20

   Remove selected scripts, ``DEL`` `or` ``Backspace``


.. note::
    If using macOS, ``Ctrl`` refers to the ``Command`` key, and ``Alt`` refers to the ``Option`` key.
