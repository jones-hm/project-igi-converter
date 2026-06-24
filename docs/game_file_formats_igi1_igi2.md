# IGI 1 vs IGI 2 File Formats

This document summarizes which file formats are **shared** between
*Project I.G.I.: I'm Going In* (IGI 1) and *IGI 2: Covert Strike*,
and which formats are **game‑specific or significantly evolved**
in IGI 2.

It is a high‑level companion to:

- [game_file_formats.md](./game_file_formats.md) – detailed IGI file format reference.[^game-formats]
- The per‑format docs in [igipy](https://github.com/artiom-rotari/igipy) under:
  - `docs/core/formats/*.md` – shared formats (TEX, SPR, PIC, WAV, QVM VM),
  - `docs/igi1/formats/*.md` – IGI 1 formats,
  - `docs/igi2/formats/*.md` – IGI 2 formats.

---

## 1. Shared formats (same extension, similar structure)

These formats are used by **both games** with compatible containers and overall
semantics. IGI 2 often adds more variants or richer usage, but a single
converter can generally support both via version checks or a `game=igi1|igi2`
switch.

| Extension | IGI 1 | IGI 2 | Container / ID          | Role                                                     |
|----------|------:|------:|-------------------------|----------------------------------------------------------|
| `.res`   | Yes   | Yes   | ILFF `IRES`             | Resource archive bundling MEF, TEX, QVM, etc.           |
| `.mef`   | Yes   | Yes   | ILFF `HSEM`             | 3D models (buildings, props, characters, vehicles).      |
| `.tex`   | Yes   | Yes   | `LOOP` v2/7/9/11        | Texture images (shared header/version scheme).          |
| `.spr`   | Yes   | Yes   | Core SPR                | Sprite / 2D images (particles, HUD, etc.).              |
| `.pic`   | Yes   | Yes   | Core PIC                | GUI/menu pictures and other static images.              |
| `.wav`   | Yes   | Yes   | InnerLoop ADPCM WAV     | Audio samples in a custom WAV dialect.                  |
| `.qvm`   | Yes   | Yes   | `LOOP`, VM v8.5         | Script bytecode for the IGI virtual machine.            |
| `.qsc`   | Yes   | Yes   | Plain text              | Script source compiled into QVM (same language).        |
| `.mtp`   | Yes   | Yes   | FORM/IFF `MTP `         | Model–texture package (MODS/TEXF/INST/VNAM, etc.).      |
| `.dat` (MTP table) | Yes | Yes | Plain text         | Per‑level model→texture table corresponding to MTP.     |
| `dat_graph` | Yes | Yes  | Graph data              | AI / navigation graph data.                             |
| `.fnt`   | Yes   | Yes   | ILFF `FONT`             | Bitmap font atlas (glyph metrics + texture).            |
| `.olm`   | Yes   | Yes   | Flat binary             | Object static lightmaps (1 layer in IGI 1, 1 or 3 in IGI 2). |

**Implementation note**

In `project-igi-converter`, these are strong candidates for a **shared core
module** (e.g. `core/formats/*`) with game‑specific behaviour selected by:

- file contents (version fields, chunk presence), and/or
- a `--game=igi1|igi2` flag in the CLI/GUI.

---

## 2. IGI 1–only formats

These formats are documented only for IGI 1 and do not have a direct IGI 2
counterpart in igipy’s IGI 2 docs.

| Extension | Game  | Role / Notes                                  |
|----------|-------|-----------------------------------------------|
| `.bit`   | IGI 1 | Small BIT format used only in IGI 1 content/tools. |
| `.cmd`   | IGI 1 | Command/config format specific to IGI 1.      |
| `.ctr`   | IGI 1 | Control/terrain‑related data for IGI 1.       |
| `.hmp`   | IGI 1 | Heightmap format for IGI 1 terrain.           |
| `.lmp`   | IGI 1 | Lightmap format for IGI 1 terrain.            |

In IGI 2, terrain/level metadata moves to a different format set (`.thm`,
`.tlm`, `.tmm`) plus new per‑level DAT variants.

**Implementation note**

These are good to group into a **legacy IGI1 terrain & engine module**:

- IGI 1 terrain converter: `.hmp` + `.lmp` + `.ctr` + related DATs.
- IGI 1 tools that will never be used in IGI 2 pipelines.

---

## 3. IGI 2–only formats

These formats are only documented for IGI 2 and do not exist as separate
formats in IGI 1.

| Extension / name | Game  | Role / Notes                                                   |
|------------------|-------|----------------------------------------------------------------|
| `dat_forest`     | IGI 2 | Vegetation instance placements (position/rotation/scale/RGB per tree) per level. |
| `dat_graphcover` | IGI 2 | Extra cover/visibility metadata layered on top of navigation graphs. |
| `.syn`           | IGI 2 | Synchronization/timing metadata for level audio/physics.      |
| `.thm`           | IGI 2 | Terrain heightmap metadata (referenced from `heightmaps.res`). |
| `.tlm`           | IGI 2 | Terrain lightmap metadata.                                     |
| `.tmm`           | IGI 2 | Terrain material/mission metadata table.                       |

These collectively replace the IGI 1 terrain pipeline based on `.hmp`/`.lmp`
and provide a more structured level/terrain metadata layer.

**Implementation note**

These belong in an **IGI2 level/terrain module**, separate from the shared core:

- IGI 2 heightmaps and lightmaps: `.thm` + `.tmm` + `.tlm`.
- IGI 2 forest/cover: `dat_forest` + `dat_graph` + `dat_graphcover`.

---

## 4. Same extension, but evolved format in IGI 2

Some shared extensions are effectively **“same family, more complex”** in IGI 2,
even though the low‑level container is compatible.

| Extension | Commonality | IGI 2 differences (high‑level) |
|----------|-------------|---------------------------------|
| `.mef`   | Both games use ILFF `HSEM` chunks for model data. | IGI 2 adds stricter render/collision/portal semantics, more verified variants (lightmapped terrain, advanced rigs), and a full FBX‑based exporter. |
| `.iff`   | Both games use IFF‑style animation containers. | IGI 2 docs describe more detailed timing behaviour, bone‑count variants, and special weapon animation rules. |
| `.qvm`   | Same VM container (LOOP magic, v8.5 opcodes). | IGI 2 has a much richer script catalog (AI, physics, per‑level `objects.qvm`, config, menus); QVM content is more complex even though the bytecode format is shared. |
| `dat_graph` | Both games use graph DATs. | IGI 2 adds `dat_graphcover` plus more consistent per‑level graph usage. |

**Implementation note**

For these it’s usually best to:

- Share the **binary reader** (same base container/fields).
- Branch behaviour on `game=igi1|igi2` and/or extra fields:
  - e.g. MEF export options, graph tooling, script decompiler support.

---

## 5. Converter‑design view

From the converter’s perspective the split looks like this:

- **Shared core module** (both games):
  - `.res`, `.mef`, `.tex`, `.spr`, `.pic`, `.wav`, `.qvm`, `.qsc`,
    `.mtp`, `.dat` (MTP table), `dat_graph`, `.fnt`, `.olm`.
- **IGI 1–only module**:
  - `.bit`, `.cmd`, `.ctr`, `.hmp`, `.lmp`.
- **IGI 2–only module**:
  - `dat_forest`, `dat_graphcover`, `.syn`, `.thm`, `.tlm`, `.tmm`.

This layout aligns `project-igi-converter` with the structure of `igipy` and
makes it straightforward to expose a **game selector** (`--game=igi1|igi2`)
in the CLI and UI.

---

[^game-formats]: `docs/game_file_formats.md` in this repository.
