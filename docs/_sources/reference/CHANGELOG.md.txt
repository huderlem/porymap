# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project somewhat adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).  The MAJOR version number is bumped when there are breaking changes in the pret projects.

The **"Breaking Changes"** listed below are changes that have been made in the decompilation projects (e.g. pokeemerald), which porymap requires in order to work properly. If porymap is used on a project that is not up-to-date with the breaking changes, then porymap will likely break or behave improperly.

## [Unreleased]
### Added
- Add scripting capabilities, which allows the user to add custom behavior to Porymap using JavaScript scripts.
- Add ability to import FRLG tileset .bvd files from Advance Map 1.92.

### Fixed
- Disallow drawing new heal locations in the events tab.
- Fix issue where the metatile selection window was not resizable.

## [4.0.0] - 2020-04-28
### Breaking Changes
- If you are using pokeemerald or pokeruby, there were changes made in [pokeemerald/#1010](https://github.com/pret/pokeemerald/pull/1010) and [pokeruby/#776](https://github.com/pret/pokeruby/pull/776) that you will need to integrate in order to use this version of porymap.

### Added
- Support for [pokefirered](https://github.com/pret/pokefirered). Kanto fans rejoice! At long last porymap supports the FRLG decompilation project.
- Add ability to export map stitches with `File -> Export Map Stitch Image...`.
- Add new project config option `use_custom_border_size`.
- Add ability to toggle project settings in `Options` menu.
- Add file monitoring, so Porymap will prompt the user to reload the project if certain project files are modified outside of Porymap.
- Add ability to reload project.
- Add `Pencil`, `Move`, and `Map Shift` tools to the Events tab.

### Changed
- Porymap now saves map and encounter json data in an order consistent with the upstream repos. This will provide more comprehensible diffs when files are saved.
- Update Porymap icon.
- The "Map" and "Events" tabs now render using the same view, so jumping between them is smooth.
- Extend connection min and max offsets to player's view boundary, rather than the map's boundary.

### Fixed
- Fix bug where pressing TAB key did not navigate through widgets in the wild encounter tables.
- Fix bug that allowed selecting an invalid metatile in the metatile selector.
- Don't allow `.` or `-` characters in new tileset names.
- Fix regression that prevented selecting empty region map squares

## [3.0.1] - 2020-03-04
### Fixed
- Fix bug on Mac where tileset images were corrupted when saving.

## [3.0.0] - 2020-03-04
### Breaking Changes
- pokeemerald and pokeruby both underwent a naming consistency update with respect to "object events". As such, these naming changes break old versions of Porymap.
  - pokeemerald object event PR: https://github.com/pret/pokeemerald/pull/910
  - pokeruby object event PR: https://github.com/pret/pokeruby/pull/768

### Added
- Add optional support for Poryscript script files via the `use_poryscript` config option.
- Selecting a group of metatiles from the map area now also copies the collision properties, too.
- Add keyboard shortcut `Ctrl + G` for toggling the map grid.

### Changed
- Draw map connections with the current map's tilesets to more accurately mimic their appearance in-game.

### Fixed
- Fix index-out-of-bounds crash when deleting the last event in an event type group.
- Fix bug where exporting tileset images could add an extra row of junk at the end.
- Fix crashes when encountering an error opening a project or map.
- Fix bug where comboboxes and wild pokemon data could grow large when opening projects multiple times during the same porymap session.
- Fix bug where dragging the metatile selector would visually extend beyond map boundary.


## [2.0.0] - 2019-10-16
### Breaking Changes
- Accomodate event object graphics pointer table being explicitly indexed. From changes introduced in commits [cdae0c1444bed98e652c87dc3e3edcecacfef8be](https://github.com/pret/pokeemerald/commit/cdae0c1444bed98e652c87dc3e3edcecacfef8be) and [0e8ccfc4fd3544001f4c25fafd401f7558bdefba](https://github.com/pret/pokeruby/commit/0e8ccfc4fd3544001f4c25fafd401f7558bdefba).
- New "field" key in wild encounter JSON data from pokeemerald and pokeruby commits [adb0a444577b59eb02788c782a3d04bc285be0ba](https://github.com/pret/pokeemerald/commit/adb0a444577b59eb02788c782a3d04bc285be0ba) and [https://github.com/pret/pokeruby/commit/c73de8bed752ca538d90cfc93c4a9e8c7965f8c9](c73de8bed752ca538d90cfc93c4a9e8c7965f8c9).


### Added
- Add wild encounter table editor.
- Add dark themes.
- Support metatile labels file introduced in pokeruby and pokeemerald commits [ad365a35c1536740cbcbc10bee66e5dd908c39e7](https://github.com/pret/pokeruby/commit/ad365a35c1536740cbcbc10bee66e5dd908c39e7) and [c68ba9f4e8e260f2e3389eccd15f6ee5f4bdcd3e](https://github.com/pret/pokeemerald/commit/c68ba9f4e8e260f2e3389eccd15f6ee5f4bdcd3e).
- Add warning when closing porymap with unsaved changes.

### Changed
- Exporting map images is now more configurable. Events, connections, collision, etc. can be toggled on and off before exporting the image.
- The entire Tileset Editor selection is now conveniently flipped when selecting x-flip or y-flip.
- Autocomplete for porymap's comboboxes no longer require typing the full string prefix.

### Fixed
- Fix bug where map group names were hardcoded when creating a new map.
- Fix bug in Tileset Editor where multi-tile selections weren't properly painted when clicking on the bottom row of the metatile layers.
- Fix bug where line breaks in C headers were not parsed properly.
- Fix bug when exporting tileset images using palettes with duplicate colors.
- Fix bug where creating new maps from existing layouts created an empty layout folder.
- Fix bug where exported tile images did not contain the last row of tiles.


## [1.2.2] - 2019-05-16
### Added
- Add region map editor
- Add ability to add new tilesets
- Add official Porymap documentation website: https://huderlem.github.io/porymap/

### Changed
- Event sprites now display as facing the direction of their movement type.
- Default values for newly-created events now use valid values from the project, rather than hardcoded values.
- Deleting events will stay in the same events tab for easier bulk deletions.
- Double-clicking on a secret base event will open the corresponding secret base map.
- Selected events are now rendered above other events.
- Default values for new events are now more sensible and guaranteed to be valid.

### Fixed
- Fix bug in zoomed metatile selector where a large selection rectangle was being rendered.
- Fix bug where edited map icons were not rendered properly.
- Fix bug where right-click copying a tile from the tileset editor's metatile layers wouldn't copy the x/y flip status.
- Parsing project data is now more resilient to crashing, and it reports more informative errors.


## [1.2.1] - 2019-02-16
### Added
- Add ability to zoom in and out the map metatile selector via a slider at the bottom of the metatile selector window.

### Fixed
- Fix crash when creating a new map from a layout that has no pre-existing maps that use it.
- Fix bug where `var_value`, `trainer_type` and `trainer_sight_or_berry_tree_id` JSON fields were being interpreted as integers.

## [1.2.0] - 2019-02-04
### Breaking Changes
- New JSON map data format in pokeemerald and pokeruby from commits  [82abc164dc9f6a74fdf0c535cc1621b7ed05318b](https://github.com/pret/pokeemerald/commit/82abc164dc9f6a74fdf0c535cc1621b7ed05318b) and [a0ba1b7c6353f7e4f3066025514c05b323a0123d](https://github.com/pret/pokeruby/commit/a0ba1b7c6353f7e4f3066025514c05b323a0123d).

### Added
- Add "magic fill" mode to fill tool (hold down CTRL key). This fills all matching metatiles on the map, rather than only the contiguous region.
- Add ability to import tileset palettes (JASC, .pal, .tpl, .gpl, .act).
- Add ability to export tileset tiles as indexed .png images. The currently-selected palette is used.
- Restore window sizes the next time the application is opened.
- Add ability to import metatiles from Advance Map 1.92 (.bvd files).
- Add About window that contains porymap information and changelog. (Found in file menu `Help > About Porymap`)
- Add option to show player's in-game view when hovering the mouse on the map.
- Add option to show an outline around the currently-hovered map tile. Its size depends on the size of the current metatile selection.
- Add ability to define custom fields for map header and all events.

### Changed
- Collapse the map list by default.
- Collision view now has a transparency slider to help make it easier to view the underlying metatiles.
- When importing tileset tiles from an image that is not indexed, the user can also provide a palette for the image. This is for the scenario where the user exports tiles and a palette from Advance Map.
- When creating a new map, the user specifies all of the map properties in a new window prompt.
- New maps can be created using existing layouts by right-clicking on an existing layout folder in the map list panel when sorted by "Layout".
- The map list panel now has "expand-all" and "collapse-all" buttons.
- Events without sprites are now partially transparent so the underlying metatile can be seen. (Warps, signs, etc.)
- Changed the Trainer checkbox to a combobox, since there are actually 3 valid values for the trainer type.
- Multiline comments are now respected when parsing C defines.
- The tiles image in the tileset editor will no longer flip according to the x/y flip checkboxes. The individual tile selection still flips, though.

### Fixed
- Fix bug where smart paths could be auto-enabled, despite the checkbox being disabled.
- Fix crash that could occur when changing the palette id in the tileset palette editor.
- Fix crash that could occur when shrinking the number of metatiles in a tileset.
- Fix bug where exported tile images from Advance Map were not handled correctly due to Advance Map using incorrect file extensions.

## [1.1.0] - 2018-12-27
### Breaking Changes
- New map header format in pokeemerald from commit [a1ea3b5e394bc115ba9b86348c161094a00dcca7](https://github.com/pret/pokeemerald/commit/a1ea3b5e394bc115ba9b86348c161094a00dcca7).

### Added
- Add `porymap.project.cfg` config file to project repos, in order to house project-specific settings, such as `base_game_version=pokeemerald`.
- Write all logs to `porymap.log` file, so users can view any errors that porymap hits.
- Changelog

### Changed
- Add `porymap.cfg` base config file, rather than using built-in system settings (e.g. registry on Windows).
- Properly read/write map headers for `pokeemerald`.
- Overhauled event editing pane, which now contains tabs for each different event. Events of the same type can be iterated through using the spinner at the top of the tab. This makes it possible to edit events that are outside the viewing window.

### Fixed
- Creating new hidden-item events now uses a valid default flag value.
- Fix bug where tilesets were sometimes not displaying their bottom row of metatiles.
- Fix bug where porymap crashes on startup due to missing map headers.
- Fix tileset editor crash that only happened on macOS.
- Fix minor bug when parsing C defines.
- Write `MAP_GROUPS_COUNT` define to `maps.h`.
- Fix bug where opening multiple projects and saving would cause junk to be written to `layouts_table.inc`.
- Fix porymap icon on macOS.

## [1.0.0] - 2018-10-26
This was the initial release.

[Unreleased]: https://github.com/huderlem/porymap/compare/4.0.0...HEAD
[4.0.0]: https://github.com/huderlem/porymap/compare/3.0.1...4.0.0
[3.0.1]: https://github.com/huderlem/porymap/compare/3.0.0...3.0.1
[3.0.0]: https://github.com/huderlem/porymap/compare/2.0.0...3.0.0
[2.0.0]: https://github.com/huderlem/porymap/compare/1.2.2...2.0.0
[1.2.2]: https://github.com/huderlem/porymap/compare/1.2.1...1.2.2
[1.2.1]: https://github.com/huderlem/porymap/compare/1.2.0...1.2.1
[1.2.0]: https://github.com/huderlem/porymap/compare/1.1.0...1.2.0
[1.1.0]: https://github.com/huderlem/porymap/compare/1.0.0...1.1.0
[1.0.0]: https://github.com/huderlem/porymap/tree/1.0.0
