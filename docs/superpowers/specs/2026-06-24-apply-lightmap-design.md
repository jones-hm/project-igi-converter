# Apply Lightmap on .mef (GUI 3D viewer)

## Goal
Right-click a `.mef` in the GUI → "Apply Lightmap" → automatically resolve and bind the
object's baked `.olm` lightmap(s) from the level's `objects.qsc` + `lightmaps/` folder,
and render them blended with the diffuse texture in the 3D viewer.

## Binding chain (from objects.qsc)
```
Task_New(<id>, "Building", "<name>", x,y,z, ..., gamma, "<model_id>",
    Task_New(-1, "Static", "", Task_New(-1, "EditRigidObj", "", ..., "<model_id2>", ...)),
    Task_New(-1, "LightmapInfo", "", scale, passes, hemi, dirlight, gamma, maxRad,
             ambR, ambG, ambB, "<logical_lightmap_id>"))
```
A lightmap logical id (e.g. `obj00000`) is bound to whichever `Task_New` *tree* contains
both the mef's model id (anywhere in the tree, not just the outer call) and a nested
`LightmapInfo` child. Resolves to files `<logical_id>_NNNNN.olm` in
`<level>/lightmaps/lightmaps_unpacked/`.

## Components

1. **`source/parsers/qsc_object_parser.{h,cpp}`** — add `LightmapBindingSet` with
   `static LightmapBindingSet parse(qscText)`. Walks every top-level `Task_New(...)`,
   recursively scans its nested-call blobs (currently discarded as opaque `Bad` tokens)
   for (a) any quoted string equal to a queried model stem and (b) a nested
   `Task_New("LightmapInfo", ...)` whose last string arg is the logical id. Exposes
   `logicalIdForModel(modelStem) -> optional<string>`.

2. **`source/parsers/lightmap_resolver.{h,cpp}`** (new) — given the level dir, finds
   `lightmaps/lightmaps_unpacked`; if absent but `lightmaps/lightmaps.res` exists, calls
   the existing `cmd_res` unpack logic to produce it. Globs `<logical_id>_*.olm`, returns
   paths sorted by sub-index.

3. **MEF parser (`source/parsers/mef_native.h/.cpp`, `mef_parser.h`)** — `RenderVertex`
   gains `glm::vec2 uv2`; for lightmap models (40-byte XTRV stride) the parser reads the
   second UV at vertex offset +32. `ParsedGeometry::renderBlocks[i]` (already existing,
   one per submesh) maps to `<logical_id>_{i:05d}.olm` by index; if the .olm count !=
   renderBlocks.size(), fall back to applying file 0 to every block and log a warning.

4. **GUI viewer (`igi1conv/gui_main.cpp`)** — right-click "Apply Lightmap" on `.mef`,
   reusing the existing `globalObjectsQscPath` / `globalModelsDir` Animation settings to
   derive the level dir (no new Settings UI). Loads matched `.olm`(s) via `ParseOlm`,
   uploads as GL textures (one per render block), adds a second sampler to the existing
   shader, fragment color = `diffuse * lightmap` when bound (diffuse-only otherwise — no
   behavior change for models without a lightmap).

## Out of scope
- IGI2 24-byte layer descriptor / multi-layer cascades (no IGI2 test corpus given).
- Terrain `.lmp`/`.tlm` viewer integration (separate existing `terrain` command).

## Testing
- `tests/test_igi1conv_commands.cpp`: add `OlmInfo`, `OlmToPng`, `OlmToTga` (currently
  zero OLM tests exist despite `cmd_olm` shipping).
- New `tests/test_lightmap_resolver.cpp`: parse the real decompiled `objects.qsc` +
  `D:\IGI1\missions\location0\level1\lightmaps\lightmaps_unpacked` corpus, assert the
  WaterTower/Building → `obj00000` → `obj00000_00000.olm` chain resolves.
- GUI/shader rendering verified manually (`run` skill / manual launch), not unit-testable.

## Commit plan (small, no push)
1. OLM CLI test coverage.
2. `LightmapBindingSet` parser + tests.
3. `lightmap_resolver` + tests.
4. MEF `uv2` parsing.
5. GUI "Apply Lightmap" menu + shader blend.
6. Release build.
