# Project IGI 1 Game Converter — Open Issues & Unimplemented Conversions

This document tracks unimplemented features, formats, and missing reverse-engineering conversions for future development.

## 1. Missing Format Converters (WAV, QAS, ILFF)

- [ ] **Proprietary Game Formats Support**
  - **WAV (Audio)**: Implement decoding of proprietary InnerLoop ADPCM sound files to standard WAV, and encoding of standard WAV back to the game's ADPCM format.
  - **QAS (AI Scripts)**: Decompile binary AI pathing and action script structures (`.qas`) to human-readable text and compile them back.
  - **ILFF (Containers)**: Implement direct standalone CLI options to extract/pack the InnerLoop File Format (ILFF) container structure.

---

## 2. Missing Inbound Compilers / Injectors (OBJ→MEF, PNG→FNT, PGM/JSON→LMP/CTR, JSON→Graph)

- [ ] **Asset Compilation and Game Injection**
  - **3D Mesh Compiler (OBJ → MEF)**: Convert Wavefront `.obj` files back into native `.mef` mesh render and bone structures.
  - **Font Recompiler (PNG → FNT)**: Rebuild font character mappings and sheet data from modified PNG texture sheets back into `.fnt` format.
  - **Terrain Recompiler (PGM/JSON → LMP/CTR)**: Compile heightmap images (`.pgm`) and cube properties (`.json`) back into game terrain formats.
  - **AI Navigation Graph Recompiler (JSON → Graph)**: Recompile modified JSON node/edge data back into the game's binary `graph*.dat` navigation files.

