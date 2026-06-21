#include "cmd_iff.h"
#include <iostream>
#include <cstring>
#include <string>
#include <filesystem>
#include <cstdlib>
#include "../source/parsers/iff_parser.h"
#include "../source/parsers/iff_to_bef.h"

namespace fs = std::filesystem;

// Removed python helper functions

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

        if (command == "convert" || command == "batch") {
            // Pure C++ converter
            if (fs::is_regular_file(src)) {
                ConvertIffToBef(src, dst);
            } else if (fs::is_directory(src)) {
                int count = 0;
                for (const auto& entry : fs::directory_iterator(src)) {
                    if (entry.is_regular_file() && 
                       (entry.path().extension() == ".iff" || entry.path().extension() == ".IFF" ||
                        entry.path().extension() == ".bff" || entry.path().extension() == ".BFF")) {
                        ConvertIffToBef(entry.path().string(), dst);
                        count++;
                    }
                }
                std::cout << "\n Converted: " << count << "\n";
            } else {
                std::cerr << "[ERROR] Source path not found: " << src << "\n";
                return 1;
            }
            return 0;
        }

        if (command == "decompile" || command == "create") {
            std::cerr << "[ERROR] " << command << " requires Python which is now fully removed for standalone execution.\n";
            return 1;
        }
    }

    std::cerr << "Unknown iff command: " << command << "\n";
    return 1;
}
