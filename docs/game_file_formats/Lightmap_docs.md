# Lightmap and OLM Format Documentation

Technical documentation of the lightmap systems, the `.olm` (Object Lightmap) format, terrain lightmaps (`.lmp` / `.tlm`), and their integration in *Project I.G.I.: I'm Going In* (2000) and *IGI 2: Covert Strike* (2003).

---

## Table of Contents

1. [Editor Light & Smoke FX Placement](#1-editor-light--smoke-fx-placement)
2. [OLM — Object Lightmap File Format](#2-olm--object-lightmap-file-format)
    - [Overview](#21-overview)
    - [IGI 1 vs IGI 2 Differences](#22-igi-1-vs-igi-2-differences)
    - [Binary Structure (Main Header)](#23-binary-structure-main-header)
    - [Layer Descriptor (IGI 1 vs IGI 2)](#24-layer-descriptor-igi-1-vs-igi-2)
    - [Extra Block (IGI 2 Only)](#25-extra-block-igi-2-only)
    - [Pixel Data](#26-pixel-data)
    - [Layout Diagrams](#27-layout-diagrams)
3. [Script & Task Integration (QVM/QSC)](#3-script--task-integration-qvmqsc)
4. [MEF Model Type 3 (MODEL_LIGHTMAP) Integration](#4-mef-model-type-3-model_lightmap-integration)
5. [Terrain Lightmaps (LMP vs TLM)](#5-terrain-lightmaps-lmp-vs-tlm)

---

## 1. Editor Light & Smoke FX Placement

The Level Editor handles placement and authoring of light objects and smoke FX emitters which are baked into static geometry or simulated at runtime.

### Light Types & Properties
- **Point light**: Omnidirectional light source defined by position, radius, color, and intensity.
- **Spotlight**: Directional cone-falloff light source defined by position, direction, inner and outer cone angles, color, and intensity.
- **Directional light**: Sun/sky lighting defined by direction, color, and intensity (no position).

**Inspector properties for lights:**
- `position` (`vec3`)
- `color` (`RGB` float)
- `intensity` (`float`)
- `radius` / falloff (point & spot lights)
- `direction` (`vec3`, spot & directional lights)
- `inner_angle`, `outer_angle` (`float`, spotlights)
- `cast_shadows` (`bool`, enables raytraced occlusion during lightmap baking)
- `light_type` (`enum` / `int`)

### Smoke FX Properties
- `position` (`vec3`)
- `emission_rate` (`float` — particles per second)
- `particle_lifetime` (`float` — seconds)
- `spread_angle` (`float` — cone half-angle in degrees)
- `color` (`RGBA` float)
- `scale` (`float` — particle size)

---

## 2. OLM — Object Lightmap File Format

### 2.1 Overview
`.olm` files store per-object static lightmap textures. Each file is associated with a specific model/object sub-index (e.g., `objXXXXX_YYYYY.olm` where `XXXXX` is the object ID and `YYYYY` is the sub-index). OLM is **not** an ILFF-based format; it uses a flat, sequential binary layout.

### 2.2 IGI 1 vs IGI 2 Differences
- **File Location**: 
  - IGI 1: `missions/<location>/<level>/lightmaps/lightmaps_unpacked/` (also packed in `lightmaps.res`).
  - IGI 2: `missions/<location>/<level>/lightmaps/`.
- **Cascade/Layers Support**:
  - **IGI 1**: Strictly single-layer files (`layer_count = 1`). A deep scan of 2,895 OLM files in the IGI 1 assets reveals **zero** multi-layer files.
  - **IGI 2**: Supports multi-layer lightmap cascades (normally 1 layer, but occasionally 3 layers of decreasing resolutions, e.g. 1024x774, 768x768, 512x512).
- **Format Version**:
  - `version2` is `0.10` in IGI 1, and `0.16` in IGI 2.
- **Layer Descriptor Size**:
  - IGI 1 uses a **16-byte** layer descriptor.
  - IGI 2 uses a **24-byte** layer descriptor.

---

### 2.3 Binary Structure: Main Header (88 bytes)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x00 | float32 | `version1` | Always `0.12` |
| 0x04 | float32 | `version2` | `0.10` in IGI 1; `0.16` in IGI 2 |
| 0x08 | uint32 | `year` | Creation year (e.g., 2000, 2002) |
| 0x0C | uint32 | `month` | Creation month (1-12) |
| 0x10 | uint32 | `day` | Creation day (1-31) |
| 0x14 | uint32 | `hour` | Creation hour (0-23) |
| 0x18 | uint32 | `minute` | Creation minute (0-59) |
| 0x1C | uint32 | `second` | Creation second (0-59) |
| 0x20 | uint32 | `millisecond` | Creation millisecond (0-999) |
| 0x24 | uint32 | `unknown_0` | Always `0` |
| 0x28 | uint32 | `count1` | Flag (always `1` for single-layer files) |
| 0x2C | uint32 | `layer_count` | Number of lightmap layers (always `1` in IGI 1; `1` or `3` in IGI 2) |
| 0x30 | uint32x4 | `reserved` | Reserved block (often contains variable runtime hashes or zero) |
| 0x40 | uint16 | `width` | Grid/block width |
| 0x42 | uint16 | `height` | Grid/block height |
| 0x44 | uint16 | `total_stride` | Perimeter/edge count or stride multiplier |
| 0x46 | uint16 | `format` | Pixel format indicator (always `3` = RGBA) |
| 0x48 | uint32 | `pad` | Always `0` |
| 0x4C | float32 | `uv_scale_u` | UV coordinate scale (U axis) |
| 0x50 | float32 | `uv_scale_v` | UV coordinate scale (V axis) |
| 0x54 | float32 | `zero` | Always `0.0` |

---

### 2.4 Layer Descriptor (IGI 1 vs IGI 2)

#### IGI 1 Layer Descriptor (16 bytes)
Appears directly at offset `88` (0x58) in IGI 1 files:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| +0x00 | uint32 | `flags` | Always `0` in IGI 1 |
| +0x04 | uint32 | `ptr1` | Runtime memory pointer. Increments sequentially by `552` (`0x228`) per lightmap sub-index (e.g. `0x0e01bab8` for index 0, `0x0e01bce0` for index 1, etc.) |
| +0x08 | uint32 | `ptr2` | Always `0` |
| +0x0C | uint16 | `pixel_width` | Actual pixel width of the lightmap layer |
| +0x0E | uint16 | `pixel_height` | Actual pixel height of the lightmap layer |

#### IGI 2 Layer Descriptor (24 bytes)
Appears directly at offset `88` (0x58) in IGI 2 files:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| +0x00 | uint32 | `flags` | Always `0x20000001` in IGI 2 |
| +0x04 | uint32 | `ptr1` | Runtime pointer |
| +0x08 | uint32 | `ptr2` | Runtime pointer |
| +0x0C | uint32 | `val` | Always `21` |
| +0x10 | uint32 | `pad` | Always `0` |
| +0x14 | uint16 | `pixel_width` | Actual pixel width of the lightmap layer |
| +0x16 | uint16 | `pixel_height` | Actual pixel height of the lightmap layer |

---

### 2.5 Extra Block (28 bytes, IGI 2 Multi-Layer Only)
For multi-layer files in IGI 2, a 28-byte block appears between consecutive layer descriptors (does not appear after the last layer descriptor):

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| +0x00 | uint32 | `pad` | Always `0` |
| +0x04 | uint16 | `block_width` | Block dimensions for next level |
| +0x06 | uint16 | `block_height` | Block dimensions for next level |
| +0x08 | uint16 | `block_stride` | Stride value |
| +0x0A | uint16 | `block_format` | Format (always `3`) |
| +0x0C | uint32 | `pad` | Always `0` |
| +0x10 | float32 | `block_uv_u` | UV scale U for this block |
| +0x14 | float32 | `block_uv_v` | UV scale V for this block |
| +0x18 | uint32 | `pad` | Always `0` |

---

### 2.6 Pixel Data
- **Offset**: Starts directly after the last layer descriptor.
  - IGI 1: offset `104` (0x68).
  - IGI 2 (1 layer): offset `112` (0x70).
- **Pixel Format**: RGBA (4 bytes per pixel: Red, Green, Blue, Alpha).
- **Size**: `pixel_width * pixel_height * 4` bytes per layer.
- **Color Correction**: For TGA/PNG exports, pixels must be converted from RGBA to BGRA format.

---

### 2.7 Layout Diagrams

#### IGI 1 Layout (Single-layer, layer_count=1)
```
  [88 bytes] Main Header
  [16 bytes] Layer 0 Descriptor
  [W*H*4 bytes] Layer 0 Pixel Data (RGBA)
```

#### IGI 2 Layout (Single-layer, layer_count=1)
```
  [88 bytes] Main Header
  [24 bytes] Layer 0 Descriptor
  [W*H*4 bytes] Layer 0 Pixel Data (RGBA)
```

#### IGI 2 Layout (Multi-layer, layer_count=3)
```
  [88 bytes] Main Header
  [24 bytes] Layer 0 Descriptor
  [28 bytes] Extra Block 0
  [24 bytes] Layer 1 Descriptor
  [28 bytes] Extra Block 1
  [24 bytes] Layer 2 Descriptor
  [W0*H0*4 bytes] Layer 0 Pixel Data (RGBA)
  [W1*H1*4 bytes] Layer 1 Pixel Data (RGBA)
  [W2*H2*4 bytes] Layer 2 Pixel Data (RGBA)
```

---

### 2.8 IGI 1 Object-Instance Lightmap Binding

In IGI 1 (and 2), lightmaps are **not** bound globally to model assets (i.e. not per-model files only). Instead, the system uses **object-instance lightmap binding**. A lightmap is bound to a placed object instance node (task) in the level tree, meaning two placed instances of the same model file (e.g. `435_01_1`) can resolve to separate baked `.olm` lightmaps based on their distinct world coordinate environments.

#### Observed Binding Chain
- **Building / Object Task**: References the model asset ID (e.g. `435_01_1` for water towers, buildings, etc.).
- **Nested Task**: The same task tree node contains a nested `LightmapInfo` task.
- **Logical Lightmap ID**: The `LightmapInfo` task stores a logical lightmap resource ID (e.g., `obj00000`).
- **File Resolution**: The logical ID resolves to one or more physical `.olm` files (e.g., `obj00000_00000.olm`).

#### Data Model for the Editor
For proper editor state tracking, lightmap information must be modeled at the placed object instance level:
```cpp
struct LightmapBinding {
    std::string logical_id;        // e.g. "obj00000"
    int file_index;                // e.g. 0 (resolves to obj00000_00000.olm)
    int offset_x;                  // Inferred coordinate offset
    int offset_y;                  // Inferred coordinate offset
    float scale_a;                 // Inferred coordinate scale
    float scale_b;                 // Inferred coordinate scale
    float coeff_r;                 // Brightness coefficients R
    float coeff_g;                 // Brightness coefficients G
    float coeff_b;                 // Brightness coefficients B
};

struct StaticBuildingInstance {
    int task_id;
    std::string type;              // e.g. "Building"
    std::string name;              // e.g. "WaterTower"
    std::string model_id;          // e.g. "435_01_1"
    Transform transform;
    std::optional<LightmapBinding> lightmap;
};
```

#### Implications
- Multiple instances of the same model placed in a level require separate, distinct logical lightmap IDs and physical files.
- The level exporter must serialize both the model reference and the lightmap binding nested under the same object task structure.
- Re-baking level lighting must deterministicly preserve or regenerate the object-to-lightmap bindings.

---

### 2.9 Verified C++ OLM Parser Reference

The following C++ snippet demonstrates how to parse and read binary IGI 1 `.olm` files (experimentally validated):

```cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

#pragma pack(push, 1)
struct MainHeader {
    float version1;          // Offset 0x00: Always 0.12
    float version2;          // Offset 0x04: 0.10 in IGI 1, 0.16 in IGI 2
    uint32_t year;           // Offset 0x08
    uint32_t month;          // Offset 0x0C
    uint32_t day;            // Offset 0x10
    uint32_t hour;           // Offset 0x14
    uint32_t minute;         // Offset 0x18
    uint32_t second;         // Offset 0x1C
    uint32_t millisecond;    // Offset 0x20
    uint32_t unknown_0;      // Offset 0x24 (Always 0)
    uint32_t count1;         // Offset 0x28 (Flag, 0 or 1)
    uint32_t layer_count;    // Offset 0x2C (1 in IGI 1)
    uint32_t reserved[4];    // Offset 0x30 (16 bytes)
    uint16_t width;          // Offset 0x40 (Grid width)
    uint16_t height;         // Offset 0x42 (Grid height)
    uint16_t total_stride;   // Offset 0x44
    uint16_t format;         // Offset 0x46 (Always 3 = RGBA)
    uint32_t pad;            // Offset 0x48 (Always 0)
    float uv_scale_u;        // Offset 0x4C
    float uv_scale_v;        // Offset 0x50
    float zero;              // Offset 0x54 (Always 0.0)
};

struct LayerDescriptorIgi1 {
    uint32_t flags;          // Offset 0x00: Always 0
    uint32_t ptr1;           // Offset 0x04: Runtime pointer
    uint32_t ptr2;           // Offset 0x08: Always 0
    uint16_t pixel_width;    // Offset 0x0C
    uint16_t pixel_height;   // Offset 0x0E
};

struct Pixel {
    uint8_t r, g, b, a;
};
#pragma pack(pop)

bool ReadOlmFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    MainHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(MainHeader));
    
    if (header.layer_count != 1) return false; // IGI 1 is always 1

    LayerDescriptorIgi1 desc;
    file.read(reinterpret_cast<char*>(&desc), sizeof(LayerDescriptorIgi1));

    uint32_t numPixels = desc.pixel_width * desc.pixel_height;
    std::vector<Pixel> pixels(numPixels);
    file.read(reinterpret_cast<char*>(pixels.data()), numPixels * sizeof(Pixel));

    return file.gcount() == numPixels * sizeof(Pixel);
}
```

---

## 3. Script & Task Integration (QVM/QSC)

Lightmap compilation variables are defined in scripts (`objects.qvm` compiled from `.qsc`) via parameters parsed by the game engine's virtual machine.

### LightmapInfo Task Parameters
The `LightmapInfo` task defines parameters for calculating and baking lightmaps onto static geometry:
```c
Task_DeclareParameters("LightmapInfo", 
    "Texture scale", "Real32", 
    "Passes", "Int32", 
    "Hemicube resolution", "Int32", 
    "Dirlight resolution", "Int32", 
    "Gamma", "Real32", 
    "Max radiosity per square meter", "Real32", 
    "Indoors ambient light", "RGB", 
    "Filename", "String16"
);
```

**Typical Script Call Example:**
```c
Task_New(-1, "LightmapInfo", "", 1, 1, 550, 1650, 0.8, 280.0, 0.08, 0.08, 0.08, "obj00000");
```
- `Texture scale`: `1.0` (factor for resolution sizing)
- `Passes`: `1` (number of radiosity bouncing passes)
- `Hemicube resolution`: `550` (hemisphere rendering resolution for radiosity)
- `Dirlight resolution`: `1650` (occlusion map size for sunlight)
- `Gamma`: `0.8` (correction factor)
- `Max radiosity per square meter`: `280.0`
- `Indoors ambient light`: `RGB(0.08, 0.08, 0.08)` (sky/ambient fallback)
- `Filename`: `"obj00000"` (associates with lightmaps `obj00000_XXXXX.olm`)

---

## 4. MEF Model Type 3 (MODEL_LIGHTMAP) Integration

When a static mesh supports static baked lightmaps, its model type in the MEF header (`HSEM.modeltype`) is set to `3` (`MODEL_LIGHTMAP`).

### Render Vertices (XTRV Chunk) Stride
- **IGI 1**: Vertex stride is **40 bytes**. It includes two sets of UV coordinates:
  - Offset `+24`: Diffuse Texture UV (`u0`, `v0`, 8 bytes)
  - Offset `+32`: Lightmap Atlas UV (`u1`, `v1`, 8 bytes)
- **IGI 2**: Vertex stride is **32 bytes** (diffuse texture UV only). Lightmap coordinates are mapped differently.

### PMTL (Render Mesh Lightmaps) Chunk
In lightmap models, the `PMTL` chunk stores sizing and resolution parameters. It consists of an array of 8-byte structures:
- `UINT16 width_res`: Width resolution value (`1 - 1024`)
- `UINT16 height_res`: Height resolution value (`1 - 904`)
- `UINT16 zero_0`: Always `0`
- `UINT16 zero_1`: Always `0`

### Collision Mesh (ECFC Chunk) Faces
- In IGI 2, face elements inside the collision chunk (`ECFC`) are 12 bytes and contain a `lightmap` index association at offset `+0x08`.
- In IGI 1, face elements inside `ECFC` are 8 bytes and omit the lightmap index.

---

## 5. Terrain Lightmaps (LMP vs TLM)

Terrain lightmaps are stored differently than object lightmaps:

### IGI 1 Terrain Lightmap (`.lmp`)
- Uses a **grayscale** format (1 byte per pixel).
- Contains a sequential list of terrain lightmap blocks.
- **Block Layout**:
  - `UINT32 size`: Resolution of the square block.
  - `BYTE pixels[size * size]`: Grayscale illumination intensity.
- **Pixel Conversion**: Flipped vertically during rendering. Expanded to RGBA at load time: `R = src`, `G = src`, `B = src`, `A = src ? 255 : 0`.

### IGI 2 Terrain Lightmap (`.tlm`)
- Uses an **RGBA** format (4 bytes per pixel).
- Consists of a flat binary layout:
  - **Header (44 bytes)**: Contains date/time stamps and the baseline grid dimensions (`width`, `height`).
  - **Mip Cascades**: Contains up to 10 mipmap levels, scaling down as `(width // 2^i) * (height // 2^i) * 4` bytes.
