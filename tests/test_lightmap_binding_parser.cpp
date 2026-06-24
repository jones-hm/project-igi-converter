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

TEST(LightmapBindingParser, AllBindingsForModelDisambiguatesReusedModel) {
    // The same model id can be placed multiple times across a level, each
    // placement getting its own baked lightmap. allBindingsForModel must
    // return every occurrence, with the root Task_New's id/name attached
    // so a caller can disambiguate.
    std::string qsc =
        "Task_New(1104, \"Building\", \"WaterTower\", 1,2,3,0,0,0, \"435_01_1\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00000\"));\n"
        "Task_New(2200, \"Building\", \"WaterTower2\", 4,5,6,0,0,0, \"435_01_1\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00014\"));\n";

    LightmapBindingSet set = LightmapBindingSet::parse(qsc);
    auto all = set.allBindingsForModel("435_01_1");
    ASSERT_EQ(all.size(), 2u);

    bool found0 = false, found14 = false;
    for (auto* b : all) {
        if (b->logicalId == "obj00000") { EXPECT_EQ(b->taskId, 1104); EXPECT_EQ(b->taskName, "WaterTower"); found0 = true; }
        if (b->logicalId == "obj00014") { EXPECT_EQ(b->taskId, 2200); EXPECT_EQ(b->taskName, "WaterTower2"); found14 = true; }
    }
    EXPECT_TRUE(found0);
    EXPECT_TRUE(found14);
}

TEST(LightmapBindingParser, SiblingBuildingsUnderSharedContainerResolveIndependently) {
    // Regression test for a real bug found against the IGI1 level1 corpus:
    // the decompiled QSC nests multiple sibling Buildings under a shared
    // wrapper (Task_New(-1, "Container", "Buildings", <building1>,
    // <building2>, ...)). Binding must resolve at the NEAREST enclosing
    // LightmapInfo, not flatten every building's model ids into whichever
    // LightmapInfo happens to appear first anywhere inside the shared
    // Container.
    std::string qsc =
        "Task_New(-1, \"Container\", \"Buildings\","
        "    Task_New(1100, \"Building\", \"First\", 0,0,0,0,0,0, \"111_00_0\","
        "        Task_New(-1, \"LightmapInfo\", \"\", 1,1,1,1,1,1,1,1,1, \"obj00000\")),"
        "    Task_New(1101, \"Building\", \"Second\", 0,0,0,0,0,0, \"222_00_0\","
        "        Task_New(-1, \"LightmapInfo\", \"\", 1,1,1,1,1,1,1,1,1, \"obj00001\")));\n";

    LightmapBindingSet set = LightmapBindingSet::parse(qsc);

    auto id111 = set.logicalIdForModel("111_00_0");
    ASSERT_TRUE(id111.has_value());
    EXPECT_EQ(*id111, "obj00000");

    auto id222 = set.logicalIdForModel("222_00_0");
    ASSERT_TRUE(id222.has_value());
    EXPECT_EQ(*id222, "obj00001");

    // The shared Container's own strings ("Container", "Buildings") must
    // not have been bound to either lightmap - the Container itself has
    // no direct LightmapInfo child, so it has nothing to bind to.
    EXPECT_FALSE(set.logicalIdForModel("Container").has_value());
    EXPECT_FALSE(set.logicalIdForModel("Buildings").has_value());
}

TEST(LightmapBindingParser, CapturesPositionAndDisambiguatesByTaskId) {
    std::string qsc =
        "Task_New(1104, \"Building\", \"WaterTower\", 1.0, 2.0, 3.0, 0,0,0, \"435_01_1\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00000\"));\n"
        "Task_New(2200, \"Building\", \"WaterTower2\", 100.0, 200.0, 300.0, 0,0,0, \"435_01_1\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00014\"));\n";

    LightmapBindingSet set = LightmapBindingSet::parse(qsc);

    const LightmapBinding* byTask = set.bindingForModelAndTaskId("435_01_1", 2200);
    ASSERT_NE(byTask, nullptr);
    EXPECT_EQ(byTask->logicalId, "obj00014");
    EXPECT_TRUE(byTask->hasPos);
    EXPECT_DOUBLE_EQ(byTask->posX, 100.0);
    EXPECT_DOUBLE_EQ(byTask->posY, 200.0);
    EXPECT_DOUBLE_EQ(byTask->posZ, 300.0);

    EXPECT_EQ(set.bindingForModelAndTaskId("435_01_1", 9999), nullptr);
    EXPECT_EQ(set.bindingForModelAndTaskId("999_99_9", 1104), nullptr);
}

TEST(LightmapBindingParser, DisambiguatesByNearestPosition) {
    std::string qsc =
        "Task_New(1104, \"Building\", \"WaterTower\", 0.0, 0.0, 0.0, 0,0,0, \"435_01_1\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00000\"));\n"
        "Task_New(2200, \"Building\", \"WaterTower2\", 1000.0, 1000.0, 1000.0, 0,0,0, \"435_01_1\","
        "    Task_New(-1, \"LightmapInfo\", \"\", 1,1,550,1650,0.8,280.0,0.08,0.08,0.08, \"obj00014\"));\n";

    LightmapBindingSet set = LightmapBindingSet::parse(qsc);

    // A query point much closer to (0,0,0) than (1000,1000,1000).
    const LightmapBinding* nearest = set.nearestBindingForModelAndPosition("435_01_1", 5.0, 5.0, 5.0);
    ASSERT_NE(nearest, nullptr);
    EXPECT_EQ(nearest->logicalId, "obj00000");
    EXPECT_EQ(nearest->taskId, 1104);

    const LightmapBinding* nearest2 = set.nearestBindingForModelAndPosition("435_01_1", 999.0, 999.0, 999.0);
    ASSERT_NE(nearest2, nullptr);
    EXPECT_EQ(nearest2->logicalId, "obj00014");

    EXPECT_EQ(set.nearestBindingForModelAndPosition("999_99_9", 0, 0, 0), nullptr);
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
