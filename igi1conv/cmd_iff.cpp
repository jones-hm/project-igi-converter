#include "cmd_iff.h"
#include <iostream>
#include <cstring>
#include <string>
#include <filesystem>
#include <cstdlib>
#include "../source/parsers/iff_parser.h"

namespace fs = std::filesystem;

// Locate the dconv Python package (parent dir that contains dconv/__main__.py)
static std::string findDconvTools() {
    const char* candidates[] = {
        "D:/IGI-Tools/GM_123/IGI_IFF_CONV_GM/IGI_IFF_CONV_GM/tools",
        "tools",
        nullptr
    };
    for (int i = 0; candidates[i]; ++i) {
        std::string p = std::string(candidates[i]) + "/dconv/__main__.py";
        if (fs::exists(p)) return candidates[i];
    }
    return "";
}

static int runPythonDconv(const std::string& toolsDir, const std::string& plugin,
                          const std::string& src, const std::string& dst) {
    if (toolsDir.empty()) {
        std::cerr << "[ERROR] dconv not found. Expected tools/dconv alongside igi1conv.exe\n";
        std::cerr << "        or at D:/IGI-Tools/GM_123/IGI_IFF_CONV_GM/IGI_IFF_CONV_GM/tools\n";
        return 1;
    }
    // cd into tools dir so "python dconv ..." finds the package by relative import
    std::string cmd = "cd /d \"" + toolsDir + "\" && python dconv iff " + plugin
                      + " \"" + src + "\" \"" + dst + "\"";
    std::cout << "[INFO] " << cmd << "\n";
    return (std::system(cmd.c_str()) == 0) ? 0 : 1;
}

int cmd_iff(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: igi1conv iff <command> <args...>\n\n";
        std::cerr << "Commands:\n";
        std::cerr << "  info      <file.iff>              Print IFF structure / dump\n";
        std::cerr << "  test      <file.iff>              Validate parse + skeleton eval\n";
        std::cerr << "  decompile <input_dir> <out_dir>   IGI1 IFF -> .iFF sub-anim files\n";
        std::cerr << "  convert   <input_dir> <out_dir>   IGI1 IFF -> .BEF text scripts\n";
        std::cerr << "  create    <input_dir> <out_dir>   IGI1 .IFF+anims folder -> .iff binary\n";
        std::cerr << "  batch     <anims_dir> <out_dir>   Run decompile + convert in one step\n";
        return 1;
    }

    std::string command = argv[1];

    // ─── info ───────────────────────────────────────────────────────────────
    if (command == "info") {
        if (argc < 3) { std::cerr << "Usage: igi1conv iff info <file.iff>\n"; return 1; }
        std::string file = argv[2];
        std::string outfile = file + ".info.txt";
        for (int i = 3; i < argc; i++)
            if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) outfile = argv[++i];
        IFF_Dump(file, outfile);
        std::cout << "Dumped IFF info to " << outfile << "\n";
        return 0;
    }

    // ─── test ───────────────────────────────────────────────────────────────
    if (command == "test") {
        if (argc < 3) { std::cerr << "Usage: igi1conv iff test <file.iff>\n"; return 1; }
        std::string file = argv[2];
        IffFile iff = IFF_Parse(file);
        if (!iff.valid) { std::cerr << "Invalid IFF file.\n"; return 1; }
        std::cout << "Bones: " << iff.skeleton.bone_count
                  << "  Clips: " << iff.clips.size() << "\n";
        if (!iff.clips.empty()) {
            const auto& clip = iff.clips[0];
            std::cout << "Clip[0] duration=" << clip.duration << " ms\n";
            for (int i = 0; i < iff.skeleton.bone_count; ++i) {
                if (i == 0 && !clip.root_translations.empty())
                    std::cout << "  RootTrans[0].x=" << clip.root_translations[0].pos[0] << "\n";
                if (i < (int)clip.bone_rotations.size() && !clip.bone_rotations[i].empty())
                    std::cout << "  Bone[" << i << "].rot[0]=" << clip.bone_rotations[i][0].rot[0] << "\n";
            }
        }
        std::cout << "Test OK.\n";
        return 0;
    }

    // ─── conversion commands (need src + dst) ───────────────────────────────
    if (command == "decompile" || command == "convert" ||
        command == "create"    || command == "batch") {
        if (argc < 4) {
            std::cerr << "Usage: igi1conv iff " << command << " <src_dir> <dst_dir>\n";
            return 1;
        }
        std::string src = argv[2];
        std::string dst = argv[3];
        std::string tools = findDconvTools();

        if (command == "decompile")
            return runPythonDconv(tools, "IGI1_decompile", src, dst);
        if (command == "convert")
            return runPythonDconv(tools, "IGI1_convert", src, dst);
        if (command == "create")
            return runPythonDconv(tools, "IGI1_create", src, dst);
        if (command == "batch") {
            int r = 0;
            std::cout << "[INFO] === Batch: decompile ===\n";
            r |= runPythonDconv(tools, "IGI1_decompile", src, dst + "/Decompiled");
            std::cout << "[INFO] === Batch: convert ===\n";
            r |= runPythonDconv(tools, "IGI1_convert",   src, dst + "/Converted");
            return r;
        }
    }

    std::cerr << "Unknown iff command: " << command << "\n";
    return 1;
}
