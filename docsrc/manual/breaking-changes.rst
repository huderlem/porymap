****************
Breaking Changes
****************

Versioning
==========

Porymap adheres somewhat to `Semantic Versioning <https://semver.org/spec/v2.0.0.html>`_. For Porymap version ``5.4.1``, ``5`` is the ``MAJOR`` version number, ``4`` is the ``MINOR`` version number, and ``1`` is the ``PATCH`` version number.

In brief...

- ``MAJOR``: Incremented when supporting new project changes in a backwards-incompatible way ("breaking changes")
- ``MINOR``: Incremented for new features or significant changes, or when supporting new project changes in a backwards-compatible way
- ``PATCH``: Incremented for bug fixes, or other minor changes

In general, you should always use the newest Porymap version for the ``MAJOR`` version that supports your project. For example if your project is supported by Porymap version ``5.x.x``, there should be no reason to use ``5.0.0`` over ``5.4.1``.

What's a breaking change?
=========================

Breaking changes are changes that have been made in the decompilation projects (e.g. pokeemerald), which Porymap requires in order to work properly. It also includes changes to the scripting API that may change the behavior of existing Porymap scripts. If Porymap is used with a project or API script that is not up-to-date with the breaking changes, then Porymap will likely break or behave improperly.

Note that this is different than a project change that "breaks" Porymap. If Porymap adds support for some new project change in a way that's backwards-compatible, this is not a breaking change, and the ``MAJOR`` version number is not incremented. If your project doesn't have this new change then the new version of Porymap will still work for your project. If your project does have these new changes, you only need to switch to the new version of Porymap.

Updating your project
=====================

If there's been a breaking change since you started your project and you'd like to upgrade to a new ``MAJOR`` version of Porymap, see the summaries below. They'll describe what the breaking changes were, how to update your project, and what you might see go wrong if you don't have these changes.

.. _5-to-6:

From ``5.x.x`` to ``6.x.x``
---------------------------

The simplest way to update (depending on your willingness to resolve merge conflicts) is to merge the changes from one of the following PRs:

- `pokeemerald <https://github.com/pret/pokeemerald/pull/2141>`__
- `pokefirered <https://github.com/pret/pokefirered/pull/693>`__
- `pokeruby <https://github.com/pret/pokeruby/pull/879>`__

If you'd prefer to reproduce the changes, or you'd simply like to know more about the breaking changes, continue reading below.


1. ``MAP_NUM``/``MAP_GROUP``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The macros ``MAP_NUM`` and ``MAP_GROUP`` were updated to take the full map constant name (e.g. ``MAP_NUM(PETALBURG_CITY)`` becomes ``MAP_NUM(MAP_PETALBURG_CITY)``). The reasoning for this change `is explained here <https://github.com/pret/pokeemerald/pull/2023>`__.

Porymap ``6.x.x`` does not explicitly require this change; however, Porymap ``5.x.x`` depended on the old format to read/write heal location data. The changes to update the handling for heal locations (described below) will assume you are using the new ``MAP_NUM(MAP_NAME)`` format.

2. Heal Locations
^^^^^^^^^^^^^^^^^

The data for heal location events was converted from C files (``src/data/heal_locations.h``, ``include/constants/heal_locations.h``) to a JSON file (``src/data/heal_locations.json``). To handle this conversion there are additional changes to ``json_data_rules.mk``, and two new inja templates (``src/data/heal_locations.json.txt`` and ``src/data/heal_locations.constants.json.txt``). To view these changes see the mentioned files in one of the PRs listed :ref:`at the top <5-to-6>`.

This was primarily motivated by the change to the ``MAP_NUM``/``MAP_GROUP`` macros; because Porymap was reading/writing the C data directly, users were very limited in what they could do with the heal locations data without breaking Porymap. Now users have full control over what the C data looks like.

If you do not have this change then Porymap will fail to open.

3. Region Map Sections
^^^^^^^^^^^^^^^^^^^^^^

``include/constants/region_map_sections.h`` is now a generated file, and the ``map_section`` field in ``src/data/region_map/region_map_sections.json`` and ``src/data/region_map/region_map_sections.json.txt`` was renamed to ``id``. The required changes for this and their explanation are shown in `this PR <https://github.com/pret/pokeemerald/pull/2063>`__.

If you do not have these changes then any ``MAPSEC`` data not listed in ``region_map_sections.json`` will not appear in Porymap, and your project will likely fail to compile after saving because the ``map_section`` field will be automatically renamed to ``id``.

4. Local IDs
^^^^^^^^^^^^

This is optional. Without these changes, the ``Local ID`` fields of Object events and Clone Object events and the ``ID`` field of Warp events will go unused, but nothing should break.

The relevant changes are to ``tools/mapjson/mapjson.cpp``, ``map_data_rules.mk``, and ``include/constants/event_objects.h``, which can be seen in in one of the PRs listed :ref:`at the top <5-to-6>`. For an explanation of these changes, see instead `this PR <https://github.com/pret/pokeemerald/pull/2047>`__ (note that the actual code in the latter PR is outdated).

.. _4-to-5:

From ``4.x.x`` to ``5.x.x``
---------------------------

The simplest way to update (depending on your willingness to resolve merge conflicts) is to merge the changes from one of the following PRs:

- `pokeemerald <https://github.com/pret/pokeemerald/pull/1807>`__
- `pokefirered <https://github.com/pret/pokefirered/pull/570>`__
- `pokeruby <https://github.com/pret/pokeruby/pull/851>`__

If you'd prefer to reproduce the changes, or you'd simply like to know more about the breaking changes, continue reading below.

1. ``MAP_NONE`` renamed
^^^^^^^^^^^^^^^^^^^^^^^

``MAP_NONE`` was renamed to ``MAP_DYNAMIC``. This change should be made everywhere ``MAP_NONE`` occurs in your project repo. The reasoning for this change `is explained here <https://github.com/pret/pokeemerald/pull/1755>`__.

If you do not have this change then "Dynamic" map values will not be properly recognized, and setting a map selection to "Dynamic" will stop your project from compiling.

2. ``dest_warp_id``
^^^^^^^^^^^^^^^^^^^

The ``dest_warp_id`` field of Warp events was converted from a string to an int. The reasoning for this change `is explained here <https://github.com/pret/pokeemerald/pull/1755>`__.

Porymap will automatically convert ``dest_warp_id`` data to strings. If you do not have this change then the outdated ``mapjson`` tool in your project repo will incorrectly convert this string, and the in-game destination warp for all your Warp events will be ``0``.

3. Clone Object events
^^^^^^^^^^^^^^^^^^^^^^

Proper support for Clone Object events was added (previously this was handled by a field called "In Connection"). The relevant changes are shown `in this PR <https://github.com/pret/pokefirered/pull/484>`__.

This is only required if your project uses Clone Object events (by default this is just pokefirered projects). If you do not have this change, new Clone Object events will not be handled correctly by your project, and Porymap will incorrectly interpret existing Clone Object events as regular Object events.

4. Region Map Editor
^^^^^^^^^^^^^^^^^^^^

The region map data in the project was reformatted, and the Region Map Editor was redesigned to accomodate this. The relevant changes are shown `in this PR <https://github.com/pret/pokefirered/pull/500>`__.

If you do not have this change, the default settings for the Region Map Editor will not function correctly for your project. You may still be able to use the Region Map Editor if you customize the settings appropriately.

5. Additional API changes
^^^^^^^^^^^^^^^^^^^^^^^^^

The scripting API changed quite a bit from the previous version. For more details see `the notes in the CHANGELOG <https://github.com/huderlem/porymap/blob/master/CHANGELOG.md#500---2022-10-30>`_.


Prior to ``4.x.x``
------------------

For these older versions please see the individual "Breaking Changes" notes in the `Changelog <https://huderlem.github.io/porymap/reference/changelog>`_.

.. note::
    pokefirered projects were not supported until version ``4.0.0``
