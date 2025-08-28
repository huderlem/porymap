# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project somewhat adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).  The MAJOR version number is bumped when there are **"Breaking Changes"** in the pret projects. For more on this, see [the manual page on breaking changes](https://huderlem.github.io/porymap/manual/breaking-changes.html).

## [Unreleased]
### Changed
- The name field now receives focus immediately for the new map/layout dialogs.

### Fixed
- Fix rare crash while quitting Porymap.
- Fix exported images on macOS using a different color space than in Porymap.

## [6.2.0] - 2025-08-08
### Added
- Add `View > Show Unused Colors` to the Palette Editor.
- Add `Tools > Find Color Usage` to the Palette Editor. This opens a dialog showing which metatiles use a particular color.
- Add `Edit > Swap Metatiles` to the Tileset Editor. While in this mode, selecting two metatiles in the selector will swap their positions. When changes to the tilesets are saved these relocations will be applied to all layouts that use the relevant tileset(s).
- Add `View > Layer Arrangement` to the Tileset Editor, which changes whether the metatile layer view is oriented vertically (default) or horizontally.
- Add an `Export Metatiles Image` option to the Tileset Editor that provides many more options for customizing metatile images.
- Add an `Export Porytiles Layer Images` option to the Tileset Editor, which is a shortcut for individually exporting layer images that Porytiles can use.
- Add an option under `Preferences` to include common scripts in the autocomplete for Script labels.
- Add a setting under `Project Settings` to change the width of the metatile selectors.
- Add versions of the API functions `[get|set]MetatileLayerOrder` and `[get|set]MetatileLayerOpacity` that work globally, rather than on individual layouts.
- A link to Porymap's manual is now available under `Help`.

### Changed
- The Player View Rectangle is now visible on the Events tab, as is the Cursor Tile Outline for certain tools.
- When hovering over tiles in the Tileset Editor their palette and x/yflip are now listed alongside the tile ID.
- The scroll position of the map view now remains the same between the Connections tab and the Map/Events tabs.
- The Move tool now behaves more like a traditional pan tool (with no momentum).
- The map image exporter now uses a checkered background to indicate transparency.
- Invalid tile IDs are now rendered as magenta (like invalid metatiles), instead of rendering the same as a transparent tile.
- While holding down `Ctrl` (`Cmd` on macOS) painting on the metatile layer view will now only change the tile's palette.
- Full menu paths are now listed for shortcuts in the Shortcuts Editor.
- Adding new event data to a map that has a `shared_events_map` will now remove the `shared_events_map`, rather than discard the event data.

### Fixed
- Fix crash when rendering tiles with invalid palette numbers.
- Fix crash when opening the Tileset Editor for tilesets with no metatiles.
- Fix crash when changing the map/border size in certain API callbacks.
- Fix metatile images exporting at 2x scale.
- Fix display errors when a project's metatile limits are not divisible by 8.
- Fix incorrect dividing line position for primary tiles images that are smaller than the maximum size.
- Fix the checkered background of the `Change Dimensions` popup shifting while scrolling around.
- Fix pasting Wild Pokémon data then changing maps resetting the pasted data.
- Fix click-drag map selections behaving unexpectedly when the cursor is outside the map grid.
- Fix events being dragged in negative coordinates lagging behind the cursor.
- Fix the shortcut for duplicating events working while on the Connections tab.
- Fix the Shortcuts Editor displaying the duplicate shortcut prompt repeatedly.
- Fix the clear text button on the left in each row of the Shortcuts Editor also clearing the shortcut on the right.
- Fix Undo/Redo ignoring the automatic resizing that occurs if a layout/border was an unexpected size.
- Fix Undo/Redo in the Tileset and Palette Editors and Paste in the Tileset Editor appearing enabled even when they don't do anything.
- Fix `Ctrl+Shift+Z` not being set as a default shortcut for Redo in the Palette Editor like it is for other windows.
- Fix the Tileset Editor's status bar not updating while selecting tiles in the metatile layer view, or when pasting metatiles.
- Fix the main window's status bar not immediately reflecting changes made while painting metatiles / movement permissions.
- Fix cleared metatile labels not updating until the project is reloaded.
- Fix some changes in the Tileset Editor being discarded if the window is closed too quickly.
- Fix the Region Map Editor incorrectly displaying whether a `MAPSEC` has region map data.
- Fix the Primary/Secondary Tileset selectors allowing invalid text, and considering a map unsaved if changed to invalid text then back again.
- Fix broken error message for the primary tileset on the new map/layout dialogs.
- Fix the dialog for duplicating/importing a map layout not allowing the tilesets to be changed.
- Fix warning not appearing when the log file exceeds maximum size.
- Fix possible lag while using the Tileset Editor's tile selector.
- Fix unnecessary resources being used to watch files.
- Fix possible crash on Linux if too many inotify instances are requested.

## [6.1.0] - 2025-06-09
### Added
- Add settings to change the application font and the map list font.

### Changed
- The scale of the map can now be changed while resizing the map.

### Fixed
- Fix duplicated maps writing the wrong name.
- Fix small maps being difficult to see while resizing.
- Fix the map border sometimes not updating to reflect changes.
- Fix expressions using the prefix '0X' as opposed to '0x' not being recognized has hex numbers.
- Fix certain characters not writing correctly to JSON files.
- Fix all `map.json` files being added to the file watcher at launch.
- Fix files sometimes being removed from the file watcher if they're deleted as part of a write.
- Fix `porymap.cfg` and `porymap.shortcuts.cfg` writing outside the `pret/porymap` folder.

## [6.0.0] - 2025-05-27
### Breaking Changes
- See [Breaking Changes](https://huderlem.github.io/porymap/manual/breaking-changes.html) in the manual.

### Added
- Redesigned the map list, adding new features including opening/editing layouts with no associated map, editing the names of map groups, rearranging maps and map groups, and hiding empty folders.
- Add a drop-down for changing the layout of the currently opened map.
- Add an option to duplicate maps/layouts.
- Redesigned the Connections tab, adding new features including the option to open or display diving maps and a list UI for easier edit access.
- Add a `Close Project` option
- Add a search button to the `Wild Pokémon` tab that shows the encounter data for a species across all maps.
- Add charts to the `Wild Pokémon` tab that show species and level distributions for the current map.
- Add options for customizing the map grid under `View -> Grid Settings`.
- Add an option to display Event sprites while editing the map.
- Add an option to display a dividing line between tilesets in the Tileset Editor.
- Add an input field to the Tileset Editor for editing the full metatile attributes value directly, including unused bits.
- An alert will be displayed when attempting to open a seemingly invalid project.
- Add support for defining project values with `enum` where `#define` was expected.
- Add support for referring to object events and warps with named IDs, rather than referring to them with their index number.
- Add a setting to specify the tile values to use for the unused metatile layer.
- Add a setting to specify the maximum number of events in a group. A warning will be shown if too many events are added.
- Add a setting to customize the size and position of the player view distance.
- Add `onLayoutOpened` to the scripting API.
- Add a splash loading screen for project openings.
- Add Back/Forward buttons for navigating to previous maps or layouts.

### Changed
- `Change Dimensions` now has an interactive resizing rectangle.
- Redesigned the new map dialog, including better error checking and a collapsible section for header data.
- New maps/layouts are no longer saved automatically, and can be fully discarded by closing without saving.
- Map groups and ``MAPSEC`` names specified when creating a new map will be added automatically if they don't already exist.
- Custom fields in JSON files that Porymap writes are no longer discarded.
- Edits to map connections now have Undo/Redo and can be viewed in exported timelapses.
- Changes to the "Mirror to Connecting Maps" setting will now be saved between sessions.
- A notice will be displayed when attempting to open the "Dynamic" map, rather than nothing happening.
- The base game version is now auto-detected if the project name contains only one of "emerald", "firered/leafgreen", or "ruby/sapphire".
- It's now possible to cancel quitting if there are unsaved changes in sub-windows.
- The triple-layer metatiles setting can now be set automatically using a project constant.
- `Export Map Stitch Image` and `Export Map Timelapse Image` now show a preview of the full image/gif, not just the current map.
- `Custom Attributes` tables now display numbers using spin boxes. The `type` column was removed, because `value`'s type is now obvious.
- Unrecognized map names in Event or Connections data will no longer be overwritten.
- It's now possible to click on an event's sprite even if a different event's rectangle is overlapping it. The old selection behavior is available via a new setting.
- Reduced diff noise when saving maps.
- Map names and ``MAP_NAME`` constants are no longer required to match.
- Porymap will no longer overwrite ``include/constants/map_groups.h`` or ``include/constants/layouts.h``.
- Primary/secondary metatile images are now kept on separate rows, rather than blending together if the primary size is not divisible by 8.
- The prompt to reload the project when a file has changed will now only appear when Porymap is the active application.
- `Script` dropdowns now autocomplete only with scripts from the current map, rather than every script in the project. The old behavior is available via a new setting.
- `Script` dropdowns now update automatically if the current map's scripts file is edited.
- The options for `Encounter Type` and `Terrain Type` in the Tileset Editor are not hardcoded anymore, they're now read from the project.
- The `symbol_wild_encounters` setting was replaced; this value is now read from the project.
- The max encounter rate is now read from the project, rather than assuming the default value from RSE.
- `MAP_OFFSET_W` and `MAP_OFFSET_H` (used to limit the maximum map size) are now read from the project.
- The rendered area of the map border is now limited to the maximum player view distance (prior to this it included two extra rows on the top and bottom).
- Right-clicking on the border metatiles image will now select that metatile.
- An error message will now be shown when Porymap is unable to save changes (e.g. if Porymap doesn't have write permissions for your project).
- Error and warning logs are now displayed in the status bar. This can be changed with a new setting.
- A project may now be opened even if it has no maps or map groups. A minimum of one map layout is required.
- The file extensions that are expected for `.png` and `.pal` data files and the extensions outputted when creating a new tileset can now be customized.
- Miscellaneous performance improvements, especially for opening projects.

### Fixed
- Fix `Add Region Map...` not updating the region map settings file.
- Fix some crashes on invalid region map tilesets.
- Improve error reporting for invalid region map editor settings.
- Fix the region map editor's palette resetting between region maps.
- Fix the region map editor's h-flip and v-flip settings being swapped.
- Fix config files being written before the project is opened successfully.
- Fix the map and other project info still displaying if a new project fails to open.
- Fix unsaved changes being ignored when quitting (such as with Cmd+Q on macOS).
- Fix selections with multiple events not always clearing when making a new selection.
- Fix the new event button not updating correctly when selecting object events.
- Fix duplicated `Hidden Item` events not copying the `Requires Itemfinder` field.
- Fix event sprites disappearing in certain areas outside the map boundaries.
- Fix deselecting an event still allowing you to drag the event around.
- Fix events rendering on top of the ruler at very high y values.
- Fix new map names not appearing in event dropdowns that have already been populated.
- Fix `About porymap` opening a new window each time it's activated.
- Fix the `Edit History` window not raising to the front when reactivated.
- New maps are now always inserted in map dropdowns at the correct position, rather than at the bottom of the list until the project is reloaded.
- Fix species on the wild pokémon tab retaining icons from previously-opened projects.
- Fix invalid species names clearing from wild pokémon data when revisited.
- Fix editing wild pokémon data not marking the map as unsaved.
- Fix editing an event's `Custom Attributes` not marking the map as unsaved.
- Fix changes to map connections not marking connected maps as unsaved.
- Fix numerous issues related to connecting a map to itself.
- Fix incorrect map connections getting selected when opening a map by double-clicking a map connection.
- Fix a visual issue when quickly dragging map connections around.
- Fix map connections rendering incorrectly if their direction name was unknown.
- Fix map connections rendering incorrectly if their dimensions were smaller than the border draw distance.
- Fix metatile/collision selection images skewing off-center after opening a map from the Connections tab.
- Fix the map list filter retaining text between project open/close.
- Fix the map list mishandling value gaps when sorting by Area.
- Fix a freeze on startup if project values are defined with mismatched parentheses.
- Fix stitched map images sometimes rendering garbage
- Fix the `Reset` button on `Export Map Timelapse Image` not resetting the Timelapse settings.
- Fix events in exported map stitch images being occluded by neighboring maps.
- Fix the map connections in exported map images coming from the map currently open in the editor, rather than the map shown in the export window.
- Fix crash when exporting a map stitch image if a map fails to load.
- Fix possible crash when exporting a timelapse that has events edit history.
- Fix exported timelapses excluding pasted events and certain map size changes.
- Fix exporting a timelapse sometimes altering the state of the current map's edit history.
- Stop sliders in the Palette Editor from creating a bunch of edit history when used.
- Fix scrolling on some containers locking up when the mouse stops over a spin box or combo box.
- Fix the selection index for some combo boxes differing from their displayed text.
- Fix some file dialogs returning to an incorrect window when closed.
- Fix bug where reloading a layout would overwrite all unsaved changes.
- Fix bug where layout json and blockdata could be saved separately leading to inconsistent data.
- Fix crash when saving tilesets with fewer palettes than the maximum.
- Fix projects not opening on Windows if the project filepath contains certain characters.
- Fix custom project filepaths not converting Windows file separators.
- Fix exported tile images containing garbage pixels after the end of the tiles.
- Fix fully transparent pixels rendering with the incorrect color.
- Fix the values for some config fields shuffling their order every save.
- Fix `key`s in `Custom Attributes` disappearing if given an empty name or the name of an existing field.
- Fix some problems with tileset detection when importing maps from AdvanceMap.
- Fix certain input fields allowing invalid identifiers, like names starting with numbers.
- Fix crash in the Shortcuts Editor when applying changes after closing certain windows.
- Fix the Shortcuts Editor clearing shortcuts after selecting them.
- Fix `Display Metatile Usage Counts` sometimes changing the counts after repeated use.
- The Metatile / Tile usage counts in the Tileset Editor now update to reflect changes.
- Fix regression that stopped the map zoom from centering on the cursor.
- Fix `Open Map Scripts` not working on maps with a `shared_scripts_map` field.
- Fix the main window sometimes exceeding the screen size on first launch.
- Fix right-click selections with the eyedropper clearing when the mouse is released.
- Fix overworld sprite facing directions if spritesheet is arranged in multiple rows.

## [5.4.1] - 2024-03-21
### Fixed
- Fix object event sprites not loading for some struct data formats.

## [5.4.0] - 2024-02-13
### Added
- Add a `Check for Updates` option to show new releases (Windows and macOS only).

### Changed
- If Wild Encounters fail to load they are now only disabled for that session, and the settings remain unchanged.
- Defaults are used if project constants are missing, rather than failing to open the project or changing settings.
- Selector images now center on the selection when eyedropping or zooming.

### Fixed
- Fix some minor visual issues with the various zoom sliders.
- Smooth out scrolling when mouse is over tile/metatile images.
- Fix the Tileset Editor selectors getting extra white space when changing tilesets.
- Fix a crash when adding disabled events with the Pencil tool.
- Fix error log about failing to find the scripts file when a new map is created.

## [5.3.0] - 2024-01-15
### Added
- Add zoom sliders to the Tileset Editor.
- Add `getMetatileBehaviorName` and `setMetatileBehaviorName` to the API.
- Add `metatile_behaviors`, `num_primary_palettes`, and `num_secondary_palettes` to `constants` in the API.

### Changed
- Metatile ID strings are now padded to their current max, not the overall max.
- Non-existent directories are now removed from the Open Recent Project menu.
- Hovering on the layer view in the Tileset Editor now displays the tile ID.
- Labels in the Script dropdown are now sorted alphabetically.
- The name of the Heal Locations table is no longer enforced.
- The API functions `addImage` and `createImage` now support project-relative paths.

### Fixed
- Fix the metatile selector rectangle jumping when selecting up or left of the origin.
- Fix the event group tabs sometimes showing an event from the wrong group.
- Fix the clear buttons in the Shortcuts Editor not actually removing shortcuts.
- Fix slow speed for the script label autcomplete.
- Fix deleted script labels still appearing in the autocomplete after project reload.
- Fix the map search bar stealing focus on startup.
- Fix border metatiles view not resizing properly.
- Fix Open Recent Project not clearing the API overlay
- Fix API error reporting.

## [5.2.0] - 2024-01-02
### Added
- Add an editor window under `Options -> Project Settings...` to customize the project-specific settings in `porymap.project.cfg` and `porymap.user.cfg`.
- Add an editor window under `Options -> Custom Scripts...` for Porymap's API scripts.
- Add an `Open Recent Project` menu
- Add a warning to warp events if they're on an incompatible metatile behavior.
- Add settings for custom images, including the collision graphics, default event icons, and pokémon icons.
- Add settings to override any symbol or macro names Porymap expects to find.
- Add a zoom slider to the Collision tab.
- Add toggleable grids to the Tileset Editor.
- Support for custom metatile ID, collision, and elevation data sizes.
- Support for 8bpp tileset tile images.

### Changed
- `Script` dropdowns now include scripts from the current map's scripts file.
- Encounter Rate now defaults to the most commonly used value, rather than 0.
- The Collision tab now allows selection of any valid elevation/collision value.
- The Palette Editor now remembers the Bit Depth setting.
- The min/max levels on the Wild Pokémon tab will now adjust automatically if they invalidate each other.
- If the recent project directory doesn't exist Porymap will open an empty project instead of failing with a misleading error message.
- Settings under `Options` were relocated either to the `Preferences` window or `Options -> Project Settings`.
- Secret Base and Weather Trigger events are automatically disabled if their respective constants files fail to parse, instead of not opening the project.
- If a Pokémon icon fails to load Porymap will attempt to predict its filepath. If this also fails it will appear with a placeholder icon, and won't disappear when edited.
- The bits in metatile attribute masks are now allowed to be non-contiguous.
- Porymap will now attempt to read metatile attribute masks from the project.

### Fixed
- Fix text boxes in the Palette Editor calculating color incorrectly.
- Fix metatile labels being sorted incorrectly for tileset names with multiple underscores.
- Fix default object sprites retaining dimensions and transparency of the previous sprite.
- Fix connections not being deleted when the map name text box is cleared.
- Fix the map border not updating when a tileset is changed.
- Improve the poor speed of the API functions `setMetatileTile` and `setMetatileTiles`.
- Stop the Tileset Editor from scrolling to the initially selected metatile when saving.
- Fix `0x0`/`NULL` appearing more than once in the scripts dropdown.
- Fix the selection outline sticking in single-tile mode on the Prefab tab.
- Fix heal location data being cleared if certain spaces aren't used in the table.
- Fix bad URL color contrast on dark themes.
- Fix some issues when too few/many pokémon are specified for a wild encounter group.
- Fix Porymap reporting errors for macros it doesn't use.
- Fix painting on the Collision tab with the opacity slider at 0 painting metatiles.
- Fix crashes when File->Reload Project fails.
- Fix overworld sprite facing directions if spritesheet has vertical layout.
- Stop reporting `Error: Interrupted` for custom scripts during project reopen

## [5.1.1] - 2023-02-20
### Added
- Add `registerToggleAction` to the scripting API

### Changed
- Change encounter tab copy and paste behavior.
- A warning will appear if a custom script fails to load or an action fails to run.

### Fixed
- Fix null characters being unpredictably written to some JSON files.
- Fix tilesets that share part of their name loading incorrectly.
- Fix events being hidden behind connecting maps.
- Metatile labels with values defined outside their tileset are no longer deleted.
- Fix the Tileset Editor retaining edit history after changing tilesets.
- Fix some minor visual issues on the Connections tab.
- Fix bug which caused encounter configurator to crash if slots in fields containing groups were deleted.
- Fix bug which caused encounter configurator to crash if last field was deleted.
- Fix map render when collision view was active while map changed.
- Fix the updated pokefirered region map graphics appearing in grayscale.
- Fix the API function `registerAction` not correctly handling actions with the same name.

## [5.1.0] - 2023-01-22
### Added
- Add new config options for reorganizing metatile attributes.
- Add `setScale` to the scripting API.
- Add option to turn off the checkerboard fill for new tilesets.
- Add option to copy wild encounters from another encounters tab.

### Changed
- Double-clicking on a connecting map on the Map or Events tabs will now open that map.
- Hovering on border metatiles with the mouse will now display their information in the bottom bar.
- The last-used directory is now preserved in import/export file dialogs.
- Encounter editing has been slightly modified in favor of a more performant ui.
- Pokémon icons in the encounter editor have their transparency set.

### Fixed
- Fix the Region Map Editor being opened by the Shortcuts Editor.
- Fix New Map settings being preserved when switching projects.
- Fix scripting API callback `onMapResized` not triggering.
- Fix crash when importing AdvanceMap metatiles while `enable_triple_layer_metatiles` is enabled.
- Fix `File -> Open Project` not resolving folder shortcuts.
- Fix bug where "Requires Itemfinder" checkbox is being checked by wrong data.
- Fix the map border not immediately reflecting Tileset Editor changes.
- Fix pasting metatiles in the Tileset Editor not triggering the unsaved changes warning.

## [5.0.0] - 2022-10-30
### Breaking Changes
- Proper support for pokefirered's clone objects was added, which requires the changes made in [pokefirered/#484](https://github.com/pret/pokefirered/pull/484).
- Warp IDs are now treated as strings, which requires the change to `mapjson` made in [pokeemerald/#1755](https://github.com/pret/pokeemerald/pull/1755). Additionally `MAP_NONE` was renamed to `MAP_DYNAMIC`. Both changes also apply to pokefirered and pokeruby.
- Overhauled the region map editor, adding support for tilemaps, and significant customization. Also now supports pokefirered. Requires the changes made in [pokeemerald/#1651](https://github.com/pret/pokeemerald/pull/1651), [pokefirered/#500](https://github.com/pret/pokefirered/pull/500), or [pokeruby/#842](https://github.com/pret/pokeruby/pull/842).
- Many API functions which were previously accessible via the `map` object are now accessible via one of the new objects `overlay`, `utility`, or `constants`. Some functions were renamed accordingly. See [porymap/#460](https://github.com/huderlem/porymap/pull/460) for a full list of API function name changes.
- Arguments for the API function `createImage` have changed: `xflip` and `yflip`  have been replaced with `hScale` and `vScale`, and `offset` has been replaced with `xOffset` and `yOffset`.
- The API function `addFilledRect` has been removed; it's been replaced by new arguments in `addRect`: `color` has been replaced with `borderColor` and `fillColor`, and a new `rounding` argument allows ellipses to be drawn.

### Added
- Add prefab support
- Add Cut/Copy/Paste for metatiles in the Tileset Editor.
- Add button to copy the full metatile label to the clipboard in the Tileset Editor.
- Add ability to export an image of the primary or secondary tileset's metatiles.
- Add new config options for customizing how new maps are filled, setting default tilesets, and whether the most recent project should be opened on launch.
- Add color picker to palette editor for taking colors from the screen.
- Add new features to the scripting API, including the ability to display messages and user input windows, set the overlay's opacity, rotation, scale, and clipping, interact with map header properties and the map border, read tile pixel data, and more.

### Changed
- Previous settings will be remembered in the New Map Options window.
- The Custom Attributes table for map headers and events now supports types other than strings.
- If an object event is inanimate, it will always render using its first frame.
- Unused metatile attribute bits are preserved instead of being cleared.
- The wild encounter editor is automatically disabled if the encounter JSON data cannot be read
- Metatiles are always rendered accurately with 3 layers, and the unused layer is not assumed to be transparent.
- `object_event_graphics_info.h` can now be parsed correctly if it uses structs with attributes.
- Tileset data in `headers`, `graphics`, and `metatiles` can now be parsed if written in C.
- The amount of time it takes to render the event panel has been reduced, which is most noticeable when selecting multiple events at once.
- The selection is no longer reset when pasting events. The newly pasted events are selected instead.
- The currently selected event for each event group will persist between tabs.
- An object event's sprite will now render if a number is specified instead of a graphics constant.
- Palette editor ui is updated a bit to allow hex and rgb value input.
- Heal location constants will no longer be deleted if they're not used in the data tables.
- The heal location prefixes `SPAWN_` and `HEAL_LOCATION_` may now be used interchangeably.
- The number and order of entries in the heal location data tables can now be changed arbitrarily, and independently of each other.
- The metatile behavior is now displayed in the bottom bar mouseover text.
- Number values are now allowed in the Tileset Editor's Metatile Behavior field.
- Removed some unnecessary error logs from the scripting API and added new useful ones.
- If any JSON data is the incorrect type Porymap will now attempt to convert it.

### Fixed
- Fix events losing their assigned script when the script autocomplete is used.
- Fix the unsaved changes indicator not disappearing when saving changes to events.
- Fix copy and paste for events not including their custom attributes.
- Fix cursor tile outline not updating at the end of a dragged selection.
- Fix cursor tile and player view outlines exiting map bounds while painting.
- Fix cursor tile and player view outlines not updating immediately when toggled in Collision view.
- Fix selected space not updating while painting in Collision view.
- Fix collision values of 2 or 3 not rendering properly.
- Fix the map tree view arrows not displaying for custom themes.
- Fix the map music dropdown being empty when importing a map from Advance Map.
- Fix object events added by pasting ignoring the map event limit.
- Fix a bug where saving the tileset editor would reselect the main editor's first selected metatile.
- Fix crashes / unexpected behavior if certain scripting API functions are given invalid palette or tile numbers.
- Fix drawing large amounts of text with the scripting API causing a significant drop in performance.
- Silence unnecessary error logging when parsing C defines Porymap doesn't use.
- Fix some windows like the Tileset Editor not raising to the front when reactivated.
- Fix incorrect limits on Floor Number and Border Width/Height in the New Map Options window.
- Fix Border Width/Height being set to 0 when creating a new map from an existing layout.
- Fix certain UI elements not highlighting red on some platforms.
- Fix Open Config Folder not responding
- Properly update the minimum offset for a connection when the map is changed.

## [4.5.0] - 2021-12-26
### Added
- WSL project paths are now supported. (For example, \wsl$\Ubuntu-20.04\home\huderlem\pokeemerald)
- Add ability to export map timelapse animated GIFs with `File -> Export Map Timelapse Image...`.
- Add tool to count the times each metatile or tile is used in the tileset editor.
- Events, current metatile selections, and map images can now be copied and pasted, including between windows.
- The grid and map border visibility are now saved as config options.
- Add ~60 new scripting API functions, including new features like reading/writing metatile data, layering, moving, and hiding items in the overlay, creating modified images and tile/metatile images, reading tileset sizes, logging warnings and errors, and more.
- Add 7 new scripting API callbacks.
- Porymap is now compatible with Qt6.

### Changed
- New events will be placed in the center of the current view of the map.
- Scripting API errors are more detailed and logged in more situations.
- Add new optional arguments to 5 existing API functions.
- Top-level UI elements now render above the scripting overlay.
- The onBlockChanged script callback is now called for blocks changed by Undo/Redo.

### Fixed
- Fix % operator in C defines not being evaluated
- Fix tileset palette editor crash that could occur when switching maps or tilesets with it open.
- The metatile selection is no longer reset if it becomes invalid by changing the tileset. Invalid metatiles in the selection will be temporarily replaced with metatile 0.
- Loading wild encounters will now properly preserve the original order, so saving the file will not give huge diffs.
- Fix bug where the tile selection cursor could be toggld on in the Events tab.

## [4.4.0] - 2020-12-20
### Added
- Add undoable edit history for Events tab.
- Add keyboard shortcut for `DEL` key to delete the currently selected event(s).
- Disable ui while there is no open project to prevent crashing.
- Add "Straight Paths" feature for drawing straight lines while holding `Ctrl`.
- The New Map dialog now gives an option to specify the "Show Location Name" field.
- Some new shortcuts were added in [porymap/#290](https://github.com/huderlem/porymap/pull/290).
- All plain text boxes now have a clear button to delete the text.
- The window sizes and positions of the tileset editor, palette editor, and region map editor are now stored in `porymap.cfg`.
- Add ruler tool for measuring metatile distance in events tab (Right-click to turn on/off, left-click to lock in place).
- Add delete button to wild pokemon encounters tab.
- Add shortcut customization via `Options -> Edit Shortcuts`.
- Add custom text editor commands in `Options -> Edit Preferences`, a tool-button next to the `Script` combo-box, and `Tools -> Open Project in Text Editor`. The tool-button will open the containing file to the cooresponding script.

### Changed
- Holding `shift` now toggles "Smart Path" drawing; when the "Smart Paths" checkbox is checked, holding `shift` will temporarily disable it.

### Fixed
- Fix a bug with the current metatile selection zoom.
- Fix bug preventing the status bar from updating the current position while dragging events.
- Fix porymap icon not showing on window or panel on Linux.
- The main window can now be resized to fit on lower resolution displays.
- Zooming the map in/out will now focus on the cursor.
- Fix bug where object event sprites whose name contained a 0 character would display the placeholder "N" picture.

## [4.3.1] - 2020-07-17
### Added
- Add keyboard shortcut `Ctrl + D` for duplicating map events.
- Add keyboard shortcut `Ctrl + Shift + Z` for "redo" in the tileset editor.
- Add scripting api to reorder metatile layers and draw them with opacity.

### Changed
- The tileset editor now syncs its metatile selection with the map's metatile selector.
- The number of object events per map is now limited to OBJECT_EVENT_TEMPLATES_COUNT
- The tileset editor can now flip selections that were taken from an existing metatile.

### Fixed
- Fix bug where editing a metatile layer would have no effect.
- Fix a crash that occured when creating a new tileset using triple layer mode.
- Fix crash when reducing number of metatiles past current selection.
- Fix various methods of selecting invalid metatiles.
- Fix sprite transparency not updating when changing object event graphics.
- Fix dropdown menu item selection when using the arrow keys.

## [4.3.0] - 2020-06-27
### Added
- Add triple-layer metatiles support.

### Changed
- The "Open Scripts" button will fall back to `scripts.inc` if `scripts.pory` doesn't exist. 

### Fixed
- Fix bug where exported tileset images could be horizontally or vertically flipped.
- Fix bug where the map list wasn't filtered properly after switching filter types.
- Don't zoom in map when mouse middle button is pressed.

## [4.2.0] - 2020-06-06
### Added
- Add more project-specific configs to better support porting features from different projects.
- Add metatile label names to the status bar when hovering over metatiles in the map editor tab.
- Add mouse coordinates to the status bar when hovering in the events tab.

### Changed
- `metatile_labels.h` is now watched for changes.

### Fixed
- Reduce time it takes to load maps and save in the tileset editor.
- Fix crash that could occur when parsing unknown symbols when evaluating `define` expressions.

## [4.1.0] - 2020-05-18
### Added
- Add scripting capabilities, which allows the user to add custom behavior to Porymap using JavaScript scripts.
- Add ability to import FRLG tileset .bvd files from Advance Map 1.92.

### Changed
- Edit modes are no longer shared between the Map and Events tabs. Pencil is default for Map tab, and Pointer is default for Events tab.

### Fixed
- Disallow drawing new heal locations in the events tab.
- Fix issue where the metatile selection window was not resizable.
- Show warning when closing project with unsaved wild Pokémon changes.
- Fix bug where negative object event coordinates were saved as "0".
- Fix maximum map dimension limits.
- Fix crash when using the Pencil tool to create an event on a map with no existing events.

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
- New "field" key in wild encounter JSON data from pokeemerald and pokeruby commits [adb0a444577b59eb02788c782a3d04bc285be0ba](https://github.com/pret/pokeemerald/commit/adb0a444577b59eb02788c782a3d04bc285be0ba) and [c73de8bed752ca538d90cfc93c4a9e8c7965f8c9](https://github.com/pret/pokeruby/commit/c73de8bed752ca538d90cfc93c4a9e8c7965f8c9).


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

[Unreleased]: https://github.com/huderlem/porymap/compare/6.2.0...HEAD
[6.2.0]: https://github.com/huderlem/porymap/compare/6.1.0...6.2.0
[6.1.0]: https://github.com/huderlem/porymap/compare/6.0.0...6.1.0
[6.0.0]: https://github.com/huderlem/porymap/compare/5.4.1...6.0.0
[5.4.1]: https://github.com/huderlem/porymap/compare/5.4.0...5.4.1
[5.4.0]: https://github.com/huderlem/porymap/compare/5.3.0...5.4.0
[5.3.0]: https://github.com/huderlem/porymap/compare/5.2.0...5.3.0
[5.2.0]: https://github.com/huderlem/porymap/compare/5.1.1...5.2.0
[5.1.1]: https://github.com/huderlem/porymap/compare/5.1.0...5.1.1
[5.1.0]: https://github.com/huderlem/porymap/compare/5.0.0...5.1.0
[5.0.0]: https://github.com/huderlem/porymap/compare/4.5.0...5.0.0
[4.5.0]: https://github.com/huderlem/porymap/compare/4.4.0...4.5.0
[4.4.0]: https://github.com/huderlem/porymap/compare/4.3.1...4.4.0
[4.3.1]: https://github.com/huderlem/porymap/compare/4.3.0...4.3.1
[4.3.0]: https://github.com/huderlem/porymap/compare/4.2.0...4.3.0
[4.2.0]: https://github.com/huderlem/porymap/compare/4.1.0...4.2.0
[4.1.0]: https://github.com/huderlem/porymap/compare/4.0.0...4.1.0
[4.0.0]: https://github.com/huderlem/porymap/compare/3.0.1...4.0.0
[3.0.1]: https://github.com/huderlem/porymap/compare/3.0.0...3.0.1
[3.0.0]: https://github.com/huderlem/porymap/compare/2.0.0...3.0.0
[2.0.0]: https://github.com/huderlem/porymap/compare/1.2.2...2.0.0
[1.2.2]: https://github.com/huderlem/porymap/compare/1.2.1...1.2.2
[1.2.1]: https://github.com/huderlem/porymap/compare/1.2.0...1.2.1
[1.2.0]: https://github.com/huderlem/porymap/compare/1.1.0...1.2.0
[1.1.0]: https://github.com/huderlem/porymap/compare/1.0.0...1.1.0
[1.0.0]: https://github.com/huderlem/porymap/tree/1.0.0
