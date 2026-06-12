# Project IGI 1 Game Convertor — Open Issues & Unimplemented Conversions

This document tracks unimplemented features, formats, and missing reverse-engineering conversions for future development.

## Missing Conversions (Formats & Inbound Compilers/Injectors)

- [ ] **WAV (Audio)**: Implement decoding of proprietary InnerLoop ADPCM sound files to standard WAV, and encoding of standard WAV back to the game's ADPCM format.
- [ ] **QAS (AI Scripts)**: Decompile binary AI pathing and action script structures (`.qas`) to human-readable text and compile them back.
- [ ] **ILFF (Containers)**: Implement direct standalone CLI options to extract/pack the InnerLoop File Format (ILFF) container structure.

