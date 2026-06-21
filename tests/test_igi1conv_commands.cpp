// test_igi1conv_commands.cpp — one or more tests for every igi1conv command and
// every subcommand, driven against the real game-file corpus.
#include "igi1conv_test_util.h"
#include <algorithm>

using namespace igi1conv_test;

// ─── tex ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, TexInfoTex) {
    IGI1CONV_NEED(f, "\\.tex$");
    EXPECT_EQ(RunIGI1Conv("tex info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TexInfoSpr) {
    IGI1CONV_NEED(f, "\\.spr$");
    EXPECT_EQ(RunIGI1Conv("tex info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TexInfoPic) {
    IGI1CONV_NEED(f, "\\.pic$");
    EXPECT_EQ(RunIGI1Conv("tex info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TexDecode) {
    IGI1CONV_NEED(f, "\\.tex$");
    TempDir tmp;
    std::string out = tmp / "texout";
    EXPECT_EQ(RunIGI1Conv("tex decode " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out + "\\FLARE00.tga"));
}
TEST_F(IGI1ConvTest, TexToPng) {
    IGI1CONV_NEED(f, "\\.tex$");
    TempDir tmp;
    std::string out = tmp / "flare.png";
    EXPECT_EQ(RunIGI1Conv("tex to-png " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, TexToTga) {
    IGI1CONV_NEED(f, "\\.tex$");
    TempDir tmp;
    std::string out = tmp / "flare.tga";
    EXPECT_EQ(RunIGI1Conv("tex to-tga " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, TexToPngResize) {
    IGI1CONV_NEED(f, "\\.tex$");
    TempDir tmp;
    std::string out = tmp / "flare_small.png";
    EXPECT_EQ(RunIGI1Conv("tex to-png " + Q(f) + " -o " + Q(out) + " --resize 16 16"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── mef ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, MefInfo) {
    IGI1CONV_NEED(f, "\\.mef$");
    EXPECT_EQ(RunIGI1Conv("mef info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, MefExport) {
    IGI1CONV_NEED(f, "\\.mef$");
    TempDir tmp;
    std::string out = tmp / "model.obj";
    EXPECT_EQ(RunIGI1Conv("mef export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, MefDump) {
    IGI1CONV_NEED(f, "\\.mef$");
    TempDir tmp;
    std::string out = tmp / "model.txt";
    EXPECT_EQ(RunIGI1Conv("mef dump " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── qsc ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, QscValidate) {
    IGI1CONV_NEED(f, "\\.qsc$");
    EXPECT_EQ(RunIGI1Conv("qsc validate " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, QscCompile) {
    IGI1CONV_NEED(f, "\\.qsc$");
    TempDir tmp;
    std::string out = tmp / "ammo.qvm";
    EXPECT_EQ(RunIGI1Conv("qsc compile " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── qvm ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, QvmInfo) {
    IGI1CONV_NEED(f, "\\.qvm$");
    EXPECT_EQ(RunIGI1Conv("qvm info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, QvmDisasm) {
    IGI1CONV_NEED(f, "\\.qvm$");
    EXPECT_EQ(RunIGI1Conv("qvm disasm " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, QvmDecompile) {
    IGI1CONV_NEED(f, "\\.qvm$");
    TempDir tmp;
    std::string out = tmp / "ammo.qsc";
    EXPECT_EQ(RunIGI1Conv("qvm decompile " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── res ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, ResList) {
    IGI1CONV_NEED(f, "\\.res$");
    EXPECT_EQ(RunIGI1Conv("res list " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, ResExtract) {
    IGI1CONV_NEED(f, "\\.res$");
    TempDir tmp;
    std::string out = tmp / "res_out";
    EXPECT_EQ(RunIGI1Conv("res extract " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(out));
}
TEST_F(IGI1ConvTest, ResUnpack) {
    IGI1CONV_NEED(f, "\\.res$");
    TempDir tmp;
    std::string out = tmp / "res_unpacked";
    EXPECT_EQ(RunIGI1Conv("res unpack " + Q(f) + " " + Q(out)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(out));
}

// ─── mtp ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, MtpInfo) {
    IGI1CONV_NEED(f, "\\.mtp$");
    EXPECT_EQ(RunIGI1Conv("mtp info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, MtpDump) {
    IGI1CONV_NEED(f, "\\.mtp$");
    TempDir tmp;
    std::string out = tmp / "mtp.json";
    EXPECT_EQ(RunIGI1Conv("mtp dump " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, MtpToDat) {
    IGI1CONV_NEED(f, "\\.mtp$");
    TempDir tmp;
    std::string out = tmp / "common_from_mtp.dat";
    EXPECT_EQ(RunIGI1Conv("mtp to-dat " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── dat ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, DatInfo) {
    IGI1CONV_NEED(f, "^(?!.*graph).*\\.dat$");
    EXPECT_EQ(RunIGI1Conv("dat info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, DatExportJson) {
    IGI1CONV_NEED(f, "^(?!.*graph).*\\.dat$");
    TempDir tmp;
    std::string out = tmp / "dat.json";
    EXPECT_EQ(RunIGI1Conv("dat export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, DatExportText) {
    IGI1CONV_NEED(f, "^(?!.*graph).*\\.dat$");
    TempDir tmp;
    std::string out = tmp / "dat.txt";
    EXPECT_EQ(RunIGI1Conv("dat export " + Q(f) + " -o " + Q(out) + " --text"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, DatToMtp) {
    IGI1CONV_NEED(f, "^(?!.*graph).*\\.dat$");
    TempDir tmp;
    std::string out = tmp / "common_from_dat.mtp";
    EXPECT_EQ(RunIGI1Conv("dat to-mtp " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── graph ───────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, GraphInfo) {
    IGI1CONV_NEED(f, "graph.*\\.dat$");
    EXPECT_EQ(RunIGI1Conv("graph info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, GraphExport) {
    IGI1CONV_NEED(f, "graph.*\\.dat$");
    TempDir tmp;
    std::string out = tmp / "graph.json";
    EXPECT_EQ(RunIGI1Conv("graph export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── fnt ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, FntInfo) {
    IGI1CONV_NEED(f, "\\.fnt$");
    EXPECT_EQ(RunIGI1Conv("fnt info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, FntExport) {
    IGI1CONV_NEED(f, "\\.fnt$");
    TempDir tmp;
    std::string out = tmp / "font1.png";
    EXPECT_EQ(RunIGI1Conv("fnt export " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── terrain ─────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, TerrainInfoLmp) {
    IGI1CONV_NEED(f, "\\.lmp$");
    EXPECT_EQ(RunIGI1Conv("terrain info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TerrainInfoCtr) {
    IGI1CONV_NEED(f, "\\.ctr$");
    EXPECT_EQ(RunIGI1Conv("terrain info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, TerrainExportLmp) {
    IGI1CONV_NEED(f, "\\.lmp$");
    TempDir tmp;
    std::string out = tmp / "lmp.pgm";
    EXPECT_EQ(RunIGI1Conv("terrain export-lmp " + Q(f) + " -o " + Q(out)), 0);
    // single-pic LMP writes exactly <out>; multi-pic writes <stem>_N.pgm
    bool wrote = NonEmptyFile(out) || NonEmptyFile(tmp / "lmp_0.pgm");
    EXPECT_TRUE(wrote);
}
TEST_F(IGI1ConvTest, TerrainExportCtr) {
    IGI1CONV_NEED(f, "\\.ctr$");
    TempDir tmp;
    std::string out = tmp / "ctr.json";
    EXPECT_EQ(RunIGI1Conv("terrain export-ctr " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}

// ─── iff ─────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, IffInfo) {
    IGI1CONV_NEED(f, "\\.iff$");
    EXPECT_EQ(RunIGI1Conv("iff info " + Q(f)), 0);
}
TEST_F(IGI1ConvTest, IffConvert) {
    IGI1CONV_NEED(f, "\\.iff$");
    TempDir tmp;
    std::string outDir = tmp / "iff_convert";
    EXPECT_EQ(RunIGI1Conv("iff convert " + Q(f) + " " + Q(outDir)), 0);
    // At least one .BEF should appear (one per animation in the file).
    bool any = false;
    for (auto& e : std::filesystem::directory_iterator(outDir)) {
        if (e.is_regular_file()) {
            std::string ext = e.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".bef") { any = true; break; }
        }
    }
    EXPECT_TRUE(any) << "no .BEF files in " << outDir;
}
TEST_F(IGI1ConvTest, IffRebuild) {
    IGI1CONV_NEED(f, "\\.iff$");
    TempDir tmp;
    std::string out = tmp / "iff_rebuild.iff";
    EXPECT_EQ(RunIGI1Conv("iff rebuild " + Q(f) + " " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
    // The rebuilt IFF must parse with iff info without error.
    EXPECT_EQ(RunIGI1Conv("iff info " + Q(out)), 0);
}
TEST_F(IGI1ConvTest, IffDecompile) {
    IGI1CONV_NEED(f, "\\.iff$");
    TempDir tmp;
    std::string outDir = tmp / "iff_decomp";
    EXPECT_EQ(RunIGI1Conv("iff decompile " + Q(f) + " " + Q(outDir)), 0);
    EXPECT_TRUE(std::filesystem::is_directory(outDir));
    // We expect a <basename>.IFF text file at the top of outDir.
    std::string base = std::filesystem::path(f).stem().string();
    EXPECT_TRUE(NonEmptyFile(outDir + "\\" + base + ".IFF"));
}
TEST_F(IGI1ConvTest, IffCreateFromBefs) {
    IGI1CONV_NEED(f, "\\.iff$");
    TempDir tmp;
    std::string bDir   = tmp / "befs";
    std::string outIff = tmp / "from_befs.iff";
    ASSERT_EQ(RunIGI1Conv("iff convert " + Q(f) + " " + Q(bDir)), 0);
    EXPECT_EQ(RunIGI1Conv("iff create " + Q(bDir) + " " + Q(outIff)), 0);
    EXPECT_TRUE(NonEmptyFile(outIff));
    EXPECT_EQ(RunIGI1Conv("iff info " + Q(outIff)), 0);
}
TEST_F(IGI1ConvTest, IffExportGif) {
    IGI1CONV_NEED(f, "\\.iff$");
    TempDir tmp;
    std::string out = tmp / "anim.gif";
    // Use a small canvas to keep the test fast; fps deliberately low
    // to minimise frame count.
    EXPECT_EQ(RunIGI1Conv("iff export-gif " + Q(f) + " " + Q(out) + " 160 120 8"), 0);
    EXPECT_TRUE(NonEmptyFile(out));
    // GIF89a magic.
    std::ifstream g(out, std::ios::binary);
    char magic[6] = {0};
    g.read(magic, 6);
    EXPECT_EQ(std::string(magic, 6), std::string("GIF89a"));
}

