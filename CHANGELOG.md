# Changelog

All notable changes to this project will be documented in this file.

## [1.8.0] - 2026-06-21

### Added
- **3D Graph Editor Viewport**: Implemented interactive 3D viewport rendering for graph files (`graph*.dat`), displaying graph nodes as 3D red cubes and visual connections as lines.
- **Dynamic Node Scaling**: Implemented automatic dynamic scaling for graph nodes based on the spatial span of the loaded map.
- **Interactive Node Scale Controls**: Added hotkeys to manually scale nodes: `Ctrl +` / `Ctrl -` to increase/decrease node size, `Ctrl + Mouse Wheel`, and `Middle Mouse + Mouse Wheel`.
- **View Toggles**: Added toggles (keys 'N' for Nodes, 'L' for Links) to hide/show nodes and links.
- **Improved View Defaults**: Configured default view rules where any `.dat` file with `graph` in its name defaults to '3D' view, and `.qvm` files default to 'Text' view.
- **View Mode Renaming**: Renamed '3D View' to '3D', and 'Image View' to 'Image' mode in the GUI.

### Fixed
- **Large Graphs Visibility**: Corrected sizing issue where nodes were too small to see in large graphs with 30-100+ nodes.

---

## [1.7.0] - 2026-06-13

### Added
- **Recursive ATTA Support in Exports**: Both "Export to OBJ" (`mef export-bundle`) and "Build Rigid Model" (`mef build-rigid`) now walk the full attachment hierarchy, automatically merging all sub-models and their textures into a single output
- **Texture Resolution from DAT Files**: `mef build-rigid` now accepts `--dat` and `--texdir` flags to resolve and convert `.tex` textures to `.tga` format with correct material slot offsets
- **Export Merged Geometry to Bundle**: New `ExportMergedToObjBundle()` exporter API for writing pre-merged geometries with resolved texture names
- **GUI Build Rigid Command Enhancement**: The GUI now automatically passes level DAT and texture directory to the build-rigid command when configured in Settings

### Fixed
- **Bone Model Rendering in GUI**: Character bone models (type1) now render at correct world positions in the 3D viewer, matching the OBJ exporter output. Previously used bone-local positions, causing visible distortion and incorrect placement
- **Texture Fallback Chain**: Improved texture discovery with priority: DAT+texDir (game assets) → local `mat_N.*` files (fallback for custom textures)

### Changed
- **MEF Export Bundle**: Now recursively merges ATTA children instead of exporting only the top-level model
- **Build Rigid Model**: Geometry from all ATTA children is inlined; no external ATTA references remain in the output

### Technical Details
- Fixed `MergeGeometryRecursive()` to use `v.pos * 40.96f` for bone models (world-positioned) instead of `v.rawPos` (bone-local), aligning with MEF native parser behavior
- Material slot offsets correctly propagated through merged hierarchy
- Portal and TAMC records properly offset and merged from all attachment levels

---

## [1.6.0] - 2026-05-15

### Added
- Full support for AI character bone models (Type 1 skeletal models)
- DNER/ECAF split face rendering for bone models
- Bone vertex weights and indices parsing
- Upside-down UV mapping correction for bone model textures

### Fixed
- ATTA parsing for recursive attachments
- MEX (text MEF) file support in export and compilation

---

## [1.5.0] - Earlier

See commit history for previous releases.
