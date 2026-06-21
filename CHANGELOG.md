# Changelog

All notable changes to this project will be documented in this file.

## [1.9.0] - 2026-06-22

### Added
- **Native IFF Decompiler** (`iff decompile`): Splits a binary IFF skeleton-animation file into a human-readable `.IFF` text representation of the bone skeleton plus a per-animation `anims_<id>/anim_NNN.IFF` text file. Mirrors the layout produced by the old dconv `IGI1_iff.py` reference.
- **Native IFF Writer** (`iff create`): Reads a directory of BEF text files (output of `iff convert`) **or** the decompile text format (output of `iff decompile`) and assembles a single binary `.iff` file. The C++ round trip `iff convert` → `iff create` reproduces the original byte-for-byte for every IFF in the corpus.
- **`iff rebuild <src.iff> <out.iff>`**: One-shot IFF → BEF → IFF round trip via a temp directory.
- **`iff emit-qsc <dir> <out.qsc>`**: Generates the `Anims.qsc` for a folder of `.BEF` scripts (the file `IGI1_convert.py` produced in the legacy toolchain).
- **`iff export-gif <file.iff> <out.gif> [w] [h] [fps]`**: Headless software renderer that turns the IFF skeleton animation into a looping `GIF89a`. Uses an orthographic 3/4 projection, orange bones, cyan/white joint discs, and the `gif.h` NETSCAPE2.0 extension. Replaces the old OpenGL-based GUI export.
- **GUI IFF context menu**: New right-click actions on IFF files (`Decompile to text + per-anim IFFs`) and on folders of BEF scripts (`Create IFF from .BEF scripts`, `Generate Anims.qsc`). The `Export Animation as GIF...` GUI action now routes through the same `iff export-gif` headless renderer used by the CLI, so it works without the OpenGL viewport being visible.

### Changed
- **Removed `tools/` folder entirely**: the legacy `dconv/` (Python) and `gconv/` (3DS Max plugin) helpers that igi1conv used to shell out to have been deleted. `igi1conv.exe` is now a fully native, standalone C++ project with zero external runtime dependencies.
- **`igi1conv iff help`**: The help text now lists every IFF subcommand (`info`, `test`, `decompile`, `convert`, `create`, `rebuild`, `emit-qsc`, `export-gif`).
- **`iff_parser`**: The parser no longer aborts on the `>100MB` size sanity check when the chunk tag is `FORM`, because the IFF root/inner FORM sizes are intentionally "broken" (a small value that the engine walks past by tag). The 6-field `BOED` layout (2 int32 + 4 float32) is now decoded correctly with `bone_id` replacing the old misplaced `param` field.
- **BEF event field**: `BefEvent.param` is renamed to `BefEvent.bone_id`; `TriggerData(..., bone, px, py, pz)` writes/reads it correctly. The BEF writer's IFF writer now writes 24-byte events with all 6 fields.
- **BOSH object_id**: the IFF writer derives the BOSH object id from the leading digits of the first BEF filename (e.g. `003_anim_002.BEF` → object_id 3) so rebuilt IFFs report the same model id as the source.
- **`gif.h`**: every function in the header is now `static` (gated by `GIF_H_API`) so the header can be `#include`d from multiple translation units (`gui_main.cpp` + `iff_gif_exporter.cpp`) without ODR violations.
- **GUI**: dropped all references to the legacy `.bff` format and the "Batch Convert IFF/BFF" wording.

### Fixed
- **IFF writer byte order**: chunk sizes (FORM, BOSH, PLST, TLST, BOAH, BOEH, BOEH, BOED, BOTH, BOTD, BORH, BORD) are written big-endian, matching the IFF wire format. All multi-byte data values remain little-endian. The original 0-clips / wrong-object-id bugs are gone.
- **BEF parser tokenisation**: line comments (`//`) and the `,;()` separators are now stripped correctly so `AnimInit("003_anim_002",0,3741,0);` parses to the right four-arg form.

### Technical Details
- New files: `source/parsers/iff_bef.{h,cpp}`, `iff_decompiler.{h,cpp}`, `iff_writer.{h,cpp}`, `iff_gif_exporter.{h,cpp}`.
- 8 new gtest cases (`IffInfo`, `IffConvert`, `IffRebuild`, `IffDecompile`, `IffCreateFromBefs`, `IffExportGif`, `IffRoundTripSizeMatches`, `IffDecompileCreateRoundTrip`), all driven against `D:\IGI1\common\ANIMS` (000/001/002/003/005/006.IFF).
- End-to-end verified for all six IFFs: `iff convert` + `iff create` reproduces the original byte-for-byte (same `604088`/`1565672`/etc. file sizes); `iff decompile` + `iff create` round-trips successfully (size matches to within ~700 bytes due to the FORM-size convention); `iff export-gif` produces valid `GIF89a` files for all six.

---

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
