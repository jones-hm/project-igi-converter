#pragma once
#include "mef_native.h"
#include <string>

namespace MefExporter {

// Export to OBJ + sibling MTL (placeholder texture names mat_N.tga).
bool ExportToObj(const ParsedGeometry &geometry, const std::string &outpath);

// Export to a self-contained bundle folder: outDir/modelStem/{modelStem.obj,
// modelStem.mtl, mat_0.tga … mat_N.tga}.  Resolves real texture names from
// datPath and converts .tex files found in texDir to TGA.
bool ExportToObjBundle(const ParsedGeometry &geometry,
                       const std::string &modelStem,
                       const std::string &outDir,
                       const std::string &datPath,
                       const std::string &texDir,
                       bool skipObj = false);

// Export binary ParsedGeometry to text-based MEF format (parsed by MEFParser).
bool ExportToMefAscii(const ParsedGeometry &geometry,
                      const std::string &outpath);

// Write/read the binary sidecar that preserves all opaque ILFF chunks for
// lossless text→binary round-trips.  The sidecar path is <textPath>.extra.
bool WriteMefSidecar(const std::vector<ParsedGeometry::RawChunk>& chunks,
                     const std::string &sidecarPath);

std::vector<ParsedGeometry::RawChunk> ReadMefSidecar(const std::string &sidecarPath);

} // namespace MefExporter
