// test_qsc_object_parser_realfiles.cpp - integration test that runs
// the parser against the real objects.qsc on disk.  Skipped when the
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

TEST(QscObjectParser, RealLevel1ObjectsQsc) {
    // Real path on the user's IGI1 install: D:/IGI1/missions/location0/level1/objects.qsc.
    // We try a few common layouts so the test runs on different
    // installations and on CI.
    const char* candidates[] = {
        "D:/IGI1/missions/location0/level1/objects.qsc",
        "C:/IGI1/missions/location0/level1/objects.qsc",
        "../missions/location0/level1/objects.qsc",
    };
    std::ifstream f;
    for (const char* p : candidates) {
        f.open(p);
        if (f) break;
    }
    if (!f.is_open()) {
        GTEST_SKIP() << "no real objects.qsc found in known locations";
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
