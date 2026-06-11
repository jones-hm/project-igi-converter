# gconv — Command Reference

`gconv` is a standalone CLI for reading, converting, and inspecting *Project IGI
1* game assets. It has no OpenGL or editor dependency.

```powershell
gconv --help
gconv --version
gconv <command> --help
```

**Exit codes:** `0` success · `1` bad args · `2` file not found · `3` parse error · `4` write error

---

## `tex` — Textures (TEX / SPR / PIC)

```powershell
gconv tex info   <input.tex|.spr|.pic>
gconv tex decode <input.tex|.spr|.pic> -o <output_dir>
gconv tex decode <folder/> -o <output_dir> --batch
gconv tex to-png <input> [-o <out.png>] [--resize <W> <H>]
gconv tex to-tga <input> [-o <out.tga>] [--resize <W> <H>]
```

`to-png` / `to-tga` accept `.tex .spr .pic .tga .png .bmp .jpg .jpeg` as input.
`-o` is optional (defaults to the input path with the new extension).

## `mef` — 3D Meshes

```powershell
gconv mef info   <input.mef>
gconv mef dump   <input.mef> [-o <output.txt>]
gconv mef export <input.mef> -o <output.obj>
gconv mef export <folder/> -o <output_dir> --batch
gconv mef bundle <input.mef> -o <outdir> --dat <file.dat> --texdir <dir>
```

## `qsc` — Quest Script (source)

```powershell
gconv qsc validate <file.qsc>
gconv qsc compile  <file.qsc> -o <out.qvm>
```

## `qvm` — Quest VM (bytecode)

```powershell
gconv qvm info      <file.qvm>
gconv qvm disasm    <file.qvm> [-o <output.txt>]
gconv qvm decompile <file.qvm> -o <out.qsc>
```

## `res` — RES Archives

```powershell
gconv res list    <input.res>
gconv res extract <input.res> -o <output_dir> [--file <name>]
gconv res compile <file.qsc>
gconv res pack    <dir> <out.res>
gconv res unpack  <file.res> <dir>
gconv res append  <input.res> <file1> [file2 ...] -o <out.res> [--prefix LOCAL:textures/]
```

## `mtp` — Model-Texture Package (binary FORM/IFF)

```powershell
gconv mtp info   <input.mtp>
gconv mtp dump   <input.mtp> [-o <output.json>]
gconv mtp to-dat <input.mtp> [-o <out.dat>]
gconv mtp repair <input.mtp>                 # sync VNAM/GTT counts
gconv mtp sync   <input.mtp> <input.dat>     # add DAT models missing from MTP
```

## `dat` — Model-Texture Data (text)

```powershell
gconv dat info   <file.dat>
gconv dat export <file.dat> [-o <out.json>] [--filter <model>] [--text]
gconv dat to-mtp <file.dat> [-o <out.mtp>]
```

`mtp` and `dat` describe the same model→texture mapping in two formats; `mtp to-dat`
and `dat to-mtp` convert between them.

## `fnt` — Bitmap Fonts

```powershell
gconv fnt info   <file.fnt>
gconv fnt export <file.fnt> -o <out.png>
```

## `terrain` — Lightmaps & Quad-Tree

```powershell
gconv terrain info       <file.lmp|.ctr>
gconv terrain export-lmp <file.lmp> -o <output.pgm>   # 16-bit PGM(s)
gconv terrain export-ctr <file.ctr> -o <output.json>
```

## `graph` — AI Navigation Graph

```powershell
gconv graph info   <file.dat>
gconv graph export <file.dat> -o <out.json>
```

---

For the binary layouts behind each format, see
[`game_file_formats.md`](game_file_formats.md) and
[`game_parsers.md`](game_parsers.md).
