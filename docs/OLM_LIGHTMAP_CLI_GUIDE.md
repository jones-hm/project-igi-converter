# OLM / Lightmap — CLI Usage Guide

Step-by-step instructions for applying lightmaps to any `.mef` model using
`igi1conv`. This doc only covers **which commands to run and which args to
pass** — for the binary file format internals, see
[`game_file_formats.md` §13](game_file_formats.md#13-olm----lightmap) and
[`Lightmap_docs.md`](Lightmap_docs.md).

---

## Table of Contents

1. [Before you start](#1-before-you-start)
2. [Step 1 — Decompile `objects.qvm` to `objects.qsc`](#2-step-1--decompile-objectsqvm-to-objectsqsc)
3. [Step 2 — Find which lightmap(s) a model uses](#3-step-2--find-which-lightmaps-a-model-uses)
4. [Step 3 — Disambiguate when a model is placed more than once](#4-step-3--disambiguate-when-a-model-is-placed-more-than-once)
5. [Step 4 — Export the resolved `.olm` files to PNG/TGA](#5-step-4--export-the-resolved-olm-files-to-pngtga)
6. [Step 5 — View the lightmap blended onto the model (GUI)](#6-step-5--view-the-lightmap-blended-onto-the-model-gui)
7. [`olm` command reference](#7-olm-command-reference)
8. [`lightmap` command reference](#8-lightmap-command-reference)
9. [Exit codes](#9-exit-codes)
10. [Common errors & fixes](#10-common-errors--fixes)
11. [Full worked example](#11-full-worked-example)

---

## 1. Before you start

You need:

- A level directory containing `objects.qvm` (or an already-decompiled `objects.qsc`).
- That same level's `lightmaps/lightmaps.res` (or an already-unpacked `lightmaps/lightmaps_unpacked/` folder).
- The model id you want to apply a lightmap to — this is the `.mef` filename without the extension, e.g. `435_01_1` from `435_01_1.mef`.

You do **not** need to manually unpack `lightmaps.res` — both the CLI
`lightmap resolve` command and the GUI's "Apply Lightmap" action do this
automatically the first time they need it.

---

## 2. Step 1 — Decompile `objects.qvm` to `objects.qsc`

Lightmap bindings live in the level's script, not the `.mef` file itself.
If you don't already have a decompiled `objects.qsc` next to `objects.qvm`:

```powershell
igi1conv qvm decompile "<level_dir>\objects.qvm" -o "<level_dir>\objects.qsc"
```

**Example:**

```powershell
igi1conv qvm decompile "D:\IGI1\missions\location0\level1\objects.qvm" -o "D:\IGI1\missions\location0\level1\objects.qsc"
```

If `objects.qsc` already exists next to `objects.qvm`, skip this step.

---

## 3. Step 2 — Find which lightmap(s) a model uses

```powershell
igi1conv lightmap list --model <model_id> --qsc <path_to_objects.qsc>
```

**Args:**

| Flag | Required | Meaning |
|---|---|---|
| `--model <id>` | yes | The `.mef` filename stem, e.g. `435_01_1` |
| `--qsc <path>` | yes | Path to the level's decompiled `objects.qsc` |

**Example:**

```powershell
igi1conv lightmap list --model 435_01_1 --qsc "D:\IGI1\missions\location0\level1\objects.qsc"
```

**Output:**

```
lightmap: 1 placement(s) of "435_01_1":
  task 1104 "WaterTower" -> obj00000 @ (24658470, -55957188, 174412128)
```

If the model is placed only **once** in the level, you're done — go
straight to [Step 4](#5-step-4--export-the-resolved-olm-files-to-pngtga)
or [Step 5](#6-step-5--view-the-lightmap-blended-onto-the-model-gui) using
the logical id printed (`obj00000` above).

If it shows **more than one placement**, see the next step.

---

## 4. Step 3 — Disambiguate when a model is placed more than once

The same `.mef` model can be placed at several different spots in a level,
and each placement can have its own baked lightmap. `lightmap list` shows
every placement; `lightmap resolve` picks exactly one.

```powershell
igi1conv lightmap resolve --model <model_id> --qsc <path_to_objects.qsc> [--task-id <id> | --pos X,Y,Z]
```

**Args:**

| Flag | Required | Meaning |
|---|---|---|
| `--model <id>` | yes | The `.mef` filename stem |
| `--qsc <path>` | yes | Path to the level's decompiled `objects.qsc` |
| `--task-id <id>` | only if model has >1 placement | Exact `Task_New` id of the placement you want (from `lightmap list`'s `task <id>` column) |
| `--pos X,Y,Z` | only if model has >1 placement | World position (comma-separated, no spaces) — picks the placement whose position is closest |

Pass **at most one** of `--task-id` / `--pos`. If the model has only one
placement, you can omit both.

**Example — disambiguate by task id:**

```powershell
igi1conv lightmap resolve --model 410_01_1 --qsc "D:\IGI1\missions\location0\level1\objects.qsc" --task-id 1107
```

**Example — disambiguate by position:**

```powershell
igi1conv lightmap resolve --model 410_01_1 --qsc "D:\IGI1\missions\location0\level1\objects.qsc" --pos 24932566,-56062908,174413136
```

**Output (either form):**

```
lightmap: resolved   task 1107 "SmallGarage" -> obj00003 @ (24689824, -56330996, 174413136)
lightmap: 69 .olm file(s):
  D:\IGI1\...\lightmaps_unpacked\obj00003_00000.olm
  D:\IGI1\...\lightmaps_unpacked\obj00003_00001.olm
  ...
```

If you call `resolve` **without** `--task-id`/`--pos` and the model has
more than one placement, it refuses to guess — it lists every candidate
and exits with code `4`:

```powershell
igi1conv lightmap resolve --model 410_01_1 --qsc "D:\IGI1\missions\location0\level1\objects.qsc"
```
```
lightmap: "410_01_1" is placed at 3 locations - pass --task-id or --pos to disambiguate:
  task 1106 "SmallGarage" -> obj00002 @ (24590148, -56231896, 174413136)
  task 1107 "SmallGarage" -> obj00003 @ (24689824, -56330996, 174413136)
  task 1120 "SmallGarage" -> obj00016 @ (24932566, -56062908, 174413136)
```

---

## 5. Step 4 — Export the resolved `.olm` files to PNG/TGA

Take any `.olm` file path printed by `lightmap resolve` (or `list`) and
convert it:

```powershell
igi1conv olm to-png "<path\to\obj00003_00000.olm>" -o "<output.png>"
igi1conv olm to-tga "<path\to\obj00003_00000.olm>" -o "<output.tga>"
```

`-o` is optional — if omitted, the output is written next to the input
with the new extension.

**Example:**

```powershell
igi1conv olm to-png "D:\IGI1\missions\location0\level1\lightmaps\lightmaps_unpacked\obj00003_00000.olm" -o "D:\Temp\obj00003_00000.png"
```

A model's lightmap is usually split across multiple `.olm` fragments
(`_00000`, `_00001`, ...) — convert each one you need, or loop over all of
them returned by `lightmap resolve`.

To inspect a file's dimensions before converting:

```powershell
igi1conv olm info "<path\to\file.olm>"
```

---

## 6. Step 5 — View the lightmap blended onto the model (GUI)

The CLI resolves *which* lightmap applies; the GUI is what actually
blends it onto the model in the 3D viewer. Steps:

1. Launch the GUI (`igi1conv` with no args, or `igi1conv --gui`).
2. **Settings → Animation → Set Objects.qsc...** and point it at the
   correct level's `objects.qsc` (the same path you passed to `--qsc`
   above). **This must match the model's actual level** — pointing it at
   the wrong level's `objects.qsc` is the most common cause of "no
   lightmap binding found."
3. In the file tree, right-click the `.mef` model → **Textures → Apply
   Lightmap**.
4. If the model is placed at more than one location, a picker dialog
   appears listing each placement (`name (task <id>) -> <logical_id>`) —
   pick the one you want, matching what `lightmap list` showed you.
5. The model loads in the 3D viewer with the lightmap blended onto the
   diffuse texture.

---

## 7. `olm` command reference

```powershell
igi1conv olm info   <input.olm>
igi1conv olm to-png <input.olm> [-o <out.png>]
igi1conv olm to-tga <input.olm> [-o <out.tga>]
```

| Subcommand | Required args | Optional args |
|---|---|---|
| `info` | `<input.olm>` | — |
| `to-png` | `<input.olm>` | `-o <out.png>` |
| `to-tga` | `<input.olm>` | `-o <out.tga>` |

---

## 8. `lightmap` command reference

```powershell
igi1conv lightmap list    --model <id> --qsc <objects.qsc>
igi1conv lightmap resolve --model <id> --qsc <objects.qsc> [--task-id <id> | --pos X,Y,Z]
```

| Subcommand | Required args | Optional args |
|---|---|---|
| `list` | `--model <id>`, `--qsc <path>` | — |
| `resolve` | `--model <id>`, `--qsc <path>` | `--task-id <id>` **or** `--pos X,Y,Z` (not both) |

`igi1conv lightmap --help` prints the same reference at the terminal.

---

## 9. Exit codes

| Code | Meaning |
|---|---|
| `0` | Success |
| `1` | Bad/missing arguments (e.g. no `--model`, malformed `--pos`, both `--task-id` and `--pos` given) |
| `2` | File not found (`--qsc` path doesn't exist) |
| `3` | No binding found for the model, or the resolved binding has no `.olm` files on disk |
| `4` | Ambiguous — model has multiple placements and no `--task-id`/`--pos` was given |

---

## 10. Common errors & fixes

| Symptom | Cause | Fix |
|---|---|---|
| `lightmap: no bindings found for model "X"` | Model isn't placed in **this** level, or `--qsc` points at the wrong level | Re-check the level directory; run `lightmap list` against the correct `objects.qsc` |
| `lightmap: file not found: ...` (exit 2) | `--qsc` path is wrong or `objects.qsc` hasn't been decompiled yet | Run `igi1conv qvm decompile <level>\objects.qvm -o <level>\objects.qsc` first |
| `... is placed at N locations - pass --task-id or --pos` (exit 4) | Model is reused across the level | Re-run with `--task-id <id>` or `--pos X,Y,Z` from the listed candidates |
| `lightmap: binding <id> found but no .olm files on disk` | `lightmaps.res` is missing, or the level has no baked lightmaps | Confirm `<level>\lightmaps\lightmaps.res` exists |
| GUI says "no lightmap binding found" but you know the binding exists | **Settings → Animation → Objects.qsc** in the GUI points at a *different* level than the model you're viewing | Re-point it at the correct level's `objects.qsc` |

---

## 11. Full worked example

End-to-end: find and export the lightmap for model `410_01_1`, which is
placed three times in `level1`.

```powershell
# 1. Decompile the script (skip if objects.qsc already exists)
igi1conv qvm decompile "D:\IGI1\missions\location0\level1\objects.qvm" -o "D:\IGI1\missions\location0\level1\objects.qsc"

# 2. See every placement of this model
igi1conv lightmap list --model 410_01_1 --qsc "D:\IGI1\missions\location0\level1\objects.qsc"
#   task 1106 "SmallGarage" -> obj00002 @ (24590148, -56231896, 174413136)
#   task 1107 "SmallGarage" -> obj00003 @ (24689824, -56330996, 174413136)
#   task 1120 "SmallGarage" -> obj00016 @ (24932566, -56062908, 174413136)

# 3. Resolve the placement at task 1120 specifically
igi1conv lightmap resolve --model 410_01_1 --qsc "D:\IGI1\missions\location0\level1\objects.qsc" --task-id 1120
#   lightmap: resolved   task 1120 "SmallGarage" -> obj00016 @ (24932566, -56062908, 174413136)
#   lightmap: 67 .olm file(s):
#     D:\IGI1\...\lightmaps_unpacked\obj00016_00000.olm
#     ...

# 4. Export the first fragment to PNG to preview it
igi1conv olm to-png "D:\IGI1\missions\location0\level1\lightmaps\lightmaps_unpacked\obj00016_00000.olm" -o "D:\Temp\obj00016_00000.png"

# 5. Or just open the GUI, right-click 410_01_1.mef -> Textures -> Apply Lightmap,
#    pick "SmallGarage (task 1120) -> obj00016" from the picker, and view it blended live.
```
