# igi1conv — Project IGI 1 Game Converter
This is a standalone command-line converter and inspector for *Project IGI 1* game files. Inspired by the original IGI1Conv shipped with IGI 2, it allows you to seamlessly read, convert, and inspect the engine's proprietary formats—textures, meshes, scripts, archives, terrain, fonts, and AI graphs—with **no OpenGL or game-editor dependency**.

It's the ultimate tool for modders and researchers looking to extract game assets, modify them using modern tools, and inject them back into the game.

## 🖥️ GUI Version (Graphical User Interface)

An interactive workspace designed for visual inspection, navigation, and quick asset modifications:
*   **File Tree Navigator**: Browse game folders with real-time directory searching.
*   **Interactive 3D Viewer**: Render native `.mef` and `.obj` models with rotation, zoom, and wireframe views.
*   **2D Image Viewer**: Instantly preview proprietary `.tex`, `.spr`, and `.pic` texture assets.
*   **Texture Paint & Editor**: Edit Game assets directly by drawing, painting, and modifying textures using pencil, eraser, and color pickers, and save modifications back to the native game format.
*   **Script & Hex Inspectors**: Read decompiled `.qsc` script files and inspect raw binary data.
*   **Right-Click Context Menu**: Trigger conversion tasks directly from the graphical user interface.

> [!IMPORTANT]
> To use **Apply Textures** on 3D models in the GUI, you must first select the active level folder from the **Settings** menu to resolve the correct texture mappings.

> [!NOTE]
> **AI Models / Characters Support**: As of v1.7.0, AI character bone models, parsing (DNER), and textures (including upside-down mapping fixes) are now fully supported.

> [!NOTE]
> **v1.7.0 Updates**: 
> - The GUI "Export to Obj" now intuitively prompts for a destination folder when exporting both **binary** and **ASCII/text** `MEF/MEX` models. All textures and `.mtl` materials are generated in the chosen output directory seamlessly without cluttering the game directory.
> - **Recursive ATTA Support**: Both "Export to Obj" and "Build Rigid Model" now walk the full attachment hierarchy, merging all sub-models and resolving their textures automatically from the level's DAT file.
> - **Fixed Bone Model Rendering**: Character bone models (type1) now render correctly in the GUI with proper world-space positioning.


### GUI Screenshots

**_1. Main View Interface_**  
![Main View](assets/01_main_view.png)  
*This screenshot showcases the primary **IGI Game Converter** graphical interface. It actively features the robust directory tree on the left panel, allowing seamless navigation and real-time visualization of supported asset files.*
**_2. Intelligent Model Search_**  
![Model Search](assets/02_model_search.png)  
*Demonstrates the powerful global model search functionality. By seamlessly filtering the massive directory tree, users can instantly locate specific internal game assets, drastically accelerating extensive modding and reverse-engineering workflows.*

**_3. Texture Renderer (TEX)_**  
![Texture View 01](assets/03a_tex_000_01.png)  
*Presents the native 2D image renderer displaying standard proprietary `.tex` files. This highly optimized viewer decodes internal texture chunks flawlessly, offering direct visual inspection of weapon skins, characters, and environmental surfaces.*

**_4. High-Res Texture Inspection_**  
![Texture View 04](assets/03b_tex_000_04.png)  
*Highlights the image viewer processing larger texture assets. It efficiently unpacks complex multi-layered game textures on-the-fly, providing critical diagnostic insights and verifying accurate pixel-perfect extraction for custom modding applications.*

**_5. Texture Context Menu_**  
![Texture Context Menu](assets/03c_tex_context_menu.png)  
*Reveals the interactive context menu available on `.tex` files. This vital popup menu grants immediate access to essential conversion commands, dynamically bridging the GUI with the underlying CLI extraction and repacking tools.*

**_6. Texture Paint & Drawing Editor (Edit Game Assets)_**  
<p align="center">
  <img src="assets/05a_tex_paint.png" width="32%" alt="Texture Paint Tools" />
  <img src="assets/05aa_tex_paint.png" width="32%" alt="Texture Paint Canvas" />
  <img src="assets/05b_tex_paint.png" width="32%" alt="Texture Paint Brush" />
</p>
<p align="center">
  <img src="assets/05c_tex_paint.png" width="49%" alt="Editing Texture Asset" />
  <img src="assets/05d_tex_paint.png" width="49%" alt="Save Modified Asset" />
</p>
*Showcases the integrated advanced Image Editor. With this tool, we can edit Game assets directly; users can draw, paint, erase, change pen size/color, and save modified texture assets directly back into the game's proprietary format.*

**_7. Raw 3D Mesh Viewer (MEF)_**  
![3D Raw Viewer](assets/04a_mef_3d_raw.png)  
*Shows the built-in interactive 3D renderer displaying a `.mef` model without applied textures. It provides smooth camera navigation, allowing modders to closely examine the structural geometry and skeletal framework of game objects.*

**_8. 3D Model Zoom Inspection_**  
![3D Zoomed View](assets/04_mef_003_zoomed.png)  
*Displays the 3D viewer dynamically zoomed in on a specific geometric mesh component. This fine-tuned hardware-accelerated OpenGL view is essential for inspecting intricate polygonal details and diagnosing precise vertex modeling structures.*

**_9. 3D Wireframe Rendering_**  
![3D Wireframe](assets/04c_3d_wireframe.png)  
*Features the 3D viewer explicitly toggled into wireframe rendering mode. This highly technical diagnostic view uncovers the underlying polygonal topography, exposing hidden mesh complexities critical for advanced geometric model reconstruction.*

**_10. Mesh Context Menu_**  
![MEF Context Menu](assets/04c_mef_context_menu.png)  
*Illustrates the interactive right-click menu tailored specifically for `.mef` 3D model files. It exposes critical workflow actions like automated texture application, batch bundle processing, and exporting models directly into the universal OBJ format. **Note: For "Apply Textures" to map textures correctly on 3D models, you must first select the level from settings to get the texture mapping.***

**_11. Integrated Text Editor_**  
![Text View](assets/07a_qvm_text_view.png)  
*Captures the built-in text editor actively displaying decompiled `.qsc` script files. It features an integrated file-searching utility and provides a lightweight, seamless environment for analyzing and actively modifying raw game logic.*

**_12. Hexadecimal Inspector_**  
![Hex View](assets/07a_qvm_hex_view.png)  
*Showcases the integrated Hex View inspector toggled for raw binary analysis. This specialized mode empowers developers to meticulously investigate unrecognized proprietary file structures byte-by-byte for deep reverse-engineering capabilities.*

**_13. UI Themes_**  
<p align="center">
  <img src="assets/08c_theme_dark.png" width="32%" alt="Dark Theme" />
  <img src="assets/08c_theme_military.png" width="32%" alt="Military Theme" />
  <img src="assets/08c_theme_solarized.png" width="32%" alt="Solarized Theme" />
</p>
*Customizable visual interfaces including Dark, Military, and Solarized styles.*

**_14. About & Documentation_**  
![About Dialog](assets/09b_about_dialog.png)  
*Features the comprehensive informational dialog box. It elegantly provides crucial versioning details, integrated clickable GitHub repository documentation links, and essential architectural overviews regarding the core C++ conversion engine and Qt framework.*

---

## 🐚 CLI Version (Command-Line Interface)

A lightweight, high-performance command-line utility optimized for scripting, automated builds, and batch tasks:
*   **Asset Extraction**: Unpack and pack `.res` archives.
*   **Mesh Conversion**: Export proprietary `.mef` models directly to standard `.obj` files.
*   **Texture Conversion**: Convert images to and from `.tex`, `.spr`, and `.pic` formats (with resizing).
*   **Script Decompilation**: Decompile `.qvm` bytecode to `.qsc` source, and compile `.qsc` back to `.qvm`.
*   **Metadata Editing**: Export `.dat` mappings to JSON and compile them back to binary `.dat`/`.mtp` packages.
*   **Batch Operations**: Recursively process entire folders of assets in one command.

### CLI Usage & Commands

Below are practical, real-world examples showing how to leverage the `igi1conv` CLI to mod Project IGI 1 assets:

#### 1. Working with Textures (`.tex`, `.spr`, `.pic`)
The game stores textures in a proprietary format. You can export them to `.png` to edit them, and then repack them.
```bash
# Get information about a texture (dimensions, mipmaps, mode)
igi1conv tex info textures/FLARE00.TEX

# Export a texture to PNG for editing
igi1conv tex to-png textures/FLARE00.TEX -o my_edits/FLARE00.png

# Resize and export simultaneously
igi1conv tex to-png textures/arrow1_1.spr -o out/arrow1_1.png --resize 32 32

# Convert your edited PNG back to the game's TEX format
igi1conv tex to-tga my_edits/FLARE00.png -o textures/FLARE00.TEX
```

#### 2. Exporting 3D Meshes (`.mef`)
Extract weapons, characters, or level geometry into `.obj` format.
```bash
# Dump the structural data of a mesh to a text file for inspection
igi1conv mef dump models/model1.mef -o model1_struct.txt

# Export a single model to OBJ
igi1conv mef export models/model1.mef -o model1.obj

# Export all models in a folder iteratively (Batch Mode)
igi1conv mef export models/weapons/ -o output_objs/ --batch

# Bundle a MEF with its actual textures (requires the map's .dat file)
igi1conv mef bundle models/level1/model2.mef -o out_model2 --dat common.dat --texdir textures/

# Recursively merge all ATTA sub-models into a single rigid MEF geometry (flatten hierarchy)
igi1conv mef build-rigid models/435_01_1.mef -o models/435_01_1_rigid.mef

# Build rigid model and resolve all textures from the level DAT file
igi1conv mef build-rigid models/435_01_1.mef -o models/435_01_1_rigid.mef --dat level1.dat --texdir textures/
```

#### 3. Modifying Game Logic (`.qvm` & `.qsc`)
The game logic is driven by "Q" scripts. You can decompile them, edit the logic, and put them back.
```bash
# Decompile the AMMO script binary into human-readable source code
igi1conv qvm decompile scripts/AMMO.QVM -o scripts_src/AMMO.qsc

# Edit AMMO.qsc in your favorite text editor, then check for syntax errors
igi1conv qsc validate scripts_src/AMMO.qsc

# Re-compile your modified source code back into bytecode for the game
igi1conv qsc compile scripts_src/AMMO.qsc -o scripts/AMMO.QVM
```

#### 4. Unpacking & Packing Archives (`.res`)
`.res` files are the main asset containers in IGI.
```bash
# List all files inside SOUNDS.RES
igi1conv res list SOUNDS.RES

# Unpack the entire archive into a directory
igi1conv res unpack SOUNDS.RES sounds_unpacked/

# Modify some files in sounds_unpacked/, then pack it back into a new RES file
igi1conv res pack sounds_unpacked/ MODDED_SOUNDS.RES
```

#### 5. Editing Level Mappings (`.dat` & `.mtp`)
`.dat` files hold mappings for which textures apply to which models.
```bash
# Export the level's DAT file to JSON to easily view and edit mappings
igi1conv dat export level1.dat -o level1_mappings.json

# If you edit the JSON, you can convert it back to DAT (or the binary MTP format)
igi1conv dat to-mtp level1_modded.dat -o level1.mtp
```

### Supported Formats Reference

| Format | Extension | Operations Available |
| :------- | :----- | :--------- |
| **Textures** | `.tex`, `.spr`, `.pic` | info, decode, to-png, to-tga (+resize) |
| **3D Meshes** | `.mef` | info, dump, export (OBJ), bundle, to-text, compile, build-rigid |
| **Q Source** | `.qsc` | validate, compile → QVM |
| **Q VM** | `.qvm` | info, disasm, decompile → QSC |
| **Archives** | `.res` | list, extract, compile, pack, unpack, append |
| **Model Packages** | `.mtp` | info, dump, to-dat, repair, sync |
| **Model Mappings** | `.dat` | info, export, to-mtp |
| **Fonts** | `.fnt` | info, export (PNG atlas) |
| **Terrain** | `.lmp`, `.ctr` | info, export-lmp (PGM), export-ctr (JSON) |
| **AI Graph** | `graph*.dat` | info, export (JSON) |

See [`docs/COMMANDS.md`](docs/COMMANDS.md) for the full command tree, [`docs/SUPPORTED_FORMATS.md`](docs/SUPPORTED_FORMATS.md) for a summary of supported and planned formats, [`docs/ISSUES.md`](docs/ISSUES.md) for unimplemented features/conversions, and [`docs/game_file_formats.md`](docs/game_file_formats.md) for in-depth binary layouts.

## Building & Deployment

### Requirements
*   **CMake** ≥ 3.16
*   **C++20 Compiler** (MSVC 2022 / 2019 on Windows)
*   **Qt SDK**:
    *   **64-bit (x64)**: Qt 6.5.3 or newer (system default)
    *   **32-bit (Win32)**: Qt 5.15.2 (locally provided at `5.15.2/msvc2019`)

The build system automatically detects the target architecture from the CMake `-A` generator option and configures dependencies:
*   **Win32 builds** target **Qt5** (resolving from the local `5.15.2/msvc2019` folder) and output compiled binaries to `bin/Release32/`.
*   **x64 builds** target **Qt6** (resolving from the system `D:/Qt/6.5.3/msvc2019_64`) and output compiled binaries to `bin/Release/`.

---

### 🛠️ Step 1: Build from Source

#### Compile 32-bit (Win32) Release Build:
```powershell
# Configure for 32-bit (Win32) MSVC
cmake -S . -B build32 -G "Visual Studio 17 2022" -A Win32

# Compile the Release target
cmake --build build32 --config Release --target igi1conv
```
This generates `igi1conv.exe` and automatically copies the required runtime assets (`IGIModels.json`, `IGIModelsAllLevel.json`, and `IGIAutoComplete.txt`) into `bin/Release32/`.

#### Compile 64-bit (x64) Release Build:
```powershell
# Configure for 64-bit (x64) MSVC
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# Compile the Release target
cmake --build build --config Release --target igi1conv
```
This generates `igi1conv.exe` and automatically copies the required runtime assets (`IGIModels.json`, `IGIModelsAllLevel.json`, and `IGIAutoComplete.txt`) into `bin/Release/`.

The version string is defined once by the CMake `project(... VERSION 1.7.0)` declaration and injected as `IGI1CONV_VERSION`; `igi1conv --version` reports it.

---

### 📦 Step 2: Deploy Dependencies (Standalone Package)

To run the application outside of your build environment (or distribute it to others), you must deploy the required Qt DLLs and plugins. Use the appropriate `windeployqt` tool:

#### Deploy 32-bit (Win32) Dependencies:
```powershell
# Run 32-bit windeployqt to copy Qt5 DLLs and plugins to the Release32 directory
D:\Code\project-igi-conv\5.15.2\msvc2019\bin\windeployqt.exe bin/Release32/igi1conv.exe
```
This will copy `Qt5Core.dll`, `Qt5Widgets.dll`, `platforms/qwindows.dll`, and all other 32-bit runtime dependencies directly into the `bin/Release32/` directory.

#### Deploy 64-bit (x64) Dependencies:
```powershell
# Run 64-bit windeployqt to copy Qt6 DLLs and plugins to the Release directory
D:\Qt\6.5.3\msvc2019_64\bin\windeployqt.exe bin/Release/igi1conv.exe
```
This will copy `Qt6Core.dll`, `Qt6Widgets.dll`, `platforms/qwindows.dll`, and all other 64-bit runtime dependencies directly into the `bin/Release/` directory.

## Testing

The test suite (`igi1conv_tests`, GoogleTest) spawns the freshly built `igi1conv.exe` and runs every command and conversion against a corpus of real game files.

```powershell
cmake --build build --config Release          # builds igi1conv + igi1conv_tests
ctest --test-dir build -C Release --output-on-failure
```

You can also use the integrated CLI advanced test suite if you want to test conversions natively against a live game folder:
```powershell
igi1conv test --game-path "D:/IGI1"
```

## Relationship to the IGI Editor

`igi1conv` was originally built internally specifically to power the [project-igi-editor](https://github.com/jones-hm/project-igi-editor). However, we later decided to make it a standalone command-line tool for the community so anyone can convert and mod game assets directly. The editor still consumes a prebuilt `igi1conv.exe` (committed at `editor/tools/igi1conv.exe`) to execute complex conversion workflows behind a graphical interface, but this repository is the standalone source of truth for the parsers and tool releases.

## Branches

- **`main`** — released, tagged versions.
- **`develop`** — active development. Open PRs against `develop`.

## License

MIT — see [LICENSE](LICENSE).
