# IGI Game Converter — Supported Formats & Conversions

This document lists the supported file formats, their conversion targets, and what is currently missing or planned for future versions.

## Supported Formats

| Format | Description | Conversion / Operations | Status |
| :--- | :--- | :--- | :--- |
| **.res** | Resource Archive | List, extract, compile, pack, unpack, append | Supported |
| **.qvm** | Script Bytecode | Decompile to `.qsc`, disassemble, info | Supported |
| **.qsc** | Script Source Code | Validate, compile back to `.qvm` | Supported |
| **.tex** | Texture Format | Decode, info, convert to `.png` / `.tga`, resize | Supported |
| **.spr** | Sprite Format | Decode, info, convert to `.png` / `.tga`, resize | Supported |
| **.pic** | Image Format | Decode, info, convert to `.png` / `.tga`, resize | Supported |
| **.mef** | 3D Mesh Format | Info, dump, export to `.obj` + `.mtl` bundle | Supported |
| **.mtp** | Model-Texture Package | Info, dump to JSON, compile/sync to `.dat` | Supported |
| **.dat** | Level Mapping / Asset List | Info, export to JSON, compile to `.mtp` / `.dat` | Supported |
| **.fnt** | Font Format | Info, export font texture atlas to `.png` | Supported |
| **.lmp** | Terrain Heightmap | Info, export to PGM (`.pgm`) | Supported |
| **.ctr** | Terrain Cube properties | Info, export to JSON | Supported |
| **graph*.dat** | AI Navigation Graph | Info, export to JSON | Supported |

---

## Missing & Planned Formats (Future Support)

The following formats from Project I.G.I. 1 are currently missing or planned for future editions:

*   **.wav** (InnerLoop ADPCM Sound files): Decoding and converting proprietary audio files into standard WAV format.
*   **.qas** (AI Pathing / Action Scripts): Parsing, editing, and compiling AI script assets.
*   **ILFF** (InnerLoop File Format Container): Standalone extraction and packing command line options for the container wrapper.

---

## Missing Conversions (Read-Only Formats)

While several formats are supported for extraction or viewing, the write/compile operations for injecting them back into the game are currently unimplemented (read-only):

1.  **3D Models (OBJ → MEF)**: Currently, we can only export `.mef` to `.obj`. Compiling modified `.obj` files back into the game's native `.mef` mesh format is not yet supported.
2.  **Audio Encoding (WAV → IGI-ADPCM)**: Converting standard WAV files back into the game's custom compressed ADPCM sound files.
3.  **Fonts (PNG → FNT)**: Recompiling edited font texture sheets back into the `.fnt` format.
4.  **Terrain (PGM/JSON → LMP/CTR)**: Rebuilding terrain heightmaps and cube properties from standard formats.
5.  **AI Navigation (JSON → Graph)**: Recompiling JSON AI node graphs back into binary `graph*.dat` files.

