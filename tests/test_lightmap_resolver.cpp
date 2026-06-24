#include "../source/logger.h"
// res_parser.cpp's error path calls Logger::Log; support.cpp (which defines
// it) isn't linked into igi1conv_tests, so provide a minimal stub here.
void Logger::Log(LogLevel, const std::string&) {}

#include "../source/parsers/lightmap_resolver.h"
#include "../source/parsers/lightmap_resolver.cpp"
#include "../source/parsers/res_parser.cpp"
#include "igi1conv_test_util.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

using namespace igi1conv;
using namespace igi1conv_test;

TEST(LightmapResolver, ResolvesRealCorpusBinding) {
    // ResolveLightmapFiles derives the lightmaps folder as a SIBLING of the
    // directory containing objects.qsc: <level dir>/lightmaps/lightmaps_unpacked.
    // So this test must use the objects.qsc that lives in the SAME level dir
    // as a lightmaps_unpacked/ folder, not just any objects.qsc in the corpus
    // (the corpus can contain unrelated objects.qsc snapshots for other levels).
    std::string lightmapsUnpacked;
    if (std::filesystem::exists(CorpusDir())) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(CorpusDir())) {
            if (entry.is_directory() && entry.path().filename() == "lightmaps_unpacked") {
                lightmapsUnpacked = entry.path().string();
                break;
            }
        }
    }
    if (lightmapsUnpacked.empty()) GTEST_SKIP() << "no lightmaps_unpacked folder in corpus (set IGI_GAME_PATH)";

    std::filesystem::path levelDir =
        std::filesystem::path(lightmapsUnpacked).parent_path().parent_path();
    std::string qscPath = (levelDir / "objects.qsc").string();
    if (!std::filesystem::exists(qscPath)) {
        GTEST_SKIP() << "no decompiled objects.qsc next to " << lightmapsUnpacked
                     << " - run: igi1conv qvm decompile <level>/objects.qvm -o " << qscPath;
    }

    std::ifstream f(qscPath);
    std::string qscText((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    LightmapBindingSet set = LightmapBindingSet::parse(qscText);
    if (set.bindings.empty()) GTEST_SKIP() << "no lightmap bindings found in this objects.qsc";

    auto files = ResolveLightmapFiles(qscPath, set.bindings.front().first);
    EXPECT_FALSE(files.empty());
    for (auto& p : files) EXPECT_TRUE(std::filesystem::exists(p));
}

TEST(LightmapResolver, EmptyForUnknownModel) {
    auto files = ResolveLightmapFiles("Z:\\nonexistent\\objects.qsc", "no_such_model");
    EXPECT_TRUE(files.empty());
}
