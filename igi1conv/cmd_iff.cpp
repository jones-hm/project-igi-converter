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
    } else {
        std::cerr << "Unknown iff command: " << command << "\n";
        return 1;
    }

    return 0;
}
