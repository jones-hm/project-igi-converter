
## Bug ID: MEX-Convert-Overwrite
**Description:** Converting .mex binary to text format inadvertently overwrote the generated text file with its own .mex binary sidecar due to sharing the same extension, causing the conversion to appear broken.
**Resolution:** Updated cmd_mef.cpp and mef_compiler.cpp to use a .extra extension (e.g. model.mex.extra) for the sidecar when the input file is .mex. This safely decouples the text file from the binary sidecar chunks. Also transitioned the sidecar format from a custom SIDX container to standard native ILFF format in mef_exporter.cpp.

## Bug ID: MEX-Export-OBJ-Error
**Description:** The GUI was incorrectly trying to invoke export-obj when exporting .mex and .mef files to OBJ via the right-click menu, leading to unknown subcommand errors since the CLI expects export. Additionally, .mex was not wired up to correctly append the -o <output> argument.
**Resolution:** Changed the GUI action string from export-obj to export, and ensured that both mex export and mex dump correctly format the command line arguments passing -o to the CLI tool. Also added dynamic parsing of the 4-byte header ILFF in gui_main.cpp to correctly determine if a .mef/.mex file should open in 3D/Hex view (binary) or Text view (ascii) instead of blindly relying on the extension.

## Feature Request: GUI Rename and Build Rigid tweaks
**Description:** Renamed menu actions and adjusted uild-rigid behavior as requested.
**Resolution:** Renamed Build Rigid Model to Build Model. Renamed Export to OBJ to Export to Obj. Renamed Export to Text to Export to Mef(Text). Grouped Info and Dump under an Info & Dump submenu. Updated cmd_mef.cpp to output to the same filename (overwriting) instead of appending _rigid.mef. Updated gui_main.cpp to show the custom loading bar text Completing Build model finding attachments textures.... Please wait when executing uild-rigid.

## Feature Request: GUI Rename Info & Dump
**Description:** Renamed the Info & Dump submenu to Details as per user preference.
**Resolution:** Renamed in gui_main.cpp and compiled the Release build.

## Feature Request: GUI Export Obj tweaks
**Description:** User requested to rename "Export to Obj" to "Export to OBJ+MTL+TGA" and have text MEF/MEX export prompt for a folder.
**Resolution:** Updated gui_main.cpp to rename the export menu items to "Export to OBJ+MTL+TGA". Added `QFileDialog::getExistingDirectory` logic for text mef/mex exports so they ask for a folder, matching the binary export behavior. Built and ran Release version.

## Bug ID: MEX-Lighting-Fix
**Description:** When rotating objects in the 3D viewport, the lighting incorrectly shifted non-uniformly because the fragment shader was using a point light tied to FragPos which varied under object translation. Additionally, backfaces became pitch black due to single-sided lighting.
**Resolution:** Replaced the point light with a fixed directional headlamp in gui_main.cpp's fragment shader, and added abs() to the dot product to implement two-sided lighting so both the front and back of the geometry stay uniformly lit regardless of rotation angle. Built in Release mode.

## Bug ID: MEF-Recursive-ATTA-Scan
**Description:** The GUI editor did not recursively load ATTA (attachment) models and their textures when a base MEF file was opened. The 'build-rigid' CLI tool also lost textures when compiling the rigid model because it stripped the PMTL/TAMC chunks without generating a proper sidecar. This caused users to see missing geometries and textures when examining assembled levels like 435_01_1.mef.
**Resolution:** Implemented loadMefRecursive in gui_main.cpp. The GUI now parses ATTA matrices and naturally handles the hierarchical structure, natively scanning for attachments and their textures directly into the viewport. This makes 'build-rigid' unnecessary for merely viewing full levels with textures.

## Bug ID: Graph-3D-Viewport-Nodes
**Description:** The 3D graph viewer did not display nodes as red 3D cubes, did not show links and link info properly like `igi-editor`, and lacked view-toggles and node details.
**Resolution:** Replaced/fixed graph drawing logic in `gui_main.cpp` to render red cubes for nodes and lines for links, implemented toggles (keys 'N' and 'L'), and enabled mouse-hover and selection details display.

## Bug ID: Graph-Nodes-Scale-Visibility
**Description:** On graph files with a large number of nodes (30-100+), the nodes were too small to see.
**Resolution:** Added dynamic sizing based on the graph's spatial span and introduced interactive scaling hotkeys (`Ctrl +` / `Ctrl -`, `Ctrl + Wheel`, and `Middle Mouse + Wheel`) to dynamically scale the red cubes representing the nodes in the 3D viewport.

## Bug ID: MEF-Texture-Orientation-VFlip
**Description:** MEF models exported to .obj or .mef (via `mef build-rigid`) had face textures either upside down or horizontally flipped depending on model type. Type 3 (lightmap) models - which are 82% of MEFs in level1/models (340/415) - were systematically V-flipped because commit 03642a7's `isBoneModel` check (which inspected `renderLayout.find("type1")`) could not catch the "packed DNER" layout used by lightmap models. The same flaw also made bone-model faces appear upside down. The 3D viewer and the exporter were using inconsistent V-flip logic, so the live preview and the exported file rendered differently for the same source MEF.
**Resolution:** Centralised the V-flip decision in `MefVToObjV(v, modelType)` in `source/parsers/mef_exporter.cpp` and made it the identity (V is never flipped) for every model type. Replaced per-call-site `1.0f - v.uv.y` / `1.0f - uv[]` literals with the helper. The GUI 3D viewer in `igi1conv/gui_main.cpp` now reads `v.uv.y` directly with no V-flip helper. Added 8 regression tests in `tests/test_igi1conv_commands.cpp` (`MefExportVFlip_Type0_Rigid_DoesNotFlipV`, `MefExportVFlip_Type1_Bone_DoesNotFlipV`, `MefExportVFlip_Type3_Lightmap_DoesNotFlipV`, `MefExportVFlip_AllTypesRespectRule`, `MefExportVFlip_BinaryAndTextPathsAgree`, `MefExportVFlip_NoStrayOneMinusVLiterals`, `MefViewerDoesNotFlipV`, plus the existing `MefExportObjHasRealUvs` / `MefExportBinaryAndTextObjUvsMatch`) so the rule cannot drift again.

## Bug ID: Tests-Hardcoded-Corpus-Path
**Description:** `tests/igi1conv_test_util.h` hard-coded a `D:\IGI1\full_test` default in `CorpusDir()`, which broke the test suite on any machine without that exact path and embedded a Windows-specific absolute path in the source tree.
**Resolution:** Removed the hard-coded fallback. `CorpusDir()` now returns "" when neither `--game-path=PATH` / `--corpus=PATH` (parsed in `tests/test_igi1conv_dynamic.cpp`) nor the `IGI_GAME_PATH` env var is set, and tests that call `IGI1CONV_NEED()` `GTEST_SKIP()` cleanly. Documented the new flag / env-var contract in `docs/COMMANDS.md` and bumped version to 1.9.1.
