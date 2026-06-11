# igi1conv — Project IGI 1 Game Asset Converter

`igi1conv` is a standalone command-line converter and inspector for *Project IGI 1* game files. Inspired by the original IGI1Conv shipped with IGI 2, it allows you to seamlessly read, convert, and inspect the engine's proprietary formats—textures, meshes, scripts, archives, terrain, fonts, and AI graphs—with **no OpenGL or game-editor dependency**.

It's the ultimate tool for modders and researchers looking to extract game assets, modify them using modern tools, and inject them back into the game.

## Core Features
*   **Asset Extraction**: Unpack `.res` archives into loose files.
*   **Mesh Conversion**: Export proprietary `.mef` meshes to standard `.obj` files for editing in Blender/Maya.
*   **Texture Conversion**: Convert `.tex`, `.spr`, and `.pic` images into editable formats like `.png` or `.tga`, and even convert standard images back into the game's formats.
*   **Script Decompilation**: Decompile `.qvm` binary bytecode into readable `.qsc` source files, edit the logic, and recompile back into bytecode.
*   **Metadata Editing**: Manipulate `.dat` and `.mtp` mapping packages natively to assign textures or models dynamically.

## Quick Start Examples

Below are practical, real-world examples showing how to leverage `igi1conv` to mod Project IGI 1 assets:

### 1. Working with Textures (`.tex`, `.spr`, `.pic`)
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

### 2. Exporting 3D Meshes (`.mef`)
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
```

### 3. Modifying Game Logic (`.qvm` & `.qsc`)
The game logic is driven by "Q" scripts. You can decompile them, edit the logic, and put them back.
```bash
# Decompile the AMMO script binary into human-readable source code
igi1conv qvm decompile scripts/AMMO.QVM -o scripts_src/AMMO.qsc

# Edit AMMO.qsc in your favorite text editor, then check for syntax errors
igi1conv qsc validate scripts_src/AMMO.qsc

# Re-compile your modified source code back into bytecode for the game
igi1conv qsc compile scripts_src/AMMO.qsc -o scripts/AMMO.QVM
```

### 4. Unpacking & Packing Archives (`.res`)
`.res` files are the main asset containers in IGI.
```bash
# List all files inside SOUNDS.RES
igi1conv res list SOUNDS.RES

# Unpack the entire archive into a directory
igi1conv res unpack SOUNDS.RES sounds_unpacked/

# Modify some files in sounds_unpacked/, then pack it back into a new RES file
igi1conv res pack sounds_unpacked/ MODDED_SOUNDS.RES
```

### 5. Editing Level Mappings (`.dat` & `.mtp`)
`.dat` files hold mappings for which textures apply to which models.
```bash
# Export the level's DAT file to JSON to easily view and edit mappings
igi1conv dat export level1.dat -o level1_mappings.json

# If you edit the JSON, you can convert it back to DAT (or the binary MTP format)
igi1conv dat to-mtp level1_modded.dat -o level1.mtp
```

---

## Supported Formats Reference

| Format | Extension | Operations Available |
| :------- | :----- | :--------- |
| **Textures** | `.tex`, `.spr`, `.pic` | info, decode, to-png, to-tga (+resize) |
| **3D Meshes** | `.mef` | info, dump, export (OBJ), bundle |
| **Q Source** | `.qsc` | validate, compile → QVM |
| **Q VM** | `.qvm` | info, disasm, decompile → QSC |
| **Archives** | `.res` | list, extract, compile, pack, unpack, append |
| **Model Packages** | `.mtp` | info, dump, to-dat, repair, sync |
| **Model Mappings** | `.dat` | info, export, to-mtp |
| **Fonts** | `.fnt` | info, export (PNG atlas) |
| **Terrain** | `.lmp`, `.ctr` | info, export-lmp (PGM), export-ctr (JSON) |
| **AI Graph** | `graph*.dat` | info, export (JSON) |

See [`docs/COMMANDS.md`](docs/COMMANDS.md) for the full command tree and [`docs/game_file_formats.md`](docs/game_file_formats.md) for in-depth binary layouts.

## Building

Requirements: CMake ≥ 3.16 and a C++20 compiler (MSVC 2022 on Windows).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release --target igi1conv
# -> bin/Release/igi1conv.exe
```

The version string is defined once by the CMake `project(... VERSION 1.0.0)` declaration and injected as `IGI1CONV_VERSION`; `igi1conv --version` reports it.

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
