# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project does **not** adhere to any strict versioning scheme, such as [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
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

[Unreleased]: https://github.com/huderlem/porymap/compare/1.0.0...HEAD
[1.0.0]: https://github.com/huderlem/porymap/tree/1.0.0
