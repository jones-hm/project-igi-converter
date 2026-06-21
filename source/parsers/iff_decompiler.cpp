#include "iff_decompiler.h"
#include "iff_parser.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace igi1conv {

namespace {

namespace fs = std::filesystem;

// Format a float compactly (matches BEF writer style: %.9g).
std::string fmt_float(float v) {
    if (v == 0.0f) return "0.0";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.9g", (double)v);
    return buf;
}

// Pad a clip id to 3 digits (matches "anim_004" naming used everywhere).
std::string pad3(int n) {
    char b[8];
    std::snprintf(b, sizeof(b), "%03d", n);
    return b;
}

// ─── Text writers for the two file types ──────────────────────────────

void write_main_iff_text(const std::string& path, const std::string& baseName,
                         const IffFile& iff)
{
    std::ofstream o(path);
    o << "\n\\\\ Anim Name \n\n";
    o << "[(\"" << baseName << "\")]\n\n";

    o << "\n\\\\ Bone Header \n\n";
    o << "(" << (int)iff.skeleton.object_id << ", " << iff.skeleton.bone_count << ")\n\n";

    o << "\n\\\\ Bone Links \n\n";
    o << "[";
    for (size_t i = 0; i < iff.skeleton.parents.size(); ++i) {
        o << (i ? ", " : "") << iff.skeleton.parents[i];
    }
    o << "]\n\n";

    o << "\n\\\\ Bone Hierarchy \n\n";
    o << "[";
    for (size_t i = 0; i < iff.skeleton.translations.size() / 3; ++i) {
        o << (i ? ", " : "") << "("
          << fmt_float(iff.skeleton.translations[i*3+0]) << ", "
          << fmt_float(iff.skeleton.translations[i*3+1]) << ", "
          << fmt_float(iff.skeleton.translations[i*3+2]) << ")";
    }
    o << "]\n";

    o << "\n\n\\\\ Anims Header \n\n";
    o << "(" << (int)iff.clips.size() << ", " << (int)iff.animation_capacity << ")\n";

    o << "\n\n\\\\ Anims List \n\n";
    o << "[";
    for (size_t i = 0; i < iff.clips.size(); ++i) {
        int id = (int)iff.clips[i].animation_id;
        o << (i ? " " : "") << "(\"anims_" << baseName << "\\anim_"
          << pad3(id) << ".IFF\")";
        if (i + 1 < iff.clips.size()) o << "\n";
    }
    o << "]\n";
}

void write_per_anim_iff_text(const std::string& path, const std::string& baseName,
                             const IffClip& clip)
{
    std::ofstream o(path);
    int animId = (int)clip.animation_id;

    o << "\n\\\\ Anim Category \n\n";
    o << "[(\"anim_" << pad3(animId) << ".IFF\")]\n\n";

    o << "\n\\\\ Anim Header \n\n";
    o << "(" << fmt_float(clip.duration) << ", 0, "
      << (int)clip.flags << ", " << animId << ")\n\n";

    // Trigger events.
    o << "\n\\\\ Bone Trigger Events \n\n";
    o << "(" << (int)clip.events.size() << ",)\n";
    o << "[";
    for (size_t i = 0; i < clip.events.size(); ++i) {
        const auto& e = clip.events[i];
        o << (i ? ",\n " : "") << "(" << e.event_id << ", " << e.time << ", "
          << e.bone_id << ", " << fmt_float(e.pos[0]) << ", "
          << fmt_float(e.pos[1]) << ", " << fmt_float(e.pos[2]) << ")";
    }
    o << "]\n";

    // Translation frames (root only - 1 track).
    o << "\n\\\\ Bone Translation Frames \n\n";
    o << "(" << (int)clip.root_translations.size() << ",)\n";
    o << "[";
    for (size_t i = 0; i < clip.root_translations.size(); ++i) {
        const auto& k = clip.root_translations[i];
        o << (i ? ",\n " : "") << "(" << k.pos[0] << ", " << k.pos[1] << ", "
          << k.pos[2] << ", " << k.time << ", "
          << k.tangent_in[0] << ", " << k.tangent_in[1] << ", " << k.tangent_in[2] << ", "
          << k.tangent_out[0] << ", " << k.tangent_out[1] << ", " << k.tangent_out[2] << ")";
    }
    o << "]\n";

    // Rotation frames - one (count, list) pair per bone.
    o << "\n\\\\ Bone Rotation Frames \n\n";
    o << "(";
    for (size_t i = 0; i < clip.bone_rotations.size(); ++i) {
        o << (i ? ", " : "") << (int)clip.bone_rotations[i].size();
    }
    o << ",)\n[";
    bool firstKey = true;
    for (size_t i = 0; i < clip.bone_rotations.size(); ++i) {
        for (const auto& k : clip.bone_rotations[i]) {
            if (!firstKey) o << ",\n ";
            firstKey = false;
            o << "(00)(" << i << ")("
              << k.rot[0] << ", " << k.rot[1] << ", " << k.rot[2] << ", " << k.rot[3] << ", "
              << k.time << ", "
              << k.rot_b[0] << ", " << k.rot_b[1] << ", " << k.rot_b[2] << ", " << k.rot_b[3] << ", "
              << k.rot_c[0] << ", " << k.rot_c[1] << ", " << k.rot_c[2] << ", " << k.rot_c[3] << ")";
        }
    }
    o << "]\n";
}

} // namespace

bool IFF_Decompile(const std::string& srcIffPath,
                   const std::string& outDir,
                   std::string* err)
{
    if (!fs::exists(srcIffPath)) {
        if (err) *err = "source IFF not found: " + srcIffPath;
        return false;
    }
    fs::create_directories(outDir);

    IffFile iff = IFF_Parse(srcIffPath, /*logger*/nullptr);
    if (!iff.valid) {
        if (err) *err = "failed to parse IFF: " + srcIffPath;
        return false;
    }

    // Use the file stem (e.g. "003" for "003.IFF") as the model id.
    std::string baseName = fs::path(srcIffPath).stem().string();

    // Write the main .IFF text.
    std::string mainPath = (fs::path(outDir) / (baseName + ".IFF")).string();
    write_main_iff_text(mainPath, baseName, iff);

    // Write one .IFF per animation into anims_<id>/.
    fs::path animsDir = fs::path(outDir) / ("anims_" + baseName);
    fs::create_directories(animsDir);
    for (const auto& clip : iff.clips) {
        std::string fname = "anim_" + pad3((int)clip.animation_id) + ".IFF";
        write_per_anim_iff_text((animsDir / fname).string(), baseName, clip);
    }
    return true;
}

} // namespace igi1conv
