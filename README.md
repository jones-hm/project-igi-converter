# igi1conv — Project IGI 1 Game Asset Converter

`igi1conv` is a standalone command-line converter and inspector for *Project IGI 1*
game files, inspired by the original IGI1Conv shipped with IGI 2. It reads, converts,
and dumps the engine's proprietary formats — textures, meshes, scripts, archives,
terrain, fonts, and AI graphs — with **no OpenGL or game-editor dependency**.

```powershell
igi1conv tex to-png FLARE00.TEX -o flare.png
igi1conv mef export model1.mef  -o model.obj
igi1conv qvm decompile AMMO.QVM  -o ammo.qsc
igi1conv dat to-mtp level1.dat   -o level1.mtp
```

## Supported formats

| Command   | Format | Operations |
| :-------- | :----- | :--------- |
| `tex`     | TEX / SPR / PIC textures | info, decode, to-png, to-tga (+resize) |
| `mef`     | MEF 3D meshes            | info, dump, export (OBJ), bundle |
| `qsc`     | Quest script (source)    | validate, compile → QVM |
| `qvm`     | Quest VM bytecode        | info, disasm, decompile → QSC |
| `res`     | RES archives             | list, extract, compile, pack, unpack, append |
| `mtp`     | Model-Texture Package    | info, dump, to-dat, repair, sync |
| `dat`     | Model-Texture Data       | info, export, to-mtp |
| `fnt`     | Bitmap fonts             | info, export (PNG atlas) |
| `terrain` | LMP lightmaps / CTR tree | info, export-lmp (PGM), export-ctr (JSON) |
| `graph`   | AI navigation graph      | info, export (JSON) |

See [`docs/COMMANDS.md`](docs/COMMANDS.md) for the full command tree and
[`docs/game_file_formats.md`](docs/game_file_formats.md) for the binary layouts.

## Building

Requirements: CMake ≥ 3.16 and a C++20 compiler (MSVC 2022 on Windows).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release --target igi1conv
# -> bin/Release/igi1conv.exe
```

The version string is defined once by the CMake `project(... VERSION 1.0.0)`
declaration and injected as `IGI1CONV_VERSION`; `igi1conv --version` reports it.

## Testing

The test suite (`igi1conv_tests`, GoogleTest) spawns the freshly built `igi1conv.exe`
and runs every command and conversion against a corpus of real game files.

```powershell
cmake --build build --config Release          # builds igi1conv + igi1conv_tests
ctest --test-dir build -C Release --output-on-failure
```

The corpus directory defaults to `D:\IGI1\full_test` and can be overridden with
the `IGI1CONV_TEST_CORPUS` environment variable. Tests whose required input file is
absent are **skipped**, not failed, so the suite stays green without the corpus.

To build without tests: `cmake -S . -B build -DIGI1CONV_BUILD_TESTS=OFF`.

## Relationship to the IGI Editor

`igi1conv` shares its format parsers with the
[project-igi-editor](https://github.com/jones-hm/project-igi-editor). The editor
consumes a prebuilt `igi1conv.exe` (committed at `editor/tools/igi1conv.exe`) and does
not build it. This repository is the source of truth for the tool and its
releases.

## Branches

- **`main`** — released, tagged versions.
- **`develop`** — active development. Open PRs against `develop`.

## License

MIT — see [LICENSE](LICENSE).
