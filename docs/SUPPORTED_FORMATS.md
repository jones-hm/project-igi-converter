# IGI Game Convertor — Supported Formats & Conversions

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
*   **ILFF** (InnerLoop File Format Container): Direct standalone parsing and inspection of the underlying binary container (currently only processed internally via `.mef` and `.res` formats).
*   **EAD** (Sound/Event definition or auxiliary formats): Planned for future investigation and conversion support.
