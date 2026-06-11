// test_gconv_commands.cpp — one or more tests for every gconv command and
// every subcommand, driven against the real game-file corpus.
#include "gconv_test_util.h"

using namespace gconv_test;

// ─── tex ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, TexInfoTex) {
    GCONV_NEED(f, "FLARE00.TEX");
    EXPECT_EQ(RunGConv("tex info " + Q(f)), 0);
}
TEST_F(GConvTest, TexInfoSpr) {
    GCONV_NEED(f, "arrow1_1.spr");
    EXPECT_EQ(RunGConv("tex info " + Q(f)), 0);
}
TEST_F(GConvTest, TexInfoPic) {
    GCONV_NEED(f, "loading_us.pic");
    EXPECT_EQ(RunGConv("tex info " + Q(f)), 0);
}
TEST_F(GConvTest, TexDecode) {
    GCONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "texout";
    EXPECT_EQ(RunGConv("tex decode " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out + "\\FLARE00.tga"));
}
TEST_F(GConvTest, TexToPng) {
    GCONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "flare.png";
    EXPECT_EQ(RunGConv("tex to-png " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(GConvTest, TexToTga) {
    GCONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "flare.tga";
    EXPECT_EQ(RunGConv("tex to-tga " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(GConvTest, TexToPngResize) {
    GCONV_NEED(f, "FLARE00.TEX");
    TempDir tmp;
    std::string out = tmp / "flare_small.png";
    EXPECT_EQ(RunGConv("tex to-png " + Q(f) + " -o " + Q(out) + " --resize 16 16"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── mef ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, MefInfo) {
    GCONV_NEED(f, "model1.mef");
    EXPECT_EQ(RunGConv("mef info " + Q(f)), 0);
}
TEST_F(GConvTest, MefExport) {
    GCONV_NEED(f, "model1.mef");
    TempDir tmp;
    std::string out = tmp / "model.obj";
    EXPECT_EQ(RunGConv("mef export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(GConvTest, MefDump) {
    GCONV_NEED(f, "model1.mef");
    TempDir tmp;
    std::string out = tmp / "model.txt";
    EXPECT_EQ(RunGConv("mef dump " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── qsc ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, QscValidate) {
    GCONV_NEED(f, "AMMO.qsc");
    EXPECT_EQ(RunGConv("qsc validate " + Q(f)), 0);
}
TEST_F(GConvTest, QscCompile) {
    GCONV_NEED(f, "AMMO.qsc");
    TempDir tmp;
    std::string out = tmp / "ammo.qvm";
    EXPECT_EQ(RunGConv("qsc compile " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── qvm ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, QvmInfo) {
    GCONV_NEED(f, "AMMO.QVM");
    EXPECT_EQ(RunGConv("qvm info " + Q(f)), 0);
}
TEST_F(GConvTest, QvmDisasm) {
    GCONV_NEED(f, "AMMO.QVM");
    EXPECT_EQ(RunGConv("qvm disasm " + Q(f)), 0);
}
TEST_F(GConvTest, QvmDecompile) {
    GCONV_NEED(f, "AMMO.QVM");
    TempDir tmp;
    std::string out = tmp / "ammo.qsc";
    EXPECT_EQ(RunGConv("qvm decompile " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── res ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, ResList) {
    GCONV_NEED(f, "SPRITES.RES");
    EXPECT_EQ(RunGConv("res list " + Q(f)), 0);
}
TEST_F(GConvTest, ResExtract) {
    GCONV_NEED(f, "SPRITES.RES");
    TempDir tmp;
    std::string out = tmp / "res_out";
    EXPECT_EQ(RunGConv("res extract " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(out));
}
TEST_F(GConvTest, ResUnpack) {
    GCONV_NEED(f, "SPRITES.RES");
    TempDir tmp;
    std::string out = tmp / "res_unpacked";
    EXPECT_EQ(RunGConv("res unpack " + Q(f) + " " + Q(out)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(out));
}

// ─── mtp ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, MtpInfo) {
    GCONV_NEED(f, "common.mtp");
    EXPECT_EQ(RunGConv("mtp info " + Q(f)), 0);
}
TEST_F(GConvTest, MtpDump) {
    GCONV_NEED(f, "common.mtp");
    TempDir tmp;
    std::string out = tmp / "mtp.json";
    EXPECT_EQ(RunGConv("mtp dump " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(GConvTest, MtpToDat) {
    GCONV_NEED(f, "common.mtp");
    TempDir tmp;
    std::string out = tmp / "common_from_mtp.dat";
    EXPECT_EQ(RunGConv("mtp to-dat " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── dat ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, DatInfo) {
    GCONV_NEED(f, "common.dat");
    EXPECT_EQ(RunGConv("dat info " + Q(f)), 0);
}
TEST_F(GConvTest, DatExportJson) {
    GCONV_NEED(f, "common.dat");
    TempDir tmp;
    std::string out = tmp / "dat.json";
    EXPECT_EQ(RunGConv("dat export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(GConvTest, DatExportText) {
    GCONV_NEED(f, "common.dat");
    TempDir tmp;
    std::string out = tmp / "dat.txt";
    EXPECT_EQ(RunGConv("dat export " + Q(f) + " -o " + Q(out) + " --text"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(GConvTest, DatToMtp) {
    GCONV_NEED(f, "common.dat");
    TempDir tmp;
    std::string out = tmp / "common_from_dat.mtp";
    EXPECT_EQ(RunGConv("dat to-mtp " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── graph ───────────────────────────────────────────────────────────────────
TEST_F(GConvTest, GraphInfo) {
    GCONV_NEED(f, "graph1.dat");
    EXPECT_EQ(RunGConv("graph info " + Q(f)), 0);
}
TEST_F(GConvTest, GraphExport) {
    GCONV_NEED(f, "graph1.dat");
    TempDir tmp;
    std::string out = tmp / "graph.json";
    EXPECT_EQ(RunGConv("graph export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── fnt ─────────────────────────────────────────────────────────────────────
TEST_F(GConvTest, FntInfo) {
    GCONV_NEED(f, "font1.fnt");
    EXPECT_EQ(RunGConv("fnt info " + Q(f)), 0);
}
TEST_F(GConvTest, FntExport) {
    GCONV_NEED(f, "font1.fnt");
    TempDir tmp;
    std::string out = tmp / "font1.png";
    EXPECT_EQ(RunGConv("fnt export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── terrain ─────────────────────────────────────────────────────────────────
TEST_F(GConvTest, TerrainInfoLmp) {
    GCONV_NEED(f, "TERRAIN.LMP");
    EXPECT_EQ(RunGConv("terrain info " + Q(f)), 0);
}
TEST_F(GConvTest, TerrainInfoCtr) {
    GCONV_NEED(f, "terrain.ctr");
    EXPECT_EQ(RunGConv("terrain info " + Q(f)), 0);
}
TEST_F(GConvTest, TerrainExportLmp) {
    GCONV_NEED(f, "TERRAIN.LMP");
    TempDir tmp;
    std::string out = tmp / "lmp.pgm";
    EXPECT_EQ(RunGConv("terrain export-lmp " + Q(f) + " -o " + Q(out)), 0);
    // single-pic LMP writes exactly <out>; multi-pic writes <stem>_N.pgm
    bool wrote = NonEmptyFile(out) || NonEmptyFile(tmp / "lmp_0.pgm");
    EXPECT_TRUE(wrote);
}
TEST_F(GConvTest, TerrainExportCtr) {
    GCONV_NEED(f, "terrain.ctr");
    TempDir tmp;
    std::string out = tmp / "ctr.json";
    EXPECT_EQ(RunGConv("terrain export-ctr " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
