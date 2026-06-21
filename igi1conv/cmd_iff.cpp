#include "cmd_iff.h"
#include <iostream>
#include <cstring>
#include "../source/parsers/iff_parser.h"

int cmd_iff(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: igi1conv iff <command> <file.iff> [options]\n";
        std::cerr << "Commands:\n";
        std::cerr << "  info      Print info about IFF file\n";
        return 1;
    }

    std::string command = argv[1];
    std::string file = argv[2];

    if (command == "info") {
        std::string outfile = file + ".info.txt";
        for (int i = 3; i < argc; i++) {
            if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                outfile = argv[++i];
            }
        }
        IFF_Dump(file, outfile);
        std::cout << "Dumped IFF info to " << outfile << "\n";
    } else if (command == "test") {
        IffFile iff = IFF_Parse(file);
        if (!iff.valid) {
            std::cerr << "Invalid IFF file.\n";
            return 1;
        }
        if (iff.clips.empty()) {
            std::cout << "No clips.\n";
            return 0;
        }
        std::cout << "Evaluating frame 0...\n";
        const auto& clip = iff.clips[0];
        const auto& skel = iff.skeleton;
        for (int i = 0; i < skel.bone_count; ++i) {
            float t = 0.0f;
            if (i == 0 && !clip.root_translations.empty()) {
                auto k1 = clip.root_translations.front();
                std::cout << "Root Trans Key: " << k1.pos[0] << "\n";
            }
            if (i < clip.bone_rotations.size() && !clip.bone_rotations[i].empty()) {
                auto k1 = clip.bone_rotations[i].front();
                std::cout << "Bone " << i << " Rot Key: " << k1.rot[0] << "\n";
            }
        }
        std::cout << "Test completed successfully.\n";
    } else {
        std::cerr << "Unknown iff command: " << command << "\n";
        return 1;
    }

    return 0;
}
