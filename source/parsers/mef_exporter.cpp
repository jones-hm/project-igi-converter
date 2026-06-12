#include "mef_exporter.h"
#include "dat_parser.h"
#include "tex_parser.h"
#include "../logger.h"
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <set>
#include <vector>

namespace {

// Collect unique renderBlock material slots in first-encounter order.
std::vector<int> CollectMaterialSlots(const ParsedGeometry &geometry) {
  std::set<int> seen;
  std::vector<int> ordered;
  if (!geometry.renderBlocks.empty()) {
    for (const auto &b : geometry.renderBlocks)
      if (seen.insert(b.materialSlot).second)
        ordered.push_back(b.materialSlot);
  } else {
    ordered.push_back(0);
  }
  return ordered;
}

// Write the OBJ body to an already-open stream.
void WriteObjBody(std::ostream &f, const ParsedGeometry &geometry,
                  const std::string &mtlFilename) {
  f << "# IGI Editor MEF -> OBJ Export\n";
  f << "# Vertices: " << geometry.vertices.size() << "\n";
  f << "# Triangles: " << geometry.triangles.size() << "\n";
  f << "# Layout: " << geometry.renderLayout << "\n\n";
  f << "mtllib " << mtlFilename << "\n\n";

  f << std::fixed << std::setprecision(6);

  for (const auto &v : geometry.vertices)
    f << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";

  bool isBoneModel = (geometry.renderLayout.find("type1") != std::string::npos);
  for (const auto &v : geometry.vertices) {
    float v_coord = isBoneModel ? v.uv.y : (1.0f - v.uv.y);
    f << "vt " << v.uv.x << " " << v_coord << "\n";
  }

  f << "\no model_mesh\n";

  if (!geometry.renderBlocks.empty()) {
    int currentMat = -1;
    for (const auto &block : geometry.renderBlocks) {
      if (block.materialSlot != currentMat) {
        currentMat = block.materialSlot;
        f << "\nusemtl mat_" << currentMat << "\n";
      }
      for (size_t i = 0; i < block.triangleCount; ++i) {
        const auto &tri = geometry.triangles[block.triangleStart + i];
        uint32_t a = tri[0] + 1, b = tri[1] + 1, c = tri[2] + 1;
        f << "f " << a << "/" << a << " " << b << "/" << b
          << " " << c << "/" << c << "\n";
      }
    }
  } else {
    f << "\nusemtl mat_0\n";
    for (const auto &tri : geometry.triangles) {
      uint32_t a = tri[0] + 1, b = tri[1] + 1, c = tri[2] + 1;
      f << "f " << a << "/" << a << " " << b << "/" << b
        << " " << c << "/" << c << "\n";
    }
  }
}

// Write the MTL body to an already-open stream.
// texNames[matIdx] is the TGA filename to reference for that slot.
// If texNames is shorter than a slot index, fall back to "mat_N.tga".
void WriteMtlBody(std::ostream &m, const ParsedGeometry &geometry,
                  const std::vector<std::string> &texNames) {
  m << "# IGI Editor MEF -> MTL Export\n";
  for (int matIdx : CollectMaterialSlots(geometry)) {
    m << "\nnewmtl mat_" << matIdx << "\n";
    if (matIdx >= 0 && matIdx < static_cast<int>(texNames.size()) &&
        !texNames[matIdx].empty())
      m << "map_Kd " << texNames[matIdx] << "\n";
    else
      m << "map_Kd mat_" << matIdx << ".tga\n";
  }
}

} // anonymous namespace

namespace MefExporter {

bool ExportToObj(const ParsedGeometry &geometry, const std::string &outpath) {
  // Derive MTL filename (same stem as OBJ)
  auto sepPos = outpath.find_last_of("/\\");
  std::string filename =
      (sepPos != std::string::npos) ? outpath.substr(sepPos + 1) : outpath;
  auto dotPos = filename.rfind('.');
  std::string stem =
      (dotPos != std::string::npos) ? filename.substr(0, dotPos) : filename;
  std::string mtlFilename = stem + ".mtl";
  std::string mtlPath = (sepPos != std::string::npos)
                            ? outpath.substr(0, sepPos + 1) + mtlFilename
                            : mtlFilename;

  std::ofstream f(outpath);
  if (!f.is_open()) {
    Logger::Get().Log(LogLevel::ERR,
                      "[MefExporter] Failed to open OBJ: " + outpath);
    return false;
  }
  WriteObjBody(f, geometry, mtlFilename);
  f.close();

  // Placeholder texture names (pmtlTextures populated if DAT data was injected)
  std::vector<std::string> texNames(geometry.pmtlTextures.begin(),
                                    geometry.pmtlTextures.end());
  std::ofstream m(mtlPath);
  if (m.is_open()) {
    WriteMtlBody(m, geometry, texNames);
    m.close();
  }

  Logger::Get().Log(LogLevel::INFO,
                    "[MefExporter] Exported OBJ to: " + outpath);
  return true;
}

bool ExportToObjBundle(const ParsedGeometry &geometry,
                       const std::string &modelStem,
                       const std::string &outDir,
                       const std::string &datPath,
                       const std::string &texDir,
                       bool skipObj) {
  namespace fs = std::filesystem;

  // 1. Create bundle folder: outDir/modelStem/
  fs::path bundleDir = fs::path(outDir) / modelStem;
  std::error_code ec;
  fs::create_directories(bundleDir, ec);
  if (ec) {
    Logger::Get().Log(LogLevel::ERR,
                      "[MefExporter] Cannot create bundle dir: " +
                          bundleDir.string() + " — " + ec.message());
    return false;
  }

  // 2. Resolve texture list from DAT
  std::vector<std::string> datTextures;
  if (!datPath.empty()) {
    DATFile dat = DAT_Parse(datPath);
    for (const auto &entry : dat.models) {
      if (entry.modelName == modelStem ||
          entry.modelName.find(modelStem) != std::string::npos) {
        datTextures = entry.textures;
        Logger::Get().Log(
            LogLevel::INFO,
            "[MefExporter] DAT: model '" + entry.modelName + "' has " +
                std::to_string(datTextures.size()) + " texture(s)");
        break;
      }
    }
    if (datTextures.empty())
      Logger::Get().Log(LogLevel::WARNING,
                        "[MefExporter] Model '" + modelStem +
                            "' not found in DAT; textures will be placeholders");
  }

  // 3. Convert .tex → mat_N.tga and build the MTL texture name list
  std::vector<std::string> texNames(datTextures.size());
  for (size_t i = 0; i < datTextures.size(); ++i) {
    std::string tgaName = "mat_" + std::to_string(i) + ".tga";
    texNames[i] = tgaName;

    if (!texDir.empty()) {
      fs::path texFile = fs::path(texDir) / (datTextures[i] + ".tex");
      TEXFile tex = TEX_Parse(texFile.string());
      if (tex.valid && !tex.images.empty()) {
        std::string tgaPath = (bundleDir / tgaName).string();
        if (TEX_WriteTGA(tgaPath, tex.images[0]))
          Logger::Get().Log(LogLevel::INFO,
                            "[MefExporter] Wrote " + tgaName + " from " +
                                datTextures[i] + ".tex");
        else
          Logger::Get().Log(LogLevel::WARNING,
                            "[MefExporter] Failed to write " + tgaPath);
      } else {
        Logger::Get().Log(LogLevel::WARNING,
                          "[MefExporter] TEX not found/invalid: " +
                              texFile.string());
      }
    }
  }

  // 4. Write OBJ
  if (!skipObj) {
    std::string objPath = (bundleDir / (modelStem + ".obj")).string();
    std::ofstream f(objPath);
    if (!f.is_open()) {
      Logger::Get().Log(LogLevel::ERR,
                        "[MefExporter] Cannot open OBJ: " + objPath);
      return false;
    }
    WriteObjBody(f, geometry, modelStem + ".mtl");
    f.close();
  }

  // 5. Write MTL
  std::string mtlPath = (bundleDir / (modelStem + ".mtl")).string();
  std::ofstream m(mtlPath);
  if (!m.is_open()) {
    Logger::Get().Log(LogLevel::ERR,
                      "[MefExporter] Cannot open MTL: " + mtlPath);
    return false;
  }
  WriteMtlBody(m, geometry, texNames);
  m.close();

  Logger::Get().Log(LogLevel::INFO,
                    "[MefExporter] Bundle exported to: " + bundleDir.string());
  return true;
}

bool ExportToMefAscii(const ParsedGeometry &geometry,
                      const std::string &outpath) {
  std::ofstream f(outpath);
  if (!f.is_open()) {
    Logger::Get().Log(LogLevel::ERR,
                      "[MefExporter] Failed to open output file: " + outpath);
    return false;
  }

  f << std::setprecision(9);

  f << "NewObject(\"model_mesh\");\n";

  if (geometry.modelType != 0)
    f << "ModelType(" << geometry.modelType << ");\n";

  // Emit one Material() + MaterialShininess() per TAMC entry.
  // Use white diffuse (1,1,1) so textures render at full brightness.
  // TAMC opacity==0 means fully opaque — passing it as diffuse.r would make
  // the mesh render black, which is the "broken" symptom.
  if (!geometry.tamcRecords.empty()) {
    for (size_t i = 0; i < geometry.tamcRecords.size(); ++i) {
      f << "Material(" << i << ", \"mat_" << i
        << "\", 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1);\n";
      f << "MaterialShininess(" << i << ", 0.0);\n";
    }
  } else {
    f << "Material(0, \"mat_0\", 1.0, 1.0, 1.0, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1);\n";
    f << "MaterialShininess(0, 0.0);\n";
  }

  const size_t numVerts = geometry.vertices.size();

  // Check if rawPos is populated (i.e. at least one vertex has a non-zero
  // rawPos)
  bool hasRawPos = false;
  for (const auto &v : geometry.vertices) {
    if (v.rawPos.x != 0.f || v.rawPos.y != 0.f || v.rawPos.z != 0.f) {
      hasRawPos = true;
      break;
    }
  }

  // Vertices (rawPos keeps original game-unit coordinates if available)
  for (size_t i = 0; i < numVerts; ++i) {
    const auto &v = geometry.vertices[i];
    glm::vec3 p = hasRawPos ? v.rawPos : (v.pos * 40.96f);
    f << "Vertex(" << i << ", " << p.x << ", " << p.y << ", " << p.z << ");\n";
  }

  // Normals
  for (size_t i = 0; i < numVerts; ++i) {
    const auto &v = geometry.vertices[i];
    f << "Normal(" << i << ", " << v.normal.x << ", " << v.normal.y << ", "
      << v.normal.z << ");\n";
  }

  // UVs
  for (size_t i = 0; i < numVerts; ++i) {
    const auto &v = geometry.vertices[i];
    f << "UV(" << i << ", " << v.uv.x << ", " << v.uv.y << ");\n";
  }

  // Bone vertex data (Type 1 skeletal models only)
  if (geometry.modelType == 1) {
    for (size_t i = 0; i < numVerts; ++i) {
      const auto &v = geometry.vertices[i];
      f << "BoneVertex(" << i << ", " << v.boneIndex
        << ", " << v.weight << ", " << v.localVertexId << ");\n";
    }
  }

  // Faces: clamp normal indices to valid range
  const int maxNormIdx = numVerts > 0 ? static_cast<int>(numVerts) - 1 : 0;

  auto clampIdx = [&](uint32_t idx) -> int {
    return std::min(static_cast<int>(idx), maxNormIdx);
  };

  if (!geometry.renderBlocks.empty()) {
    for (const auto &block : geometry.renderBlocks) {
      for (size_t i = 0; i < block.triangleCount; ++i) {
        const auto &tri = geometry.triangles[block.triangleStart + i];
        size_t faceIdx = block.triangleStart + i;
        f << "Face(" << faceIdx << ", " << tri[0] << ", " << tri[1] << ", "
          << tri[2] << ", " << clampIdx(tri[0]) << ", " << clampIdx(tri[1])
          << ", " << clampIdx(tri[2]) << ", " << block.materialSlot << ");\n";
      }
    }
  } else {
    for (size_t i = 0; i < geometry.triangles.size(); ++i) {
      const auto &tri = geometry.triangles[i];
      f << "Face(" << i << ", " << tri[0] << ", " << tri[1] << ", " << tri[2]
        << ", " << clampIdx(tri[0]) << ", " << clampIdx(tri[1]) << ", "
        << clampIdx(tri[2]) << ", 0);\n";
    }
  }

  f << std::fixed << std::setprecision(6);
  for (const auto &a : geometry.mefAttachments) {
    std::string n(a.name, strnlen(a.name, 16));
    f << "Attachment(\"" << n << "\", "
      << a.px << ", " << a.py << ", " << a.pz << ", "
      << a.r00 << ", " << a.r01 << ", " << a.r02 << ", "
      << a.r03 << ", " << a.r04 << ", " << a.r05 << ", "
      << a.r06 << ", " << a.r07 << ", " << a.r08 << ", "
      << a.boneId << ");\n";
  }

  for (const auto &p : geometry.portals) {
    f << "PortalBegin(" << p.portalId << ", " << p.materialId << ");\n";
    for (const auto &v : p.verts)
        f << "PortalVertex(" << std::fixed << std::setprecision(6)
          << v.x << ", " << v.y << ", " << v.z << ");\n";
    for (const auto &face : p.faces)
        f << "PortalFace(" << face[0] << ", " << face[1] << ", " << face[2] << ");\n";
    f << "PortalEnd();\n";
  }

  f.close();
  Logger::Get().Log(LogLevel::INFO,
                    "[MefExporter] Successfully exported ASCII MEF to: " +
                        outpath);
  return true;
}

// ---------------------------------------------------------------------------
// Sidecar: preserves all ILFF chunks verbatim for lossless round-trips.
// Format: magic "MEXS" + uint32 version=1 + uint32 numChunks
//         then for each: char fourcc[4] + uint32 dataSize + data (4-byte padded)
// ---------------------------------------------------------------------------

static void SidecarWriteU32(std::ofstream& f, uint32_t v) {
    f.write(reinterpret_cast<const char*>(&v), 4);
}

bool WriteMefSidecar(const std::vector<ParsedGeometry::RawChunk>& chunks,
                     const std::string& sidecarPath) {
    std::ofstream f(sidecarPath, std::ios::binary);
    if (!f.is_open()) {
        Logger::Get().Log(LogLevel::ERR,
            "[MefExporter] Cannot write sidecar: " + sidecarPath);
        return false;
    }
    f.write("MEXS", 4);
    SidecarWriteU32(f, 1u);  // version
    SidecarWriteU32(f, static_cast<uint32_t>(chunks.size()));
    const uint8_t pad4[4] = {0,0,0,0};
    for (const auto& rc : chunks) {
        f.write(rc.fourcc, 4);
        const uint32_t sz = static_cast<uint32_t>(rc.data.size());
        SidecarWriteU32(f, sz);
        if (sz > 0) f.write(reinterpret_cast<const char*>(rc.data.data()), sz);
        const uint32_t rem = (4 - (sz % 4)) % 4;
        if (rem) f.write(reinterpret_cast<const char*>(pad4), rem);
    }
    Logger::Get().Log(LogLevel::INFO,
        "[MefExporter] Sidecar written: " + sidecarPath +
        " (" + std::to_string(chunks.size()) + " chunks)");
    return true;
}

std::vector<ParsedGeometry::RawChunk> ReadMefSidecar(const std::string& sidecarPath) {
    std::vector<ParsedGeometry::RawChunk> result;
    std::ifstream f(sidecarPath, std::ios::binary);
    if (!f.is_open()) return result;

    char magic[4];
    f.read(magic, 4);
    if (std::memcmp(magic, "MEXS", 4) != 0) return result;

    uint32_t version = 0, numChunks = 0;
    f.read(reinterpret_cast<char*>(&version), 4);
    f.read(reinterpret_cast<char*>(&numChunks), 4);

    result.reserve(numChunks);
    for (uint32_t i = 0; i < numChunks; ++i) {
        ParsedGeometry::RawChunk rc;
        f.read(rc.fourcc, 4);
        uint32_t sz = 0;
        f.read(reinterpret_cast<char*>(&sz), 4);
        if (sz > 0) {
            rc.data.resize(sz);
            f.read(reinterpret_cast<char*>(rc.data.data()), sz);
        }
        const uint32_t rem = (4 - (sz % 4)) % 4;
        if (rem) f.seekg(rem, std::ios::cur);
        result.push_back(std::move(rc));
    }
    Logger::Get().Log(LogLevel::INFO,
        "[MefExporter] Sidecar read: " + sidecarPath +
        " (" + std::to_string(result.size()) + " chunks)");
    return result;
}

} // namespace MefExporter
