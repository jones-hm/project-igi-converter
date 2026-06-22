# Changelog

All notable changes to this project will be documented in this file.

## [1.9.2] - 2026-06-22

### Added
- **3D graph toolbar**: New toolbar under the 3D viewer, shown only when a `graph*.dat` file is loaded. Mirrors the layout pattern used by the IFF media bar / image editor toolbar. Includes:
  - **+ Node / - Node** buttons to resize the red node cubes from the GUI (no keyboard shortcuts required).
  - **Reset** button to restore the default node scale.
  - **Nodes** and **Links** check-box toggles to show or hide the corresponding layer.
  - **Total Nodes: N** and **Total Links: N** read-only labels that update automatically every time a graph is loaded. Wired through a new `ModelViewer::onGraphLoaded` callback so the toolbar (in `MainWindow`) stays in sync with the `GraphFile` state (in `ModelViewer`).

### Changed
- **.DAT context menu** now detects `graph*.dat` (graph.dat, graph1.dat, graph2.dat, graph3.dat, and any filename starting with "graph") by base name and presents a dedicated context menu instead of the regular level-DAT (model/texture bindings) menu. The new graph menu offers: **View Graph in 3D**, **Export to JSON**, **Export to Table (.md)**, **Info**, **Dump**. All other `.dat` files (level1.dat, levelX.dat) keep the existing Convert / Export / Info options.
- **Export to Table (.md)** captures both `graph info` and `graph dump` into a structured Markdown report that lists every node, every link, and aggregate stats in fenced code blocks - a single human-readable summary of the entire AI navigation graph.
- **"Pack to .res archive"** renamed to **"Pack to Archive"** and now auto-saves to a sibling `<folder>.res` when one exists, without prompting the user with a save dialog. The folder's prefix is always `folderName/` (no more QInputDialog prompt), which matches how the IGI engine indexes texture resources.

### Fixed
- **Image-to-TEX conversion error** used to log the INPUT path on error (`Failed to convert image to TEX: <input>`), which was misleading - the conversion writes to a sibling `.tex` file. Now logs `input -> output` and shows a `QMessageBox::critical` with the actual error. `saveAsTex()` now also early-rejects images larger than 65535 px on either axis (the TEX v11 header stores width/height as `uint16`) and ensures the destination directory exists before opening the file.

## [1.9.1] - 2026-06-22

### Fixed
- **MEF texture orientation (the "upside down / sideways" bug)**: The `.mef → .obj` and `.mef → .mef` (via `mef build-rigid`) export paths now preserve V verbatim for every model type, matching the orientation the IGI engine and the GUI 3D viewer use. The 3D viewer (the live preview in `igi1conv.exe`) and the exported `.obj` / `.mef` now render identically for the same source `.mef`. Concretely, the original `015_01_1.mef` (Type 0 rigid), `001_01_1.mef` (Type 1 bone), and `404_01_1.mef` (Type 3 lightmap) all render right-side-up in the viewer **and** in the exported files. The 82% of MEFs that are Type 3 (lightmap) no longer come out flipped, and bone-model face textures are no longer upside down.

### Changed
- **Centralised V-flip decision** in `MefVToObjV(v, modelType)` (`source/parsers/mef_exporter.cpp`). The old per-call-site `1.0f - v.uv.y` / `1.0f - uv[]` literals have been removed and replaced with a single helper. The GUI 3D viewer reads `v.uv.y` directly with no V-flip helper. The rule is now the identity for every model type — no more special-casing of Type 1 (bone) vs Type 3 (lightmap) vs Type 0 (rigid).
- **Removed hardcoded `D:\IGI1` path** from the test suite. The corpus location is now fully user-controlled via the new `--game-path=PATH` / `--corpus=PATH` CLI flag or the `IGI_GAME_PATH` env var. When neither is set, `CorpusDir()` returns "" and tests `GTEST_SKIP()` instead of failing. `tests/igi1conv_test_util.h` no longer mentions a Windows-specific path.

### Added
- **Comprehensive MEF V-flip regression suite** in `tests/test_igi1conv_commands.cpp`:
  - `MefExportVFlip_Type0_Rigid_DoesNotFlipV`: rigid models preserve V.
  - `MefExportVFlip_Type1_Bone_DoesNotFlipV`: bone models preserve V.
  - `MefExportVFlip_Type3_Lightmap_DoesNotFlipV`: lightmap models preserve V.
  - `MefExportVFlip_AllTypesRespectRule`: sweeps up to 64 MEFs in the corpus, asserts every model obeys the per-type rule.
  - `MefExportVFlip_BinaryAndTextPathsAgree`: binary MEF → OBJ and text MEF → OBJ produce identical V for the same source MEF.
  - `MefExportVFlip_NoStrayOneMinusVLiterals`: structural test that greps `mef_exporter.cpp` for stray `1.0f - v.uv.y` / `1.0f - uv[` literals outside the helper, so the formula can't drift between call sites.
  - `MefViewerDoesNotFlipV`: structural test that the GUI 3D viewer never re-introduces a V-flip helper.
  - `MefExportObjHasRealUvs` and `MefExportBinaryAndTextObjUvsMatch`: lock in that the OBJ export has real per-vertex UVs and that the binary and text paths agree.
- **New test util helpers** in `tests/igi1conv_test_util.h`:
  - `FindCorpusMefOfModelType(int wantedType, const std::string& namePattern = "")` — finds a MEF with a specific `model_type` (0 = rigid, 1 = bone, 3 = lightmap) in the corpus.
  - `GetMefModelType(const std::string& mefPath)` — parses `model_type` from `mef info` output.
  - `FirstVtFromObj(const std::string& objPath)` — reads the first non-(0,0) `vt` line from an OBJ.
- **Test corpus** (`D:\IGI1\igi1conv_test_suite` — outside the repo, no hardcoded path committed): added `320_01_1.mef` (Type 0 rigid) and `404_01_1.mef` (Type 3 lightmap) so the per-type tests have files to exercise. The pre-existing `0_000_01_1.mef` (Type 1 bone) covers the third category.

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
