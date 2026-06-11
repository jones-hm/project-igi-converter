# Changelog

All notable changes to this project will be documented in this file.

## [1.2.0] - 2026-06-11
### Added
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
