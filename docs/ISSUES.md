# Project IGI 1 Game Convertor — Open Issues & Unimplemented Conversions

This document tracks unimplemented features, formats, and missing reverse-engineering conversions for future development.

## 1. Missing Format Converters (Inbound/Outbound)

- [ ] **WAV Audio Decoder/Encoder**
  - **Description**: Support for proprietary InnerLoop ADPCM `.wav` audio files.
  - **Goal**: Implement decoding of game sound files to standard WAV, and encoding of standard WAV back to IGI-compatible ADPCM sound format.

- [ ] **QAS Script Compiler & Decompiler**
  - **Description**: Modifying AI pathing/action script packages (`.qas`).
  - **Goal**: Decompile binary `.qas` script structures to text representation and compile them back to binary.

- [ ] **Direct ILFF Container Pack/Unpack**
  - **Description**: Standalone CLI actions to extract and package ILFF binary files.
  - **Goal**: Allow users to manipulate chunk headers ("DNER", "XTRV", etc.) directly without invoking the mesh or archive managers.

---

## 2. Missing Inbound Conversions (Compilers / Injectors)

The editor currently supports exporting the following files, but lacks the ability to compile them back into native game assets:

- [ ] **3D Mesh Compiler (OBJ → MEF)**
  - **Status**: Read-Only (`MEF → OBJ`).
  - **Goal**: Read Wavefront `.obj` files and compile them into native `.mef` 3D bone and render blocks.

- [ ] **Font Recompiler (PNG → FNT)**
  - **Status**: Read-Only (`FNT → PNG` atlas).
  - **Goal**: Recompile modified PNG font sheets and character mappings back into native `.fnt` format.

- [ ] **Terrain Recompiler (PGM/JSON → LMP/CTR)**
  - **Status**: Read-Only (`LMP/CTR → PGM/JSON`).
  - **Goal**: Rebuild heightmaps and cube metadata files from raw images/JSON files.

- [ ] **AI Navigation Recompiler (JSON → Graph)**
  - **Status**: Read-Only (`Graph → JSON`).
  - **Goal**: Convert JSON node lists and edge properties back into binary `graph*.dat` files.
