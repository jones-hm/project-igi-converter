#include "../source/parsers/mef_native.h"
#include "../source/parsers/mef_native.cpp"
#include "igi1conv_test_util.h"
#include <gtest/gtest.h>

using namespace igi1conv_test;

TEST(MefLightmapUv, LightmapModelHasNonZeroSecondUv) {
    std::string corpusMef = FindCorpusMefOfModelType(3);
    if (corpusMef.empty()) GTEST_SKIP() << "no modelType==3 .mef in corpus (set IGI_GAME_PATH)";

    ParsedGeometry geo = ParseMefFile(corpusMef);
    ASSERT_EQ(geo.modelType, 3u);
    ASSERT_FALSE(geo.vertices.empty());

    bool anyNonZeroUv2 = false;
    for (const auto& v : geo.vertices) {
        if (v.uv2.x != 0.0f || v.uv2.y != 0.0f) { anyNonZeroUv2 = true; break; }
    }
    EXPECT_TRUE(anyNonZeroUv2) << "expected at least one lightmap UV2 coordinate in " << corpusMef;
}

TEST(MefLightmapUv, NonLightmapModelHasZeroSecondUv) {
    std::string corpusMef = FindCorpusMefOfModelType(0);
    if (corpusMef.empty()) GTEST_SKIP() << "no modelType==0 .mef in corpus (set IGI_GAME_PATH)";

    ParsedGeometry geo = ParseMefFile(corpusMef);
    ASSERT_EQ(geo.modelType, 0u);
    for (const auto& v : geo.vertices) {
        EXPECT_FLOAT_EQ(v.uv2.x, 0.0f);
        EXPECT_FLOAT_EQ(v.uv2.y, 0.0f);
    }
}
