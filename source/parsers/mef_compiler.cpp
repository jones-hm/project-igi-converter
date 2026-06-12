#include "mef_compiler.h"
#include "mef_parser.h"
#include "../logger.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

namespace MefCompiler {

namespace {

// ---------------------------------------------------------------------------
// Binary write helpers (little-endian)
// ---------------------------------------------------------------------------

static void WriteU8(std::vector<uint8_t>& out, uint8_t v) {
    out.push_back(v);
}

static void WriteU16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

static void WriteI16(std::vector<uint8_t>& out, int16_t v) {
    WriteU16(out, static_cast<uint16_t>(v));
}

static void WriteU32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

static void WriteFloat(std::vector<uint8_t>& out, float f) {
    uint32_t bits = 0;
    std::memcpy(&bits, &f, sizeof(float));
    WriteU32(out, bits);
}

static void WriteZeros(std::vector<uint8_t>& out, size_t count) {
    out.insert(out.end(), count, 0u);
}

static void WriteTag(std::vector<uint8_t>& out, const char tag[4]) {
    out.insert(out.end(), tag, tag + 4);
}

static void PadTo4(std::vector<uint8_t>& out) {
    while (out.size() % 4 != 0)
        out.push_back(0u);
}

// ---------------------------------------------------------------------------
// Chunk header: FourCC | dataSize | alignment=4 | skip
//   skip = 0 means this is the last chunk (no following chunk).
//   skip != 0: offset from chunk start to next chunk start.
//   For non-last chunks: skip = 16 (header) + aligned(dataSize).
// ---------------------------------------------------------------------------
static void WriteChunkHeader(std::vector<uint8_t>& out,
                              const char fourcc[4],
                              uint32_t dataSize,
                              uint32_t skip) {
    WriteTag(out, fourcc);
    WriteU32(out, dataSize);
    WriteU32(out, 4); // alignment
    WriteU32(out, skip);
}

// ---------------------------------------------------------------------------
// Per-vertex combined data for XTRV Type 0 (32 bytes each)
// ---------------------------------------------------------------------------
struct BinaryVertex {
    float px{0}, py{0}, pz{0};
    float nx{0}, ny{0}, nz{1};
    float u{0},  v{0};
};

// ---------------------------------------------------------------------------
// Build vertex buffer, deduplicating on (vertex_index, normal_index) pairs
// so that the binary vertex has exactly one normal.
// ---------------------------------------------------------------------------
static std::vector<BinaryVertex> BuildVertices(
    const MEFObject& obj,
    const std::vector<MEFFace>& faces,
    std::vector<uint32_t>& outRemappedA,
    std::vector<uint32_t>& outRemappedB,
    std::vector<uint32_t>& outRemappedC)
{
    std::vector<BinaryVertex> verts;
    std::map<std::pair<int,int>, uint32_t> cache; // (vi, ni) -> output idx

    outRemappedA.reserve(faces.size());
    outRemappedB.reserve(faces.size());
    outRemappedC.reserve(faces.size());

    auto getVert = [&](int vi, int ni) -> uint32_t {
        auto key = std::make_pair(vi, ni);
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;

        BinaryVertex bv;
        if (vi >= 0 && vi < static_cast<int>(obj.vertices.size())) {
            bv.px = obj.vertices[vi][0];
            bv.py = obj.vertices[vi][1];
            bv.pz = obj.vertices[vi][2];
        }
        if (ni >= 0 && ni < static_cast<int>(obj.normals.size())) {
            bv.nx = obj.normals[ni][0];
            bv.ny = obj.normals[ni][1];
            bv.nz = obj.normals[ni][2];
        }
        // UV: use vi as UV index (matches ExportToMefAscii output)
        if (vi >= 0 && vi < static_cast<int>(obj.uvs.size())) {
            const auto& uv = obj.uvs[vi];
            if (uv.size() >= 2) { bv.u = uv[0]; bv.v = uv[1]; }
        }

        uint32_t idx = static_cast<uint32_t>(verts.size());
        verts.push_back(bv);
        cache[key] = idx;
        return idx;
    };

    for (const auto& face : faces) {
        outRemappedA.push_back(getVert(face.v0, face.n0));
        outRemappedB.push_back(getVert(face.v1, face.n1));
        outRemappedC.push_back(getVert(face.v2, face.n2));
    }

    return verts;
}

// ---------------------------------------------------------------------------
// Compute bounding sphere radius from origin
// ---------------------------------------------------------------------------
static float ComputeRadius(const std::vector<BinaryVertex>& verts) {
    float r2 = 0.0f;
    for (const auto& v : verts) {
        float d2 = v.px * v.px + v.py * v.py + v.pz * v.pz;
        if (d2 > r2) r2 = d2;
    }
    return std::sqrt(r2);
}

// ---------------------------------------------------------------------------
// Build HSEM chunk data (156 bytes, marks file as IGI1)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> BuildHsem(uint32_t numFaces,
                                       uint32_t numVerts,
                                       uint32_t numBlocks,
                                       uint16_t numAttachments,
                                       float    radius)
{
    std::vector<uint8_t> d;
    d.reserve(156);

    WriteFloat(d, 0.0f);          // _V (4)
    WriteZeros(d, 28);            // Date[7] (28)
    WriteU32(d, 0u);              // model_type = 0 rigid (4) [offset 32]
    WriteZeros(d, 12);            // reserved_0[3] (12)
    WriteZeros(d, 48);            // vectors[12] (48)
    WriteU32(d, numFaces);        // num_r_faces (4)
    WriteU32(d, numVerts);        // num_r_verts (4)
    WriteU32(d, numBlocks);       // num_r_buffer (4)
    WriteZeros(d, 12);            // sum_c_faces/verts/buffer (12)
    WriteFloat(d, radius);        // model_radius (4)
    WriteU16(d, 0u);              // num_mverts (2)
    WriteU16(d, numAttachments);  // num_attachments (2)
    WriteU16(d, 0u);              // num_pverts (2)
    WriteU16(d, 0u);              // num_pfaces (2)
    WriteU16(d, 0u);              // num_portals (2)
    WriteZeros(d, 22);            // rs[22] (22)

    return d; // 4+28+4+12+48+4+4+4+12+4+2+2+2+2+2+22 = 156
}

// ---------------------------------------------------------------------------
// Build D3DR chunk data (16 bytes, Type 0)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> BuildD3dr(uint32_t numFaces,
                                       uint32_t numMeshes,
                                       uint32_t numVerts)
{
    std::vector<uint8_t> d;
    d.reserve(16);
    WriteU32(d, 0u);        // unknown
    WriteU32(d, numFaces);
    WriteU32(d, numMeshes);
    WriteU32(d, numVerts);
    return d;
}

// ---------------------------------------------------------------------------
// Build XTRV chunk data (32 bytes per vertex, Type 0)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> BuildXtrv(const std::vector<BinaryVertex>& verts) {
    std::vector<uint8_t> d;
    d.reserve(verts.size() * 32);
    for (const auto& v : verts) {
        WriteFloat(d, v.px);
        WriteFloat(d, v.py);
        WriteFloat(d, v.pz);
        WriteFloat(d, v.nx);
        WriteFloat(d, v.ny);
        WriteFloat(d, v.nz);
        WriteFloat(d, v.u);
        WriteFloat(d, v.v);
    }
    return d;
}

// ---------------------------------------------------------------------------
// Build DNER chunk data (packed, Type 0)
// Groups faces by material and emits one render block per material.
// Block layout (28-byte fixed header + uint16 indices):
//   bytes  0-11: px,py,pz (float, zeroed)
//   bytes 12-13: indexCount (uint16) = numFacesInBlock * 3
//   bytes 14-15: nextoffs (int16): 0 for non-last, -1 for last
//   bytes 16-17: td / materialSlot (int16)
//   bytes 18-19: offVerts (uint16): min vertex index in block
//   bytes 20-21: numVerts (uint16): max - min + 1
//   bytes 22-23: opacity (uint16): 0 = opaque
//   bytes 24-27: eflame, mshine, scolor, opacitd (uint8 x4)
//   bytes 28+:   uint16 indices[indexCount] (local, relative to offVerts)
// ---------------------------------------------------------------------------
static std::vector<uint8_t> BuildDner(
    const std::vector<MEFFace>& faces,
    const std::vector<uint32_t>& remA,
    const std::vector<uint32_t>& remB,
    const std::vector<uint32_t>& remC,
    int numMaterials)
{
    // Collect sorted unique material indices
    std::vector<int> matList;
    {
        std::map<int, bool> seen;
        for (const auto& f : faces) seen[f.material_index] = true;
        for (auto& [k, _] : seen) matList.push_back(k);
    }
    if (matList.empty()) matList.push_back(0);

    std::vector<uint8_t> d;

    for (size_t bi = 0; bi < matList.size(); ++bi) {
        const int mat = matList[bi];
        const bool isLast = (bi == matList.size() - 1);

        // Gather face indices for this material
        std::vector<size_t> faceIdxs;
        for (size_t fi = 0; fi < faces.size(); ++fi) {
            if (faces[fi].material_index == mat) faceIdxs.push_back(fi);
        }
        if (faceIdxs.empty()) continue;

        // Find vertex range
        uint32_t minV = UINT32_MAX, maxV = 0;
        for (size_t fi : faceIdxs) {
            minV = std::min(minV, remA[fi]);
            minV = std::min(minV, remB[fi]);
            minV = std::min(minV, remC[fi]);
            maxV = std::max(maxV, remA[fi]);
            maxV = std::max(maxV, remB[fi]);
            maxV = std::max(maxV, remC[fi]);
        }

        const uint16_t offVerts  = static_cast<uint16_t>(minV);
        const uint16_t numVerts  = static_cast<uint16_t>(maxV - minV + 1);
        const uint16_t indexCount = static_cast<uint16_t>(faceIdxs.size() * 3);

        // Write 28-byte block header
        WriteFloat(d, 0.0f); WriteFloat(d, 0.0f); WriteFloat(d, 0.0f); // px,py,pz
        WriteU16(d, indexCount);
        WriteI16(d, isLast ? int16_t(-1) : int16_t(0));  // nextoffs
        WriteI16(d, static_cast<int16_t>(mat));           // td = material slot
        WriteU16(d, offVerts);
        WriteU16(d, numVerts);
        WriteU16(d, 0u);  // opacity (0 = opaque)
        WriteU8 (d, 0u);  // eflame
        WriteU8 (d, 0u);  // mshine
        WriteU8 (d, 0u);  // scolor
        WriteU8 (d, 0u);  // opacitd

        // Write local face indices
        for (size_t fi : faceIdxs) {
            WriteU16(d, static_cast<uint16_t>(remA[fi] - minV));
            WriteU16(d, static_cast<uint16_t>(remB[fi] - minV));
            WriteU16(d, static_cast<uint16_t>(remC[fi] - minV));
        }
    }

    // Pad to 4-byte boundary
    while (d.size() % 4 != 0) d.push_back(0u);

    return d;
}

// ---------------------------------------------------------------------------
// Build TAMC chunk data (12 bytes per material, IGI1 format)
// One record per material slot 0..numMaterials-1
// ---------------------------------------------------------------------------
static std::vector<uint8_t> BuildTamc(const std::vector<MEFMaterial>& mats,
                                       int numSlots)
{
    std::vector<uint8_t> d;
    d.reserve(static_cast<size_t>(numSlots) * 12);
    for (int i = 0; i < numSlots; ++i) {
        // Find material with this index slot
        float opacity = 1.0f;
        if (i < static_cast<int>(mats.size())) {
            opacity = mats[i].has_collision ? 0.0f : 1.0f;
        }
        WriteFloat(d, opacity);   // opacity (4)
        WriteU16(d, 0u);          // portal (2)
        WriteI16(d, -1);          // diffuse (-1 = none) (2)
        WriteI16(d, static_cast<int16_t>(i)); // mat_id (2)
        WriteU16(d, 0u);          // unknown (2)
    }
    return d;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool Compile(const std::string& textMefPath, const std::string& outBinaryPath) {
    // 1. Parse ASCII MEF
    MEFParser parser;
    std::vector<MEFObject> objects;
    try {
        objects = parser.parse_file(textMefPath);
    } catch (const std::exception& e) {
        Logger::Get().Log(LogLevel::ERR,
            std::string("[MefCompiler] Parse error: ") + e.what());
        return false;
    }

    if (objects.empty()) {
        Logger::Get().Log(LogLevel::ERR,
            "[MefCompiler] No objects found in: " + textMefPath);
        return false;
    }

    const MEFObject& obj = objects[0];

    if (obj.vertices.empty() || obj.faces.empty()) {
        Logger::Get().Log(LogLevel::ERR,
            "[MefCompiler] Object has no vertices or faces: " + textMefPath);
        return false;
    }

    // 2. Build combined vertex buffer
    std::vector<uint32_t> remA, remB, remC;
    std::vector<BinaryVertex> verts = BuildVertices(obj, obj.faces, remA, remB, remC);

    // 3. Compute stats
    const uint32_t numVerts  = static_cast<uint32_t>(verts.size());
    const uint32_t numFaces  = static_cast<uint32_t>(obj.faces.size());
    const float    radius    = ComputeRadius(verts);

    // Count material render blocks
    std::map<int, bool> matSeen;
    for (const auto& f : obj.faces) matSeen[f.material_index] = true;
    const uint32_t numBlocks = static_cast<uint32_t>(matSeen.empty() ? 1 : matSeen.size());
    const int numMatSlots    = static_cast<int>(obj.materials.empty()
                                ? (matSeen.empty() ? 1 : matSeen.rbegin()->first + 1)
                                : obj.materials.size());

    // 4. Build chunk data
    auto hsemData = BuildHsem(numFaces, numVerts, numBlocks,
                               0 /* no attachments from ASCII */, radius);
    auto d3drData = BuildD3dr(numFaces, numBlocks, numVerts);
    auto xtrvData = BuildXtrv(verts);
    auto dnerData = BuildDner(obj.faces, remA, remB, remC, numMatSlots);
    auto tamcData = BuildTamc(obj.materials, numMatSlots);

    // 5. Compute chunk layout & skip values
    // Chunk order: HSEM, D3DR, XTRV, DNER, TAMC (last)
    // skip(chunk) = 16 + dataSize for non-last; 0 for last
    const bool hasTamc = !tamcData.empty();

    struct ChunkDesc { const char* tag; std::vector<uint8_t>* data; };
    std::vector<ChunkDesc> chunks;
    chunks.push_back({"HSEM", &hsemData});
    chunks.push_back({"D3DR", &d3drData});
    chunks.push_back({"XTRV", &xtrvData});
    chunks.push_back({"DNER", &dnerData});
    if (hasTamc)
        chunks.push_back({"TAMC", &tamcData});

    // Ensure all data sizes are 4-byte aligned
    for (auto& c : chunks) {
        while (c.data->size() % 4 != 0) c.data->push_back(0u);
    }

    // Compute total file size
    // 20 (ILFF header) + sum of (16 + dataSize) for each chunk
    uint32_t fileSize = 20;
    for (auto& c : chunks) fileSize += 16 + static_cast<uint32_t>(c.data->size());

    // 6. Assemble binary
    std::vector<uint8_t> out;
    out.reserve(fileSize);

    // ILFF outer header (20 bytes)
    WriteTag(out, "ILFF");
    WriteU32(out, fileSize);
    WriteU32(out, 4u);   // alignment
    WriteU32(out, 0u);   // skip = 0
    WriteTag(out, "OCEM"); // format ID

    // Chunk headers + data
    for (size_t ci = 0; ci < chunks.size(); ++ci) {
        const bool isLast = (ci == chunks.size() - 1);
        const uint32_t dataSize = static_cast<uint32_t>(chunks[ci].data->size());
        const uint32_t skip = isLast ? 0u : (16u + dataSize);
        WriteChunkHeader(out, chunks[ci].tag, dataSize, skip);
        out.insert(out.end(), chunks[ci].data->begin(), chunks[ci].data->end());
    }

    if (out.size() != fileSize) {
        Logger::Get().Log(LogLevel::ERR,
            "[MefCompiler] Internal size mismatch: expected " +
            std::to_string(fileSize) + " got " + std::to_string(out.size()));
        return false;
    }

    // 7. Write to file
    std::ofstream f(outBinaryPath, std::ios::binary);
    if (!f.is_open()) {
        Logger::Get().Log(LogLevel::ERR,
            "[MefCompiler] Cannot open output file: " + outBinaryPath);
        return false;
    }
    f.write(reinterpret_cast<const char*>(out.data()),
            static_cast<std::streamsize>(out.size()));
    f.close();

    Logger::Get().Log(LogLevel::INFO,
        "[MefCompiler] Compiled binary MEF to: " + outBinaryPath +
        " (" + std::to_string(out.size()) + " bytes, " +
        std::to_string(numVerts) + " verts, " +
        std::to_string(numFaces) + " faces, " +
        std::to_string(numBlocks) + " render blocks)");
    return true;
}

} // namespace MefCompiler
