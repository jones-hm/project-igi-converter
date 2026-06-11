// test_gconv_roundtrip.cpp — conversion round-trips, version reporting, and
// error-handling behaviour.
#include "gconv_test_util.h"

using namespace gconv_test;

// ─── round-trips ─────────────────────────────────────────────────────────────

// QVM -> QSC -> QVM: decompiled source must recompile cleanly.
TEST_F(GConvTest, RoundtripQvmQscQvm) {
    GCONV_NEED(qvm, "AMMO.QVM");
    TempDir tmp;
    std::string qsc  = tmp / "ammo.qsc";
    std::string qvm2 = tmp / "ammo_recompiled.qvm";

    ASSERT_EQ(RunGConv("qvm decompile " + Q(qvm) + " -o " + Q(qsc)), 0);
    ASSERT_TRUE(NonEmptyFile(qsc));
    EXPECT_EQ(RunGConv("qsc compile " + Q(qsc) + " -o " + Q(qvm2)), 0);
    EXPECT_TRUE(NonEmptyFile(qvm2));
}

// DAT -> MTP -> DAT: model/texture mappings survive both conversions.
TEST_F(GConvTest, RoundtripDatMtpDat) {
    GCONV_NEED(dat, "common.dat");
    TempDir tmp;
    std::string mtp  = tmp / "common.mtp";
    std::string dat2 = tmp / "common_back.dat";

    ASSERT_EQ(RunGConv("dat to-mtp " + Q(dat) + " -o " + Q(mtp)), 0);
    ASSERT_TRUE(NonEmptyFile(mtp));
    EXPECT_EQ(RunGConv("mtp to-dat " + Q(mtp) + " -o " + Q(dat2)), 0);
    EXPECT_TRUE(NonEmptyFile(dat2));
}

// MTP -> DAT -> MTP: the inverse direction.
TEST_F(GConvTest, RoundtripMtpDatMtp) {
    GCONV_NEED(mtp, "common.mtp");
    TempDir tmp;
    std::string dat  = tmp / "common.dat";
    std::string mtp2 = tmp / "common_back.mtp";

    ASSERT_EQ(RunGConv("mtp to-dat " + Q(mtp) + " -o " + Q(dat)), 0);
    ASSERT_TRUE(NonEmptyFile(dat));
    EXPECT_EQ(RunGConv("dat to-mtp " + Q(dat) + " -o " + Q(mtp2)), 0);
    EXPECT_TRUE(NonEmptyFile(mtp2));
}

// TEX -> TGA -> PNG: decode native texture, then re-encode via stb input path.
TEST_F(GConvTest, RoundtripTexTgaPng) {
    GCONV_NEED(tex, "FLARE00.TEX");
    TempDir tmp;
    std::string tga = tmp / "flare.tga";
    std::string png = tmp / "flare.png";

    ASSERT_EQ(RunGConv("tex to-tga " + Q(tex) + " -o " + Q(tga)), 0);
    ASSERT_TRUE(NonEmptyFile(tga));
    EXPECT_EQ(RunGConv("tex to-png " + Q(tga) + " -o " + Q(png)), 0);
    EXPECT_TRUE(NonEmptyFile(png));
}

// ─── version reporting (regression: used to print 3.0) ───────────────────────

TEST_F(GConvTest, VersionFlagReportsOneZeroZero) {
    std::string out;
    EXPECT_EQ(RunGConv("--version", &out), 0);
    EXPECT_NE(out.find("1.0.0"), std::string::npos) << "got: " << out;
    EXPECT_EQ(out.find("3.0"), std::string::npos) << "stale version string: " << out;
}

TEST_F(GConvTest, HelpReportsOneZeroZero) {
    std::string out;
    EXPECT_EQ(RunGConv("--help", &out), 0);
    EXPECT_NE(out.find("v1.0.0"), std::string::npos) << "got: " << out;
}

// ─── error handling ──────────────────────────────────────────────────────────

TEST_F(GConvTest, MissingFileReturnsNonZero) {
    EXPECT_NE(RunGConv("tex info " + Q("C:\\nonexistent_file_xyz.TEX")), 0);
}
TEST_F(GConvTest, UnknownCommandReturnsNonZero) {
    EXPECT_NE(RunGConv("badcmd info somefile"), 0);
}
TEST_F(GConvTest, UnknownSubcommandReturnsNonZero) {
    EXPECT_NE(RunGConv("tex bogus somefile"), 0);
}
TEST_F(GConvTest, NoArgsReturnsNonZero) {
    EXPECT_NE(RunGConv(""), 0);
}
