# igi1conv — Command Reference

`igi1conv` is a standalone CLI for reading, converting, and inspecting *Project IGI
1* game assets. It has no OpenGL or editor dependency.

```powershell
igi1conv --help
igi1conv --version
igi1conv <command> --help
```

**Exit codes:** `0` success · `1` bad args · `2` file not found · `3` parse error · `4` write error

---

## `tex` — Textures (TEX / SPR / PIC)

```powershell
igi1conv tex info   <input.tex|.spr|.pic>
igi1conv tex decode <input.tex|.spr|.pic> -o <output_dir>
igi1conv tex decode <folder/> -o <output_dir> --batch
igi1conv tex to-png <input> [-o <out.png>] [--resize <W> <H>]
igi1conv tex to-tga <input> [-o <out.tga>] [--resize <W> <H>]
```

`to-png` / `to-tga` accept `.tex .spr .pic .tga .png .bmp .jpg .jpeg` as input.
`-o` is optional (defaults to the input path with the new extension).

## `mef` — 3D Meshes

```powershell
igi1conv mef info   <input.mef>
igi1conv mef dump   <input.mef> [-o <output.txt>]
igi1conv mef export <input.mef> -o <output.obj>
igi1conv mef export <folder/> -o <output_dir> --batch
igi1conv mef bundle <input.mef> -o <outdir> --dat <file.dat> --texdir <dir>
igi1conv mef to-text <input.mef> -o <output.txt>
igi1conv mef compile <input.txt> -o <output.mef>
igi1conv mef build-rigid <input.mef> [-o <output.mef>]
```

## `qsc` — Q Script (source)

```powershell
igi1conv qsc validate <file.qsc>
igi1conv qsc compile  <file.qsc> -o <out.qvm>
```

## `qvm` — Q VM (bytecode)

```powershell
igi1conv qvm info      <file.qvm>
igi1conv qvm disasm    <file.qvm> [-o <output.txt>]
igi1conv qvm decompile <file.qvm> -o <out.qsc>
```

## `res` — RES Archives

```powershell
igi1conv res list    <input.res>
igi1conv res extract <input.res> -o <output_dir> [--file <name>]
igi1conv res compile <file.qsc>
igi1conv res pack    <dir> <out.res>
igi1conv res unpack  <file.res> <dir>
igi1conv res append  <input.res> <file1> [file2 ...] -o <out.res> [--prefix LOCAL:textures/]
```

## `mtp` — Model-Texture Package (binary FORM/IFF)

```powershell
igi1conv mtp info   <input.mtp>
igi1conv mtp dump   <input.mtp> [-o <output.json>]
igi1conv mtp to-dat <input.mtp> [-o <out.dat>]
igi1conv mtp repair <input.mtp>                 # sync VNAM/GTT counts
igi1conv mtp sync   <input.mtp> <input.dat>     # add DAT models missing from MTP
```

## `dat` — Model-Texture Data (text)

```powershell
igi1conv dat info   <file.dat>
igi1conv dat export <file.dat> [-o <out.json>] [--filter <model>] [--text]
igi1conv dat to-mtp <file.dat> [-o <out.mtp>]
```

`mtp` and `dat` describe the same model→texture mapping in two formats; `mtp to-dat`
and `dat to-mtp` convert between them.

## `fnt` — Bitmap Fonts

```powershell
igi1conv fnt info   <file.fnt>
igi1conv fnt export <file.fnt> -o <out.png>
```

## `terrain` — Lightmaps & Quad-Tree

```powershell
igi1conv terrain info       <file.lmp|.ctr>
igi1conv terrain export-lmp <file.lmp> -o <output.pgm>   # 16-bit PGM(s)
igi1conv terrain export-ctr <file.ctr> -o <output.json>
```

## `graph` — AI Navigation Graph

```powershell
igi1conv graph info   <file.dat>
igi1conv graph export <file.dat> -o <out.json>
```

---

For the binary layouts behind each format, see
[`game_file_formats.md`](game_file_formats.md) and
[`game_parsers.md`](game_parsers.md).
