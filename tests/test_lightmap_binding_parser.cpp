#include "../source/parsers/qsc_object_parser.h"
#include <gtest/gtest.h>

using namespace igi1conv;

TEST(LightmapBindingParser, ResolvesModelInsideNestedTaskTree) {
    std::string qsc =
        "Task_New(1104, \"Building\", \"WaterTower\", 24658918.0, -55966376.0, 174413136.0, 0, 0, 3.1415929794311523, \"435_01_1\", \n"
        "    Task_New(-1, \"Static\", \"\", \n"
        "        Task_New(-1, \"EditRigidObj\", \"\", 24631140.0, -56011972.0, 174510640.0, 0, 0, 0, \"490_02_1\", 1, 1, 1, 0, 0, 0)), \n"
        "    Task_New(-1, \"LightmapInfo\", \"\", 1, 1, 550, 1650, 0.8, 280.0, 0.08, 0.08, 0.08, \"obj00000\")); \n";

    LightmapBindingSet set = LightmapBindingSet::parse(qsc);

    auto idOuter = set.logicalIdForModel("435_01_1");
    ASSERT_TRUE(idOuter.has_value());
    EXPECT_EQ(*idOuter, "obj00000");

    // The nested EditRigidObj model id lives in the SAME Task_New tree as the
    // LightmapInfo sibling, so it must resolve to the same logical id.
    auto idNested = set.logicalIdForModel("490_02_1");
    ASSERT_TRUE(idNested.has_value());
    EXPECT_EQ(*idNested, "obj00000");
}

TEST(LightmapBindingParser, NoMatchForUnboundModel) {
    std::string qsc =
        "Task_New(1, \"Building\", \"Shed\", 0,0,0,0,0,0, \"999_00_0\");\n";
    LightmapBindingSet set = LightmapBindingSet::parse(qsc);
    EXPECT_FALSE(set.logicalIdForModel("999_00_0").has_value());
}

TEST(LightmapBindingParser, NoMatchWhenSiblingTreeHasNoLightmap) {
    // Two separate top-level trees: only the second has a LightmapInfo.
    // The first tree's model id must NOT pick up the second tree's id.
    std::string qsc =
        "Task_New(1, \"Building\", \"NoLight\", 0,0,0,0,0,0, \"111_00_0\");\n"
        "Task_New(2, \"Building\", \"HasLight\", 0,0,0,0,0,0, \"222_00_0\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00099\"));\n";
    LightmapBindingSet set = LightmapBindingSet::parse(qsc);
    EXPECT_FALSE(set.logicalIdForModel("111_00_0").has_value());
    auto id = set.logicalIdForModel("222_00_0");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, "obj00099");
}
