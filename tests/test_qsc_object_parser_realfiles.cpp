// test_qsc_object_parser_realfiles.cpp - integration test that runs
// the parser against a real objects.qsc on disk.  Skipped when the
// corpus is not present (see igi1conv_test_util.h).

#include "igi1conv_test_util.h"
// Header-only include - the parser implementation is already
// compiled in by test_qsc_object_parser.cpp.
#include "../source/parsers/qsc_object_parser.h"

#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>

using igi1conv::QscObjectSet;
using igi1conv_test::Corpus;
using igi1conv_test::CorpusDir;

TEST(QscObjectParser, RealLevel1ObjectsQsc) {
    // Resolve the corpus root from the IGI_GAME_PATH env var (or the
    // --game-path CLI flag) so the test is portable.  No machine-
    // specific paths are hardcoded; the test skips cleanly if the
    // corpus is absent.
    if (CorpusDir().empty()) {
        GTEST_SKIP() << "IGI_GAME_PATH not set and --game-path not provided";
    }
    // Look under <IGI_GAME_PATH>/missions/location0/level1/objects.qsc
    // first; fall back to a sibling "objects.qsc" at the corpus root
    // so the test also runs on installs that put the mission data
    // somewhere else.
    const std::string candidates[3] = {
        Corpus("missions/location0/level1/objects.qsc"),
        Corpus("level1/objects.qsc"),
        Corpus("objects.qsc"),
    };
    std::ifstream f;
    for (const std::string& p : candidates) {
        f.open(p);
        if (f) break;
    }
    if (!f.is_open()) {
        GTEST_SKIP() << "no real objects.qsc found under " << CorpusDir();
    }
    std::stringstream ss; ss << f.rdbuf();
    std::string text = ss.str();
    f.close();

    std::string err;
    QscObjectSet set = QscObjectSet::parse(text, &err);
    EXPECT_TRUE(err.empty()) << err;
    // The real level1/objects.qsc has at least a handful of
    // HumanSoldier Task_New calls (verified by grep: 31 lines).
    EXPECT_GE(set.entries.size(), 5u) << "parser found only " << set.entries.size() << " entries";
    // At least one model id should be present.
    EXPECT_FALSE(set.modelIds().empty());
    // The level1 mission uses "000_01_1" (Jones) for HumanSoldier -
    // pin this so a regression in the model-id index breaks loud.
    bool hasJones = false;
    for (const auto& id : set.modelIds()) {
        if (id == "000_01_1") { hasJones = true; break; }
    }
    EXPECT_TRUE(hasJones) << "parser missed model 000_01_1 (Jones)";
}
