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
#include <set>

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

    auto files = ResolveLightmapFiles(qscPath, set.bindings.front().modelId);
    EXPECT_FALSE(files.empty());
    for (auto& p : files) EXPECT_TRUE(std::filesystem::exists(p));
}

// Finds the objects.qsc that lives in the same level dir as a
// lightmaps_unpacked/ folder somewhere under the corpus, the same
// discovery ResolveLightmapFiles itself expects callers to set up
// (objects.qsc must be a sibling of lightmaps/). No path is hard-coded -
// everything is discovered under IGI_GAME_PATH / --game-path.
static std::string FindLevelObjectsQscNextToLightmaps() {
    if (!std::filesystem::exists(CorpusDir())) return "";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(CorpusDir())) {
        if (!entry.is_directory() || entry.path().filename() != "lightmaps_unpacked") continue;
        std::filesystem::path levelDir = entry.path().parent_path().parent_path();
        std::filesystem::path qscPath = levelDir / "objects.qsc";
        if (std::filesystem::exists(qscPath)) return qscPath.string();
    }
    return "";
}

TEST(LightmapResolver, EveryBindingForReusedModelsResolvesToFiles) {
    // Real-world levels can reuse the same model at multiple placements,
    // each with its own baked lightmap - a bare model id is NOT always a
    // unique key. This test scans whatever objects.qsc the corpus
    // provides for any model id with more than one distinct binding, and
    // checks every one of those bindings (not just the first) resolves to
    // real .olm files via ResolveLightmapFilesForLogicalId.
    std::string qscPath = FindLevelObjectsQscNextToLightmaps();
    if (qscPath.empty()) {
        GTEST_SKIP() << "no level dir in the corpus has both lightmaps_unpacked/ and a "
                        "decompiled objects.qsc (set IGI_GAME_PATH; run 'igi1conv qvm "
                        "decompile <level>/objects.qvm -o <level>/objects.qsc' first)";
    }

    std::ifstream f(qscPath);
    std::string qscText((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    LightmapBindingSet set = LightmapBindingSet::parse(qscText);
    if (set.bindings.empty()) GTEST_SKIP() << "no lightmap bindings found in " << qscPath;

    std::set<std::string> modelIds;
    for (const auto& b : set.bindings) modelIds.insert(b.modelId);

    bool checkedAnyReusedModel = false;
    for (const auto& modelId : modelIds) {
        auto all = set.allBindingsForModel(modelId);
        if (all.size() < 2) continue;
        checkedAnyReusedModel = true;
        for (auto* b : all) {
            EXPECT_FALSE(b->logicalId.empty()) << "model " << modelId;
            auto files = ResolveLightmapFilesForLogicalId(qscPath, b->logicalId);
            EXPECT_FALSE(files.empty()) << "no .olm files for logical id " << b->logicalId
                                         << " (model " << modelId << ", task " << b->taskId
                                         << " \"" << b->taskName << "\")";
        }
    }
    if (!checkedAnyReusedModel) GTEST_SKIP() << "no reused model ids in this objects.qsc snapshot";
}

TEST(LightmapResolver, BindingMatchesTheTaskTreeItActuallyCameFrom) {
    // Stronger than EveryBindingForReusedModelsResolvesToFiles: that test
    // only checks a binding's logicalId resolves to SOME files on disk,
    // which can't catch a binding being attributed to the WRONG Task_New
    // tree (every logicalId in the corpus has files, so a mis-rooted scan
    // would still "pass" that check). This test cross-checks against the
    // raw text: every binding for a model must come from a Task_New tree
    // that literally contains both that model id string and that
    // logicalId string within a bounded number of characters of each
    // other (siblings under the same Building, never bleeding into an
    // unrelated neighboring Task_New tree).
    std::string qscPath = FindLevelObjectsQscNextToLightmaps();
    if (qscPath.empty()) GTEST_SKIP() << "no level dir in the corpus has both lightmaps_unpacked/ and objects.qsc";

    std::ifstream f(qscPath);
    std::string qscText((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    LightmapBindingSet set = LightmapBindingSet::parse(qscText);
    if (set.bindings.empty()) GTEST_SKIP() << "no lightmap bindings found in " << qscPath;

    // A real Building -> LightmapInfo tree in this corpus can be large
    // (e.g. "SecurityBuilding" has dozens of furniture EditRigidObj
    // children and spans ~6900 chars) but not unbounded; the mis-rooting
    // bug this test caught attributed bindings across ENTIRE Container
    // wrappers spanning 16,000-127,000 chars. 20000 stays comfortably
    // above any legitimate single-building span while still well below
    // a mis-rooted one. Model id strings (and class-name strings like
    // "EditRigidObj") are commonly reused across many UNRELATED trees
    // throughout the file, so checking only the first occurrence of each
    // string is unreliable - instead check whether ANY occurrence of the
    // model id is within range of the logicalId's occurrence (logicalIds
    // like "obj00015" are unique to one tree).
    constexpr size_t kMaxTreeSpan = 20000;
    auto findAll = [&](const std::string& needle) {
        std::vector<size_t> positions;
        size_t pos = 0;
        while ((pos = qscText.find(needle, pos)) != std::string::npos) {
            positions.push_back(pos);
            ++pos;
        }
        return positions;
    };

    int checked = 0;
    for (const auto& b : set.bindings) {
        size_t lightmapPos = qscText.find("\"" + b.logicalId + "\"");
        if (lightmapPos == std::string::npos) continue; // covered by other tests

        auto modelPositions = findAll("\"" + b.modelId + "\"");
        if (modelPositions.empty()) continue; // covered by other tests
        ++checked;

        size_t bestSpan = std::string::npos;
        for (size_t modelPos : modelPositions) {
            size_t span = lightmapPos > modelPos ? lightmapPos - modelPos : modelPos - lightmapPos;
            if (span < bestSpan) bestSpan = span;
        }
        EXPECT_LT(bestSpan, kMaxTreeSpan)
            << "binding model=" << b.modelId << " logicalId=" << b.logicalId
            << " (task " << b.taskId << " \"" << b.taskName << "\"): the CLOSEST of "
            << modelPositions.size() << " occurrence(s) of this model id is " << bestSpan
            << " chars from the logicalId - likely mis-rooted to an unrelated tree";
    }
    EXPECT_GT(checked, 0) << "no bindings could be cross-checked against the raw text";
}

TEST(LightmapResolver, EmptyForUnknownModel) {
    auto files = ResolveLightmapFiles("Z:\\nonexistent\\objects.qsc", "no_such_model");
    EXPECT_TRUE(files.empty());
}
