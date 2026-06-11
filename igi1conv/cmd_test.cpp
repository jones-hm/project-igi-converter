#include "pch.h"
#include "cmd_test.h"
#include <filesystem>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

static bool run_test_cmd(const std::string& cmd_line) {
    std::cout << "  Running: " << cmd_line << "\n";
    std::string wrapped = "\"" + cmd_line + "\"";
    int ret = std::system(wrapped.c_str());
    return ret == 0;
}

static bool read_file_content(const fs::path& path, std::string& out) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    out.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return true;
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

int cmd_test(int argc, char** argv)
{
    if (argc < 3 || std::string(argv[1]) != "--game-path") {
        std::cerr << "Usage: igi1conv test --game-path <path_to_game>\n";
        return 1;
    }

    fs::path game_path = argv[2];
    if (!fs::exists(game_path) || !fs::is_directory(game_path)) {
        std::cerr << "Error: Game path does not exist or is not a directory.\n";
        return 1;
    }

    fs::path test_dir = game_path / "igi1conv_test_suite";
    if (!fs::exists(test_dir)) {
        fs::create_directories(test_dir);
    }

    std::map<std::string, std::vector<fs::path>> collected;
    std::vector<std::string> known_exts = {".res", ".mef", ".tex", ".spr", ".pic", ".fnt", ".qvm", ".qsc", ".dat", ".mtp", ".lmp", ".ctr", ".hmp"};

    std::cout << "Scanning game path for test files...\n";
    for (const auto& entry : fs::recursive_directory_iterator(game_path)) {
        if (!entry.is_regular_file()) continue;
        
        fs::path p = entry.path();
        std::string ext = to_lower(p.extension().string());
        
        if (std::find(known_exts.begin(), known_exts.end(), ext) != known_exts.end()) {
            if (collected[ext].size() < 2) {
                if (p.string().find("igi1conv_test_suite") != std::string::npos) continue;

                fs::path target = test_dir / (std::to_string(collected[ext].size()) + "_" + p.filename().string());
                try {
                    fs::copy_file(p, target, fs::copy_options::overwrite_existing);
                    collected[ext].push_back(target);
                } catch (...) {}
            }
        }
    }

    char* pgmptr = nullptr;
    _get_pgmptr(&pgmptr);
    std::string exe_path = pgmptr ? pgmptr : "igi1conv";
    exe_path = "\"" + exe_path + "\""; // quote executable path

    int success_count = 0;
    int fail_count = 0;

    std::cout << "\nStarting tests in " << test_dir.string() << "\n";
    for (const auto& pair : collected) {
        const std::string& ext = pair.first;
        const auto& files = pair.second;
        for (const auto& f : files) {
            std::cout << "\nTesting " << ext << " file: " << f.filename().string() << "\n";
            std::string path_str = f.string();
            
            if (ext == ".tex" || ext == ".spr" || ext == ".pic") {
                if (run_test_cmd(exe_path + " tex info \"" + path_str + "\"")) {
                    std::cout << "  [OK] Parsed successfully.\n";
                    success_count++;
                } else {
                    std::cout << "  [FAIL] Parse failed.\n";
                    fail_count++;
                }
            } else if (ext == ".mef") {
                if (run_test_cmd(exe_path + " mef info \"" + path_str + "\"")) {
                    std::cout << "  [OK] Parsed successfully.\n";
                    success_count++;
                } else {
                    std::cout << "  [FAIL] Parse failed.\n";
                    fail_count++;
                }
            } else if (ext == ".fnt") {
                if (run_test_cmd(exe_path + " fnt info \"" + path_str + "\"")) {
                    std::cout << "  [OK] Parsed successfully.\n";
                    success_count++;
                } else {
                    std::cout << "  [FAIL] Parse failed.\n";
                    fail_count++;
                }
            } else if (ext == ".qvm") {
                std::string qsc1 = path_str + "_1.qsc";
                std::string qvm2 = path_str + "_2.qvm";
                std::string qsc2 = path_str + "_3.qsc";
                bool ok = run_test_cmd(exe_path + " qvm decompile \"" + path_str + "\" -o \"" + qsc1 + "\"") &&
                          run_test_cmd(exe_path + " qsc compile \"" + qsc1 + "\" -o \"" + qvm2 + "\"") &&
                          run_test_cmd(exe_path + " qvm decompile \"" + qvm2 + "\" -o \"" + qsc2 + "\"");
                if (ok) {
                    std::string s1, s2;
                    if (read_file_content(qsc1, s1) && read_file_content(qsc2, s2) && s1 == s2) {
                        std::cout << "  [OK] Semantic roundtrip matches.\n";
                        success_count++;
                    } else {
                        std::cout << "  [FAIL] Semantic roundtrip differs.\n";
                        fail_count++;
                    }
                } else {
                    std::cout << "  [FAIL] Conversion commands failed.\n";
                    fail_count++;
                }
            } else if (ext == ".mtp") {
                std::string json1 = path_str + "_1.json";
                std::string dat2 = path_str + "_2.dat";
                std::string mtp3 = path_str + "_3.mtp";
                std::string json4 = path_str + "_4.json";
                bool ok = run_test_cmd(exe_path + " mtp dump \"" + path_str + "\" -o \"" + json1 + "\"") &&
                          run_test_cmd(exe_path + " mtp to-dat \"" + path_str + "\" -o \"" + dat2 + "\"") &&
                          run_test_cmd(exe_path + " dat to-mtp \"" + dat2 + "\" -o \"" + mtp3 + "\"") &&
                          run_test_cmd(exe_path + " mtp dump \"" + mtp3 + "\" -o \"" + json4 + "\"");
                if (ok) {
                    std::string s1, s2;
                    if (read_file_content(json1, s1) && read_file_content(json4, s2) && s1 == s2) {
                        std::cout << "  [OK] Semantic roundtrip matches.\n";
                        success_count++;
                    } else {
                        std::cout << "  [FAIL] Semantic roundtrip differs.\n";
                        fail_count++;
                    }
                } else {
                    std::cout << "  [FAIL] Conversion commands failed.\n";
                    fail_count++;
                }
            } else if (ext == ".dat") {
                std::string fname_lower = to_lower(f.filename().string());
                if (fname_lower.find("graph") != std::string::npos) {
                    if (run_test_cmd(exe_path + " graph info \"" + path_str + "\"")) {
                        std::cout << "  [OK] Graph parsed successfully.\n";
                        success_count++;
                    } else {
                        std::cout << "  [FAIL] Graph parse failed.\n";
                        fail_count++;
                    }
                } else {
                    std::string json1 = path_str + "_1.json";
                    std::string mtp2 = path_str + "_2.mtp";
                    std::string dat3 = path_str + "_3.dat";
                    std::string json4 = path_str + "_4.json";
                    bool ok = run_test_cmd(exe_path + " dat export \"" + path_str + "\" -o \"" + json1 + "\"") &&
                              run_test_cmd(exe_path + " dat to-mtp \"" + path_str + "\" -o \"" + mtp2 + "\"") &&
                              run_test_cmd(exe_path + " mtp to-dat \"" + mtp2 + "\" -o \"" + dat3 + "\"") &&
                              run_test_cmd(exe_path + " dat export \"" + dat3 + "\" -o \"" + json4 + "\"");
                    if (ok) {
                        std::string s1, s2;
                        if (read_file_content(json1, s1) && read_file_content(json4, s2) && s1 == s2) {
                            std::cout << "  [OK] Semantic roundtrip matches.\n";
                            success_count++;
                        } else {
                            std::cout << "  [FAIL] Semantic roundtrip differs.\n";
                            fail_count++;
                        }
                    } else {
                        std::cout << "  [FAIL] Conversion commands failed.\n";
                        fail_count++;
                    }
                }
            } else if (ext == ".res") {
                std::string list1 = path_str + "_1.txt";
                std::string dir2 = path_str + "_2_dir";
                std::string res3 = path_str + "_3.res";
                std::string list4 = path_str + "_4.txt";
                bool ok = run_test_cmd(exe_path + " res list \"" + path_str + "\" > \"" + list1 + "\"") &&
                          run_test_cmd(exe_path + " res unpack \"" + path_str + "\" \"" + dir2 + "\"") &&
                          run_test_cmd(exe_path + " res pack \"" + dir2 + "\" \"" + res3 + "\"") &&
                          run_test_cmd(exe_path + " res list \"" + res3 + "\" > \"" + list4 + "\"");
                if (ok) {
                    std::cout << "  [OK] Archive repacked successfully.\n";
                    success_count++;
                } else {
                    std::cout << "  [FAIL] Archive repack failed.\n";
                    fail_count++;
                }
            } else {
                std::cout << "  [SKIP] No advanced test defined.\n";
            }
        }
    }

    std::cout << "\nTest Suite Complete. Passed: " << success_count << ", Failed: " << fail_count << "\n";
    return fail_count == 0 ? 0 : 1;
}
