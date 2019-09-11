***********************
Editing Wild Encounters
***********************

porymap provides a tab for editing the wild pokemon encounter JSON data.
Navigate to the "Wild Pokemon" tab in porymap's main window.

If you open the tab and there are no wild encounters for the current map, you
will see an empty screen (pictured below). Adding wild pokemon data is as 
simple as :ref:`adding new encounter groups <add-encounter-groups>`.

.. figure:: images/editing-wild-encounters/no-encounters.png
    :alt: Empty Encounter Tab

    Empty Encounter Tab

Otherwise, you should see something similar to this:

.. figure:: images/editing-wild-encounters/populated-encounter-tab.png
    :alt: Populated Encounter Tab

    Populated Encounter Tab

The tab for each field is active or disabled based on the encounter data for a
given map.  If a tab is disabled, you can activate it, and therefore activate
a new encounter field for the map.  To activate a field, right-click on the
tab name for the field you want to add.

.. figure:: images/editing-wild-encounters/activate-tab.png
    :alt: Activate Encounter Field

    Activate Encounter Field

Editing the wild encounters is otherwise pretty straightforward.  You can 
adjust the minimum and maximum levels, the encounter rate, and species with the
ui.



.. _add-encounter-groups:

Adding New Encounter Groups
---------------------------

An encounter group is just another set of wild encounters that are available 
for a single map.  In the vanilla games, only Altering Cave uses multiple
encounter groups, but there are several reasons you might want them.  

In order to create a new encounter group, click the green (+) button next to
the Group drop-down.  It will bring up this menu:

.. figure:: images/editing-wild-encounters/new-group-window.png
    :alt: New Encounter Group Window

    New Encounter Group Window

You can give your new encounter group a name (this must be uniqe, which is 
enforced), and you can choose which fields to activate for the group.  Checking
the "copy from current group" box will copy not only the active fields but also
the wild pokemon data for each field from the currently displayed group.

One possible use for having multiple encounter groups for a single map is to
implement time of day encounters.

.. figure:: images/editing-wild-encounters/time-of-day-encounter-group.gif
    :alt: Time of Day Encounter Groups

    Time of Day Encounter Groups



.. _configure-encounter-json:

Configuring the Wild Encounter Fields
-------------------------------------

An encounter field describes a group of wild encounters.  This includes the name
of the field, a default number of pokemon in that field, and the encounter 
ratio for each index in that field.  These are all things you may want to 
change.  Click on the *Configure JSON...* button to bring up this window:

.. figure:: images/editing-wild-encounters/configure-json.png
    :alt: Configure JSON Window

    Configure JSON Window

The Field drop-down will allow you select which field you want to manipulate.
You can add a new one with the *Add New Field...* button.  The green (+) and 
red (-) buttons add and take away encounter slots for the field.  For each slot
you will see an adjustible number.  This represents the encounter chance for a
pokemon at this index out of the total.

Let's add a ``headbutt_mons`` field to our wild encounters...

First, we'll add a new field and name it ``headbutt_mons``.

.. figure:: images/editing-wild-encounters/configure-json-new-field.png
    :alt: New Field Name

    New Field Name

Then, we want four slots in this field, and we set encounter ratios for each
slot.

.. figure:: images/editing-wild-encounters/headbutt-mon-field.png

If we accept the changes, we can now assign pokemon to each slots, and adjust
the levels.

.. figure:: images/editing-wild-encounters/headbutt-mons.png

Changes made to the wild encounters are not saved to disk until you save the map.

