# Changelog

All notable changes to this project will be documented in this file.

## [1.5.0] - 2026-06-12
### Added
- **32-Bit (Win32) Architecture Support**: Integrated complete compilation and deployment pipelines for 32-bit Windows targets, enabling compatibility with legacy systems.
- **Dual Qt5 / Qt6 Compatibility**: Overhauled the OpenGL rendering and widget codebase (in `gui_main.cpp`) to compile seamlessly using either the local Qt5 SDK or system Qt6 SDK.
- **Automatic Architecture Detection**: Upgraded `CMakeLists.txt` to automatically detect target architecture (pointer size) from the generator options and resolve the corresponding Qt dependencies.
- **Auto-Deployed Runtime Assets**: Grouped `IGIAutoComplete.txt`, `IGIModels.json`, and `IGIModelsAllLevel.json` under `assets/` and configured a post-build step in CMake to automatically copy them to the compiled binary folder.
- **Embedded Application Icon Everywhere**: Fixed the resource compilation order in CMake (moving AUTOMOC/AUTORCC/AUTOUIC before target declaration) and changed the resource ID in `igi1conv.rc` to `1` so the app icon displays correctly in both Windows Explorer and the window taskbar at runtime.
- **Simplified UI Documentation**: Consolidated the three theme screenshots in the README into a single horizontal row with a concise description and reordered the layout to display the GUI version first and CLI commands last.
- **Release Packaging Script**: Created a script to package compiled builds into separate zip packages (`igi1conv_v1.5.0_x86.zip` and `igi1conv_v1.5.0_x64.zip`) for distribution.

## [1.3.0] - 2026-06-12
### Added
- **Comprehensive GUI Documentation**: Added 15+ high-quality markdown screenshot descriptions covering IDE functionality, Hex View, Model Search, and comprehensive GUI Themes (Military, Solarized, Dark).
- **Automated Screenshot Engineering**: Developed a highly resilient UI automation pipeline script (`take_screenshots.py`) incorporating rigorous self-healing loops to bypass OS foreground restrictions and generate flawless IDE captures.
- **Enhanced Configurations**: Automatically registers `TextureDir` and `LevelDAT` mappings persistently via `igi1conv.ini`, completely bypassing fatal GUI dialog blockers during automated texture extraction scripts.

## [1.2.0] - 2026-06-11
- **Integrated IDE Features**: Converted the basic Qt window into a full-fledged IDE/Explorer.
  - Added Dark Theme and Light Theme toggles in the View menu.
  - Added a search bar above the file explorer tree for recursive wild-card file filtering.
  - Added an in-file text search bar for finding text inside the opened file viewer.
- **File Operations Context Menu**: Right-clicking a file in the explorer now supports native file system operations:
  - Rename files (with popup prompt).
  - Delete files (with confirmation prompt).
  - Cut, Copy, and Paste files across directories.
- **Advanced 3D Multi-Material Renderer**: Overhauled the OpenGL engine to support `SubMesh` architecture.
  - Automatically reads `RenderBlocks` from `.mef` files to map individual textures to specific geometry groups.
  - Automatically reads `usemtl` records from `.obj` and corresponding `.mtl` libraries to map separate materials perfectly.
- **Live 3D Metadata HUD**: Replaced the native OS tooltip with an anchored semi-transparent HUD overlay.
  - Tracks live mouse movements to report accurate 3D translation offsets and rotation angles (Pitch, Yaw, Roll/Zoom) in real-time.
  - Retrieves exact unique `Model ID` matching from JSON catalogs, filtering out unnecessary metadata like Graph IDs.

### Changed
- Refactored `gui_main.cpp` to use `QSortFilterProxyModel` for non-destructive filesystem filtering.
- Prevented tooltips from disappearing during mouse drags by replacing them with the HUD.
- The `gui_main.cpp` build configuration no longer attempts to reload duplicated materials.

### Fixed
- Fixed critical bug where the `.mef` and `.obj` 3D viewer would overwrite previous textures, causing the entire model to be wrapped in the mesh's final declared material.
- Fixed an issue where the OS suppressed tooltips while the mouse button was held down, preventing users from seeing rotation angles.

## [1.1.0] - Initial Advanced GUI Edition
- Introduced `igi1conv` GUI leveraging Qt6.
- Added custom text, hex, image, and basic 3D model viewers.
- Added execution wrappers for CLI commands on standard IGI files.
