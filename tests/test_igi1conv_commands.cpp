// test_igi1conv_commands.cpp — one or more tests for every igi1conv command and
// every subcommand, driven against the real game-file corpus.
#include "igi1conv_test_util.h"

using namespace igi1conv_test;

// ─── tex ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, TexInfoTex) {
    IGI1CONV_NEED(f, "FLARE00.TEX");
    EXPECT_EQ(RunIGI1Conv("tex info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TexInfoSpr) {
    IGI1CONV_NEED(f, "arrow1_1.spr");
    EXPECT_EQ(RunIGI1Conv("tex info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TexInfoPic) {
    IGI1CONV_NEED(f, "loading_us.pic");
    EXPECT_EQ(RunIGI1Conv("tex info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TexDecode) {
    IGI1CONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "texout";
    EXPECT_EQ(RunIGI1Conv("tex decode " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out + "\\FLARE00.tga"));
}
TEST_F(IGI1ConvTest, TexToPng) {
    IGI1CONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "flare.png";
    EXPECT_EQ(RunIGI1Conv("tex to-png " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, TexToTga) {
    IGI1CONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "flare.tga";
    EXPECT_EQ(RunIGI1Conv("tex to-tga " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, TexToPngResize) {
    IGI1CONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "flare_small.png";
    EXPECT_EQ(RunIGI1Conv("tex to-png " + Q(f) + " -o " + Q(out) + " --resize 16 16"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── mef ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, MefInfo) {
    IGI1CONV_NEED(f, "model1.mef");
    EXPECT_EQ(RunIGI1Conv("mef info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, MefExport) {
    IGI1CONV_NEED(f, "model1.mef");
    TempDir tmp;
    std::string out = tmp / "model.obj";
    EXPECT_EQ(RunIGI1Conv("mef export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, MefDump) {
    IGI1CONV_NEED(f, "model1.mef");
    TempDir tmp;
    std::string out = tmp / "model.txt";
    EXPECT_EQ(RunIGI1Conv("mef dump " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── qsc ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, QscValidate) {
    IGI1CONV_NEED(f, "AMMO.qsc");
    EXPECT_EQ(RunIGI1Conv("qsc validate " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, QscCompile) {
    IGI1CONV_NEED(f, "AMMO.qsc");
    TempDir tmp;
    std::string out = tmp / "ammo.qvm";
    EXPECT_EQ(RunIGI1Conv("qsc compile " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── qvm ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, QvmInfo) {
    IGI1CONV_NEED(f, "AMMO.QVM");
    EXPECT_EQ(RunIGI1Conv("qvm info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, QvmDisasm) {
    IGI1CONV_NEED(f, "AMMO.QVM");
    EXPECT_EQ(RunIGI1Conv("qvm disasm " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, QvmDecompile) {
    IGI1CONV_NEED(f, "AMMO.QVM");
    TempDir tmp;
    std::string out = tmp / "ammo.qsc";
    EXPECT_EQ(RunIGI1Conv("qvm decompile " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── res ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, ResList) {
    IGI1CONV_NEED(f, "SPRITES.RES");
    EXPECT_EQ(RunIGI1Conv("res list " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, ResExtract) {
    IGI1CONV_NEED(f, "SPRITES.RES");
    TempDir tmp;
    std::string out = tmp / "res_out";
    EXPECT_EQ(RunIGI1Conv("res extract " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(out));
}
TEST_F(IGI1ConvTest, ResUnpack) {
    IGI1CONV_NEED(f, "SPRITES.RES");
    TempDir tmp;
    std::string out = tmp / "res_unpacked";
    EXPECT_EQ(RunIGI1Conv("res unpack " + Q(f) + " " + Q(out)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(out));
}

// ─── mtp ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, MtpInfo) {
    IGI1CONV_NEED(f, "common.mtp");
    EXPECT_EQ(RunIGI1Conv("mtp info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, MtpDump) {
    IGI1CONV_NEED(f, "common.mtp");
    TempDir tmp;
    std::string out = tmp / "mtp.json";
    EXPECT_EQ(RunIGI1Conv("mtp dump " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, MtpToDat) {
    IGI1CONV_NEED(f, "common.mtp");
    TempDir tmp;
    std::string out = tmp / "common_from_mtp.dat";
    EXPECT_EQ(RunIGI1Conv("mtp to-dat " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── dat ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, DatInfo) {
    IGI1CONV_NEED(f, "common.dat");
    EXPECT_EQ(RunIGI1Conv("dat info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, DatExportJson) {
    IGI1CONV_NEED(f, "common.dat");
    TempDir tmp;
    std::string out = tmp / "dat.json";
    EXPECT_EQ(RunIGI1Conv("dat export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, DatExportText) {
    IGI1CONV_NEED(f, "common.dat");
    TempDir tmp;
    std::string out = tmp / "dat.txt";
    EXPECT_EQ(RunIGI1Conv("dat export " + Q(f) + " -o " + Q(out) + " --text"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, DatToMtp) {
    IGI1CONV_NEED(f, "common.dat");
    TempDir tmp;
    std::string out = tmp / "common_from_dat.mtp";
    EXPECT_EQ(RunIGI1Conv("dat to-mtp " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── graph ───────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, GraphInfo) {
    IGI1CONV_NEED(f, "graph1.dat");
    EXPECT_EQ(RunIGI1Conv("graph info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, GraphExport) {
    IGI1CONV_NEED(f, "graph1.dat");
    TempDir tmp;
    std::string out = tmp / "graph.json";
    EXPECT_EQ(RunIGI1Conv("graph export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── fnt ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, FntInfo) {
    IGI1CONV_NEED(f, "font1.fnt");
    EXPECT_EQ(RunIGI1Conv("fnt info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, FntExport) {
    IGI1CONV_NEED(f, "font1.fnt");
    TempDir tmp;
    std::string out = tmp / "font1.png";
    EXPECT_EQ(RunIGI1Conv("fnt export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── terrain ─────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, TerrainInfoLmp) {
    IGI1CONV_NEED(f, "TERRAIN.LMP");
    EXPECT_EQ(RunIGI1Conv("terrain info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TerrainInfoCtr) {
    IGI1CONV_NEED(f, "terrain.ctr");
    EXPECT_EQ(RunIGI1Conv("terrain info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TerrainExportLmp) {
    IGI1CONV_NEED(f, "TERRAIN.LMP");
    TempDir tmp;
    std::string out = tmp / "lmp.pgm";
    EXPECT_EQ(RunIGI1Conv("terrain export-lmp " + Q(f) + " -o " + Q(out)), 0);
    // single-pic LMP writes exactly <out>; multi-pic writes <stem>_N.pgm
    bool wrote = NonEmptyFile(out) || NonEmptyFile(tmp / "lmp_0.pgm");
    EXPECT_TRUE(wrote);
}
TEST_F(IGI1ConvTest, TerrainExportCtr) {
    IGI1CONV_NEED(f, "terrain.ctr");
    TempDir tmp;
    std::string out = tmp / "ctr.json";
    EXPECT_EQ(RunIGI1Conv("terrain export-ctr " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
