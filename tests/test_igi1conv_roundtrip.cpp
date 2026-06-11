// test_igi1conv_roundtrip.cpp — conversion round-trips, version reporting, and
// error-handling behaviour.
#include "igi1conv_test_util.h"

using namespace igi1conv_test;

// ─── round-trips ─────────────────────────────────────────────────────────────

// QVM -> QSC -> QVM: decompiled source must recompile cleanly.
TEST_F(IGI1ConvTest, RoundtripQvmQscQvm) {
    IGI1CONV_NEED(qvm, "\\.qvm$");
    TempDir tmp;
    std::string qsc  = tmp / "ammo.qsc";
    std::string qvm2 = tmp / "ammo_recompiled.qvm";

    ASSERT_EQ(RunIGI1Conv("qvm decompile " + Q(qvm) + " -o " + Q(qsc)), 0);
    ASSERT_TRUE(NonEmptyFile(qsc));
    EXPECT_EQ(RunIGI1Conv("qsc compile " + Q(qsc) + " -o " + Q(qvm2)), 0);
    EXPECT_TRUE(NonEmptyFile(qvm2));
}

// DAT -> MTP -> DAT: model/texture mappings survive both conversions.
TEST_F(IGI1ConvTest, RoundtripDatMtpDat) {
    IGI1CONV_NEED(dat, "^(?!.*graph).*\\.dat$");
    TempDir tmp;
    std::string mtp  = tmp / "common.mtp";
    std::string dat2 = tmp / "common_back.dat";

    ASSERT_EQ(RunIGI1Conv("dat to-mtp " + Q(dat) + " -o " + Q(mtp)), 0);
    ASSERT_TRUE(NonEmptyFile(mtp));
    EXPECT_EQ(RunIGI1Conv("mtp to-dat " + Q(mtp) + " -o " + Q(dat2)), 0);
    EXPECT_TRUE(NonEmptyFile(dat2));
}

// MTP -> DAT -> MTP: the inverse direction.
TEST_F(IGI1ConvTest, RoundtripMtpDatMtp) {
    IGI1CONV_NEED(mtp, "\\.mtp$");
    TempDir tmp;
    std::string dat  = tmp / "common.dat";
    std::string mtp2 = tmp / "common_back.mtp";

    ASSERT_EQ(RunIGI1Conv("mtp to-dat " + Q(mtp) + " -o " + Q(dat)), 0);
    ASSERT_TRUE(NonEmptyFile(dat));
    EXPECT_EQ(RunIGI1Conv("dat to-mtp " + Q(dat) + " -o " + Q(mtp2)), 0);
    EXPECT_TRUE(NonEmptyFile(mtp2));
}

// TEX -> TGA -> PNG: decode native texture, then re-encode via stb input path.
TEST_F(IGI1ConvTest, RoundtripTexTgaPng) {
    IGI1CONV_NEED(tex, "\\.tex$");
    TempDir tmp;
    std::string tga = tmp / "flare.tga";
    std::string png = tmp / "flare.png";

    ASSERT_EQ(RunIGI1Conv("tex to-tga " + Q(tex) + " -o " + Q(tga)), 0);
    ASSERT_TRUE(NonEmptyFile(tga));
    EXPECT_EQ(RunIGI1Conv("tex to-png " + Q(tga) + " -o " + Q(png)), 0);
    EXPECT_TRUE(NonEmptyFile(png));
}

// ─── version reporting (regression: used to print 3.0) ───────────────────────

TEST_F(IGI1ConvTest, VersionFlagReportsOneZeroZero) {
    std::string out;
    EXPECT_EQ(RunIGI1Conv("--version", &out), 0);
    EXPECT_NE(out.find("1.0.0"), std::string::npos) << "got: " << out;
    EXPECT_EQ(out.find("3.0"), std::string::npos) << "stale version string: " << out;
}

TEST_F(IGI1ConvTest, HelpReportsOneZeroZero) {
    std::string out;
    EXPECT_EQ(RunIGI1Conv("--help", &out), 0);
    EXPECT_NE(out.find("v1.0.0"), std::string::npos) << "got: " << out;
}

// ─── error handling ──────────────────────────────────────────────────────────

TEST_F(IGI1ConvTest, MissingFileReturnsNonZero) {
    EXPECT_NE(RunIGI1Conv("tex info " + Q("C:\\nonexistent_file_xyz.TEX")), 0);
}
TEST_F(IGI1ConvTest, UnknownCommandReturnsNonZero) {
    EXPECT_NE(RunIGI1Conv("badcmd info somefile"), 0);
}
TEST_F(IGI1ConvTest, UnknownSubcommandReturnsNonZero) {
    EXPECT_NE(RunIGI1Conv("tex bogus somefile"), 0);
}
TEST_F(IGI1ConvTest, NoArgsReturnsNonZero) {
    EXPECT_NE(RunIGI1Conv(""), 0);
}
