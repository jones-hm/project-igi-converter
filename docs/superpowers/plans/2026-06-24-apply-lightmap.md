# Apply Lightmap on .mef Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Right-click a `.mef` in the GUI → "Apply Lightmap" resolves and renders its baked `.olm` lightmap(s) via `objects.qsc`.

**Architecture:** A new generic `Task_New` tree-walking parser resolves model-id → lightmap logical id from `objects.qsc`; a resolver globs the matching `.olm` files (auto-unpacking `lightmaps.res` if needed); the MEF parser gains a second UV channel for lightmap models; the GUI viewer blends a second texture sampler into the existing shader per render block.

**Tech Stack:** C++17, Qt5/QOpenGLWidget, existing `igi1conv` CLI/GUI, GoogleTest.

## Global Constraints
- No git push — local commits only, one per task, on the `develop` branch.
- Build config: `Release` (already configured under `build/`, output to `bin/Release/`).
- Test corpus path: `D:\IGI1\missions\location0\level1\lightmaps\lightmaps_unpacked` (1308 `.olm` files, verified parsing cleanly with the existing `cmd_olm.cpp`).
- Do not touch IGI2 24-byte layer descriptor / multi-layer support — out of scope (no IGI2 corpus available).
- Existing behavior for models without a lightmap binding must be unchanged (diffuse-only rendering).

---

### Task 1: OLM CLI test coverage

**Files:**
- Modify: `tests/test_igi1conv_commands.cpp` (append after `TerrainExportCtr`, ~line 766, before the `IffInfo` block)

**Interfaces:**
- Consumes: `RunIGI1Conv(args)` (returns int exit code), `IGI1CONV_NEED(var, pattern)` macro, `TempDir`, `Q()`, `NonEmptyFile()` — all from `tests/igi1conv_test_util.h` (already in scope via existing includes in this file).
- Produces: nothing consumed by later tasks — this is a leaf test addition.

- [ ] **Step 1: Write the failing tests**

Insert this block immediately before `TEST_F(IGI1ConvTest, IffInfo)` (around line 767):

```cpp
// ─── olm ─────────────────────────────────────────────────────────────────────
TEST_F(IGI1ConvTest, OlmInfo) {
    IGI1CONV_NEED(f, "\\.olm$");
    std::string out;
    EXPECT_EQ(RunIGI1Conv("olm info " + Q(f), &out), 0);
    EXPECT_NE(out.find("resolution:"), std::string::npos);
}
TEST_F(IGI1ConvTest, OlmToPng) {
    IGI1CONV_NEED(f, "\\.olm$");
    TempDir tmp;
    std::string out = tmp / "lightmap.png";
    EXPECT_EQ(RunIGI1Conv("olm to-png " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
TEST_F(IGI1ConvTest, OlmToTga) {
    IGI1CONV_NEED(f, "\\.olm$");
    TempDir tmp;
    std::string out = tmp / "lightmap.tga";
    EXPECT_EQ(RunIGI1Conv("olm to-tga " + Q(f) + " -o " + Q(out)), 0);
    EXPECT_TRUE(NonEmptyFile(out));
}
```

- [ ] **Step 2: Build the test binary**

Run: `cmake --build build --config Release --target igi1conv_tests`
Expected: build succeeds (tests aren't run yet, just compiled).

- [ ] **Step 3: Run the new tests against the real corpus**

Run: `bin/Release/igi1conv_tests.exe --game-path="D:\IGI1" --gtest_filter=IGI1ConvTest.Olm*`
Expected: `[ PASSED ] 3 tests.`

- [ ] **Step 4: Commit**

```bash
git add tests/test_igi1conv_commands.cpp
git commit -m "test: add OLM info/to-png/to-tga coverage against real corpus"
```

---

### Task 2: Generic lightmap-binding parser in qsc_object_parser

**Files:**
- Modify: `source/parsers/qsc_object_parser.h`
- Modify: `source/parsers/qsc_object_parser.cpp`
- Test: Create `tests/test_lightmap_binding_parser.cpp`
- Modify: `tests/CMakeLists.txt` (add the new test source to the `igi1conv_tests` target — find the existing `add_executable(igi1conv_tests ...)` or `target_sources` listing `test_qsc_object_parser.cpp` and add the new file next to it)

**Interfaces:**
- Produces: `igi1conv::LightmapBindingSet` with `static LightmapBindingSet parse(const std::string& qscText, std::string* err = nullptr)` and `std::optional<std::string> logicalIdForModel(const std::string& modelStem) const`. Task 3 (lightmap_resolver) and Task 5 (GUI) call `logicalIdForModel`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_lightmap_binding_parser.cpp`:

```cpp
#include "qsc_object_parser.h"
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
```

- [ ] **Step 2: Add the new test file to the build and confirm it fails to compile**

Open `tests/CMakeLists.txt`, find the line listing `test_qsc_object_parser.cpp` (in the `igi1conv_tests` sources), and add `test_lightmap_binding_parser.cpp` right after it in the same list.

Run: `cmake --build build --config Release --target igi1conv_tests`
Expected: FAIL — `'LightmapBindingSet': is not a member of 'igi1conv'` (the type doesn't exist yet).

- [ ] **Step 3: Add the `LightmapBindingSet` declaration**

Append to `source/parsers/qsc_object_parser.h` (before the closing `} // namespace igi1conv` on line 77):

```cpp
// Maps a model id (the quoted string argument used anywhere inside a
// Task_New(...) tree, including nested child Task_New calls) to the
// logical lightmap id captured by a nested Task_New("LightmapInfo", ...,
// "<logical_id>") in that SAME tree.  Used to resolve which obj00000_*.olm
// files belong to a given .mef.
struct LightmapBindingSet {
    // modelId -> logical lightmap id (e.g. "435_01_1" -> "obj00000")
    std::vector<std::pair<std::string, std::string>> bindings;

    std::optional<std::string> logicalIdForModel(const std::string& modelId) const;

    static LightmapBindingSet parse(const std::string& qscText,
                                     std::string* err = nullptr);
};
```

Add `#include <optional>` and `#include <utility>` to the top of `source/parsers/qsc_object_parser.h` next to the existing `#include <vector>`.

- [ ] **Step 4: Implement the parser**

Append to `source/parsers/qsc_object_parser.cpp`, after the closing brace of `animationsForModel` (after line 294, before the final `} // namespace igi1conv` on line 296):

```cpp
std::optional<std::string>
LightmapBindingSet::logicalIdForModel(const std::string& modelId) const {
    for (const auto& [model, logicalId] : bindings) {
        if (model == modelId) return logicalId;
    }
    return std::nullopt;
}

namespace {

// Collect every quoted string literal that appears as a top-level argument
// of ANY Task_New(...) call within `src` (including nested calls), and
// separately collect the logical id of every Task_New("LightmapInfo", ...)
// call.  Both lists are scoped to one balanced top-level Task_New(...) tree
// at a time by the caller.
struct TreeScanResult {
    std::vector<std::string> modelIds;     // every quoted string seen
    std::vector<std::string> lightmapIds;  // last quoted string of each LightmapInfo call
};

void ScanTaskTree(const std::string& src, size_t start, size_t end, TreeScanResult& out) {
    size_t searchFrom = start;
    while (true) {
        size_t p = src.find("Task_New", searchFrom);
        if (p == std::string::npos || p >= end) break;
        size_t parenPos = p + 8; // strlen("Task_New")
        while (parenPos < end && std::isspace(static_cast<unsigned char>(src[parenPos]))) ++parenPos;
        if (parenPos >= end || src[parenPos] != '(') { searchFrom = p + 8; continue; }

        // Find the matching ')' for this Task_New call, honouring nested
        // parens and string literals (the call may contain nested Task_New
        // children as trailing arguments).
        size_t depth = 1;
        size_t cursor = parenPos + 1;
        while (cursor < end && depth > 0) {
            if (src[cursor] == '"') {
                ++cursor;
                while (cursor < end && src[cursor] != '"') {
                    if (src[cursor] == '\\' && cursor + 1 < end) ++cursor;
                    ++cursor;
                }
            } else if (src[cursor] == '(') {
                ++depth;
            } else if (src[cursor] == ')') {
                --depth;
            }
            ++cursor;
        }
        size_t callEnd = cursor; // one past the matching ')'

        // Pull out every top-level quoted string directly inside THIS
        // call's argument list (skip nested Task_New(...) sub-trees so we
        // don't double count their own strings here -- they get visited
        // by the recursive ScanTaskTree call below instead).
        std::vector<std::string> directStrings;
        size_t i = parenPos + 1;
        int innerDepth = 0;
        while (i < callEnd - 1) {
            if (src[i] == '(') { ++innerDepth; ++i; continue; }
            if (src[i] == ')') { --innerDepth; ++i; continue; }
            if (innerDepth == 0 && src[i] == '"') {
                size_t s = i;
                ++i;
                while (i < callEnd && src[i] != '"') {
                    if (src[i] == '\\' && i + 1 < callEnd) ++i;
                    ++i;
                }
                directStrings.push_back(src.substr(s + 1, i - s - 1));
                ++i;
                continue;
            }
            ++i;
        }

        bool isLightmapInfo = !directStrings.empty() && directStrings.size() >= 1 &&
                              std::find(directStrings.begin(), directStrings.end(), "LightmapInfo") != directStrings.end();
        if (isLightmapInfo && !directStrings.empty()) {
            out.lightmapIds.push_back(directStrings.back());
        } else {
            for (auto& s : directStrings) out.modelIds.push_back(s);
        }

        // Recurse into the nested Task_New children inside [parenPos, callEnd).
        ScanTaskTree(src, parenPos + 1, callEnd - 1, out);

        searchFrom = callEnd;
    }
}

} // namespace

LightmapBindingSet LightmapBindingSet::parse(const std::string& qscText, std::string* err) {
    LightmapBindingSet set;
    size_t searchFrom = 0;
    while (true) {
        size_t p = qscText.find("Task_New", searchFrom);
        if (p == std::string::npos) break;
        bool leftOk = (p == 0) || (!std::isalnum(static_cast<unsigned char>(qscText[p - 1])) && qscText[p - 1] != '_');
        if (!leftOk) { searchFrom = p + 8; continue; }

        // Only treat this as a *top-level* tree root if it isn't itself
        // nested inside an already-scanned tree.  Since ScanTaskTree
        // recurses and we always advance searchFrom past callEnd for
        // roots, re-finding "Task_New" here naturally lands on the next
        // sibling root, not a child (children were already consumed by
        // the recursive scan below).
        size_t parenPos = p + 8;
        while (parenPos < qscText.size() && std::isspace(static_cast<unsigned char>(qscText[parenPos]))) ++parenPos;
        if (parenPos >= qscText.size() || qscText[parenPos] != '(') { searchFrom = p + 8; continue; }

        size_t depth = 1;
        size_t cursor = parenPos + 1;
        while (cursor < qscText.size() && depth > 0) {
            if (qscText[cursor] == '"') {
                ++cursor;
                while (cursor < qscText.size() && qscText[cursor] != '"') {
                    if (qscText[cursor] == '\\' && cursor + 1 < qscText.size()) ++cursor;
                    ++cursor;
                }
            } else if (qscText[cursor] == '(') {
                ++depth;
            } else if (qscText[cursor] == ')') {
                --depth;
            }
            ++cursor;
        }
        size_t callEnd = cursor;

        TreeScanResult result;
        ScanTaskTree(qscText, parenPos + 1, callEnd - 1, result);
        // Also account for top-level direct strings of the root call itself.
        {
            std::vector<std::string> rootDirect;
            size_t i = parenPos + 1;
            int innerDepth = 0;
            while (i < callEnd - 1) {
                if (qscText[i] == '(') { ++innerDepth; ++i; continue; }
                if (qscText[i] == ')') { --innerDepth; ++i; continue; }
                if (innerDepth == 0 && qscText[i] == '"') {
                    size_t s = i;
                    ++i;
                    while (i < callEnd && qscText[i] != '"') {
                        if (qscText[i] == '\\' && i + 1 < callEnd) ++i;
                        ++i;
                    }
                    rootDirect.push_back(qscText.substr(s + 1, i - s - 1));
                    ++i;
                    continue;
                }
                ++i;
            }
            bool rootIsLightmapInfo = std::find(rootDirect.begin(), rootDirect.end(), "LightmapInfo") != rootDirect.end();
            if (rootIsLightmapInfo && !rootDirect.empty()) {
                result.lightmapIds.push_back(rootDirect.back());
            } else {
                for (auto& s : rootDirect) result.modelIds.push_back(s);
            }
        }

        if (!result.lightmapIds.empty()) {
            const std::string& logicalId = result.lightmapIds.front();
            for (const auto& modelId : result.modelIds) {
                set.bindings.push_back({modelId, logicalId});
            }
        }

        searchFrom = callEnd;
    }

    if (err) *err = "";
    return set;
}
```

Add `#include <optional>` near the other includes at the top of `source/parsers/qsc_object_parser.cpp`.

- [ ] **Step 5: Build and run the tests**

Run: `cmake --build build --config Release --target igi1conv_tests`
Run: `bin/Release/igi1conv_tests.exe --gtest_filter=LightmapBindingParser.*`
Expected: `[ PASSED ] 2 tests.`

- [ ] **Step 6: Commit**

```bash
git add source/parsers/qsc_object_parser.h source/parsers/qsc_object_parser.cpp tests/test_lightmap_binding_parser.cpp tests/CMakeLists.txt
git commit -m "feat: parse lightmap bindings from objects.qsc Task_New trees"
```

---

### Task 3: Lightmap file resolver

**Files:**
- Create: `source/parsers/lightmap_resolver.h`
- Create: `source/parsers/lightmap_resolver.cpp`
- Test: Create `tests/test_lightmap_resolver.cpp`
- Modify: `tests/CMakeLists.txt` (add `test_lightmap_resolver.cpp` next to the file added in Task 2)
- Modify: `igi1conv/CMakeLists.txt` or the main `CMakeLists.txt` source list that already lists `source/parsers/qsc_object_parser.cpp` — add `source/parsers/lightmap_resolver.cpp` right next to it so both the GUI/CLI target and the tests target pick it up. (Search for `qsc_object_parser.cpp` in `CMakeLists.txt` to find the exact list.)

**Interfaces:**
- Consumes: `igi1conv::LightmapBindingSet::parse` and `logicalIdForModel` (Task 2). `int cmd_res(int argc, char** argv)` is NOT reused directly (it's a CLI entry point) — instead this resolver calls `RES_ForEachEntry` directly from `res_parser.h` (same function `cmd_res.cpp`'s `unpack` subcommand uses) to unpack `lightmaps.res` into `lightmaps_unpacked/`.
- Produces: `igi1conv::ResolveLightmapFiles(const std::string& objectsQscPath, const std::string& mefStem) -> std::vector<std::string>` (sorted absolute paths to matching `.olm` files, empty if no binding or no files found). Task 5 (GUI) calls this directly.

- [ ] **Step 1: Write the failing test**

Create `tests/test_lightmap_resolver.cpp`:

```cpp
#include "lightmap_resolver.h"
#include "igi1conv_test_util.h"
#include <gtest/gtest.h>
#include <filesystem>

using namespace igi1conv;
using namespace igi1conv_test;

TEST(LightmapResolver, ResolvesRealCorpusBinding) {
    std::string qscPath = FindCorpusFileByRegex("objects\\.qsc$");
    if (qscPath.empty()) GTEST_SKIP() << "no objects.qsc in corpus (set IGI_GAME_PATH)";

    // The lightmaps folder used by ResolveLightmapFiles is derived as
    // <objects.qsc's mission dir>/level1/lightmaps/lightmaps_unpacked —
    // this test only checks that SOME logical id resolves to existing
    // files for at least one model id referenced by a LightmapInfo-bound
    // Task_New tree; it does not hard-code a specific model id since the
    // real objects.qsc content can vary by corpus snapshot.
    std::ifstream f(qscPath);
    std::string qscText((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    LightmapBindingSet set = LightmapBindingSet::parse(qscText);
    if (set.bindings.empty()) GTEST_SKIP() << "no lightmap bindings found in this objects.qsc";

    auto files = ResolveLightmapFiles(qscPath, set.bindings.front().first);
    EXPECT_FALSE(files.empty());
    for (auto& p : files) EXPECT_TRUE(std::filesystem::exists(p));
}

TEST(LightmapResolver, EmptyForUnknownModel) {
    auto files = ResolveLightmapFiles("Z:\\nonexistent\\objects.qsc", "no_such_model");
    EXPECT_TRUE(files.empty());
}
```

- [ ] **Step 2: Add to build and confirm failure**

Add `test_lightmap_resolver.cpp` to `tests/CMakeLists.txt` next to `test_lightmap_binding_parser.cpp`. Add `source/parsers/lightmap_resolver.cpp` to the main source list in the root `CMakeLists.txt` next to `source/parsers/qsc_object_parser.cpp`.

Run: `cmake --build build --config Release --target igi1conv_tests`
Expected: FAIL — `lightmap_resolver.h: No such file or directory`.

- [ ] **Step 3: Implement the header**

Create `source/parsers/lightmap_resolver.h`:

```cpp
#pragma once
#include <string>
#include <vector>

namespace igi1conv {

// Given the path to a level's decompiled objects.qsc and a .mef model id
// (filename stem, e.g. "435_01_1"), returns the sorted list of .olm
// lightmap file paths bound to that model, or an empty vector if no
// binding exists or no matching files are found on disk.
//
// The level's lightmaps folder is derived as a sibling of the directory
// containing objects.qsc: <dir>/lightmaps/lightmaps_unpacked. If that
// folder doesn't exist but <dir>/lightmaps/lightmaps.res does, it is
// unpacked into lightmaps_unpacked automatically.
std::vector<std::string> ResolveLightmapFiles(const std::string& objectsQscPath,
                                               const std::string& mefStem);

} // namespace igi1conv
```

- [ ] **Step 4: Implement the resolver**

Create `source/parsers/lightmap_resolver.cpp`:

```cpp
#include "lightmap_resolver.h"
#include "qsc_object_parser.h"
#include "res_parser.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

namespace igi1conv {

std::vector<std::string> ResolveLightmapFiles(const std::string& objectsQscPath,
                                               const std::string& mefStem) {
    std::vector<std::string> result;

    if (!fs::exists(objectsQscPath)) return result;

    std::ifstream f(objectsQscPath);
    std::string qscText((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    LightmapBindingSet bindings = LightmapBindingSet::parse(qscText);

    auto logicalId = bindings.logicalIdForModel(mefStem);
    if (!logicalId.has_value()) return result;

    fs::path levelDir = fs::path(objectsQscPath).parent_path();
    fs::path lightmapsDir = levelDir / "lightmaps";
    fs::path unpackedDir = lightmapsDir / "lightmaps_unpacked";

    if (!fs::exists(unpackedDir)) {
        fs::path resPath = lightmapsDir / "lightmaps.res";
        if (!fs::exists(resPath)) return result;

        std::error_code ec;
        fs::create_directories(unpackedDir, ec);
        if (ec) return result;

        std::string err;
        RES_ForEachEntry(resPath.string(),
            [&](const std::string& name, const uint8_t* data, size_t size) {
                fs::path entryPath(name);
                fs::path outPath = unpackedDir / entryPath.filename();
                std::ofstream ofs(outPath, std::ios::binary);
                if (!ofs) return;
                ofs.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
            }, err);
        if (!err.empty() || !fs::exists(unpackedDir)) return result;
    }

    std::regex pattern("^" + *logicalId + "_\\d{5}\\.olm$", std::regex_constants::icase);
    for (const auto& entry : fs::directory_iterator(unpackedDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, pattern)) {
            result.push_back(entry.path().string());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace igi1conv
```

- [ ] **Step 5: Build and run**

Run: `cmake --build build --config Release --target igi1conv_tests`
Run: `bin/Release/igi1conv_tests.exe --game-path="D:\IGI1" --gtest_filter=LightmapResolver.*`
Expected: `[ PASSED ]` (2 tests; the first may `SKIP` if the corpus's `objects.qsc` location differs — that's acceptable per the existing corpus-skip convention used throughout this test suite).

- [ ] **Step 6: Commit**

```bash
git add source/parsers/lightmap_resolver.h source/parsers/lightmap_resolver.cpp tests/test_lightmap_resolver.cpp tests/CMakeLists.txt CMakeLists.txt
git commit -m "feat: resolve bound .olm lightmap files for a model from objects.qsc"
```

---

### Task 4: MEF parser — second UV channel for lightmap models

**Files:**
- Modify: `source/parsers/mef_native.h:12-20` (RenderVertex struct)
- Modify: `source/parsers/mef_native.cpp:242-296` (ParseRenderVertices)
- Test: Create `tests/test_mef_lightmap_uv.cpp`
- Modify: `tests/CMakeLists.txt` (add the new test file)

**Interfaces:**
- Consumes: nothing new — extends the existing `ParseMefFile` / `ParseRenderVertices` pipeline.
- Produces: `RenderVertex::uv2` (glm::vec2, populated only when modelType==3; zero-initialized otherwise). Task 5 (GUI) reads `geometry.vertices[i].uv2` when modelType==3.

- [ ] **Step 1: Write the failing test**

Create `tests/test_mef_lightmap_uv.cpp`:

```cpp
#include "mef_native.h"
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
```

- [ ] **Step 2: Add to build and confirm failure**

Add `test_mef_lightmap_uv.cpp` to `tests/CMakeLists.txt` next to the other new test files.

Run: `cmake --build build --config Release --target igi1conv_tests`
Expected: FAIL — `class "RenderVertex" has no member "uv2"`.

- [ ] **Step 3: Add the `uv2` field**

In `source/parsers/mef_native.h`, change the `RenderVertex` struct (lines 12-20):

```cpp
struct RenderVertex {
    glm::vec3 pos{0.f};
    glm::vec3 rawPos{0.f};      // raw XTRV position (NOT scaled, NOT baked) — for ASCII export
    glm::vec3 normal{0.f};      // from XTRV bytes +12..+23
    glm::vec2 uv{0.f};
    glm::vec2 uv2{0.f};         // lightmap UV (XTRV bytes +32..+39, modelType==3 only)
    uint16_t boneIndex{0};
    uint16_t localVertexId{0};  // XTRV.vn @+36
    float    weight{1.0f};      // XTRV.w  @+32
};
```

- [ ] **Step 4: Populate `uv2` in the parser**

In `source/parsers/mef_native.cpp`, inside the vertex-fill loop of `ParseRenderVertices` (after the existing `vertices[i].uv = ...` block, before the `if (modelType == 1)` block at line 288), add:

```cpp
        if (modelType == 3) {
            vertices[i].uv2 = glm::vec2(
                ReadValue<float>(bytes, base + 32),
                ReadValue<float>(bytes, base + 36)
            );
        }
```

- [ ] **Step 5: Build and run**

Run: `cmake --build build --config Release --target igi1conv_tests`
Run: `bin/Release/igi1conv_tests.exe --game-path="D:\IGI1" --gtest_filter=MefLightmapUv.*`
Expected: `[ PASSED ]` (or `SKIPPED` if the corpus lacks a modelType==3 `.mef`, consistent with existing corpus-driven tests).

- [ ] **Step 6: Commit**

```bash
git add source/parsers/mef_native.h source/parsers/mef_native.cpp tests/test_mef_lightmap_uv.cpp tests/CMakeLists.txt
git commit -m "feat: parse lightmap UV2 channel for MEF modelType 3"
```

---

### Task 5: GUI "Apply Lightmap" right-click + shader blend

**Files:**
- Modify: `igi1conv/gui_main.cpp` (context menu around line 6159-6166; `ModelViewer` class starting line 139; shader source near `QOpenGLShaderProgram program;` at line 1750; `loadModel` at line 1051; `paintGL` at line 1498)

**Interfaces:**
- Consumes: `igi1conv::ResolveLightmapFiles(objectsQscPath, mefStem)` (Task 3), `OLMFile ParseOlm(const std::string& path)` (existing, `cmd_olm.h`, already included in `gui_main.cpp`), `geometry.vertices[i].uv2` (Task 4), existing `globalObjectsQscPath` member (already declared at line 4360).
- Produces: nothing consumed by later tasks (final GUI integration task).

- [ ] **Step 1: Add a lightmap GL texture + uv2 vertex attribute to ModelViewer**

In `igi1conv/gui_main.cpp`, find the `ModelViewer` class's private members (search for `QOpenGLShaderProgram program;` at line 1750) and add directly below it:

```cpp
    GLuint lightmapTexture = 0;
    bool   hasLightmap = false;
```

Find `loadModel(const QString& path)` (line 1051) and locate where it resets per-model GL state at the top of the function (look for any existing texture cleanup, e.g. where the diffuse texture is deleted/reset before loading new geometry). Add right after that existing cleanup:

```cpp
        if (hasLightmap && lightmapTexture) {
            glDeleteTextures(1, &lightmapTexture);
            lightmapTexture = 0;
        }
        hasLightmap = false;
```

- [ ] **Step 2: Add `applyLightmapOnModel` method**

Add this method to the GUI's main window class (the same class that owns `applyIffOnModel`, right after the closing brace of `applyIffOnModel` at line 4804):

```cpp
    // Right-click "Apply Lightmap" on a .mef: resolve its bound .olm
    // file(s) via objects.qsc and upload them as a lightmap texture in
    // the viewer.  Silently no-ops (with a log line) if no binding or
    // files are found - this must never break loading a plain model.
    void applyLightmapOnModel(const QString& mefPath) {
        if (globalObjectsQscPath.isEmpty() || !QFile::exists(globalObjectsQscPath)) {
            logMessage("[WARN] Apply Lightmap: no objects.qsc set (Settings > Animation > Set Objects.qsc...)");
            return;
        }
        std::string mefStem = QFileInfo(mefPath).completeBaseName().toStdString();
        std::vector<std::string> olmFiles =
            igi1conv::ResolveLightmapFiles(globalObjectsQscPath.toStdString(), mefStem);

        if (olmFiles.empty()) {
            logMessage("[INFO] Apply Lightmap: no lightmap binding found for " + QFileInfo(mefPath).fileName());
            return;
        }

        OLMFile olm = ParseOlm(olmFiles.front());
        if (!olm.valid || olm.pixels.empty()) {
            logMessage("[WARN] Apply Lightmap: failed to parse " + QString::fromStdString(olmFiles.front()));
            return;
        }

        if (modelViewer) {
            modelViewer->loadModel(mefPath);
            modelViewer->setLightmap(olm.pixels, olm.layer.pixel_width, olm.layer.pixel_height);
            modelViewer->show();
        }
        logMessage("[INFO] Apply Lightmap: " + QFileInfo(mefPath).fileName() +
            " <- " + QString::number((int)olmFiles.size()) + " lightmap file(s), using " +
            QFileInfo(QString::fromStdString(olmFiles.front())).fileName());
    }
```

Add `#include "lightmap_resolver.h"` near the other includes at the top of `gui_main.cpp` (next to `#include "cmd_olm.h"` at line 94).

- [ ] **Step 3: Add `setLightmap` to ModelViewer**

Add this public method to the `ModelViewer` class, near `loadModel` (line 1051):

```cpp
    void setLightmap(const std::vector<OlmPixel>& pixels, uint16_t width, uint16_t height) {
        if (hasLightmap && lightmapTexture) {
            glDeleteTextures(1, &lightmapTexture);
            lightmapTexture = 0;
        }
        if (pixels.empty() || width == 0 || height == 0) { hasLightmap = false; return; }

        // OLM stores RGBA already (cmd_olm.cpp only swaps channels for the
        // TGA/PNG exporters, not here) - upload as-is.
        glGenTextures(1, &lightmapTexture);
        glBindTexture(GL_TEXTURE_2D, lightmapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        hasLightmap = true;
        update();
    }
```

- [ ] **Step 4: Extend the vertex buffer layout and shader for uv2**

Find where vertex buffers are built from `geometry.vertices` in `loadModel` (search for `.uv` usage that packs interleaved vertex data, e.g. a loop writing `pos`, `normal`, `uv` into a `QVector<float>` or similar before `glBufferData`). Add `v.uv2.x, v.uv2.y` immediately after the existing `v.uv.x, v.uv.y` write in that same loop, so the interleaved stride grows by 2 floats.

Find the vertex shader source (search for `attribute vec2 aUV` or `layout(location` near the `program.addShaderFromSourceCode` calls) and add a second UV attribute:

```glsl
attribute vec2 aUV2;
varying vec2 vUV2;
```
with `vUV2 = aUV2;` added next to the existing `vUV = aUV;` line, and the matching `program.setAttributeBuffer(...)`/`enableAttributeArray` call added in `paintGL` (line 1498) right after the existing `aUV` attribute setup, using the same VBO with the new stride and an offset of `(3+3+2)*sizeof(float)` floats in.

Find the fragment shader source (same area) and change the final color computation from sampling only the diffuse texture to:

```glsl
uniform sampler2D uLightmap;
uniform bool uHasLightmap;
varying vec2 vUV2;
...
vec4 diffuseColor = texture2D(uTexture, vUV);
vec4 finalColor = diffuseColor;
if (uHasLightmap) {
    vec4 lightmapColor = texture2D(uLightmap, vUV2);
    finalColor = vec4(diffuseColor.rgb * lightmapColor.rgb, diffuseColor.a);
}
gl_FragColor = finalColor;
```

In `paintGL`, right before the existing diffuse-texture bind/draw call, add:

```cpp
        program.setUniformValue("uHasLightmap", hasLightmap);
        if (hasLightmap) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, lightmapTexture);
            program.setUniformValue("uLightmap", 1);
            glActiveTexture(GL_TEXTURE0);
        }
```

- [ ] **Step 5: Add the right-click menu entry**

In `igi1conv/gui_main.cpp`, find the `.mef` context menu block (search for the `exportMenu` additions around line 5976, inside the same `if` block that handles `.mef`/`.MEF` files). Add, right after the existing menu actions for that file type:

```cpp
            menu.addAction("Apply Lightmap", [this, path]() {
                applyLightmapOnModel(path);
            });
```

- [ ] **Step 6: Build the GUI target**

Run: `cmake --build build --config Release --target igi1conv`
Expected: build succeeds with no new warnings about unused `aUV2`/`uLightmap`.

- [ ] **Step 7: Manual verification**

Launch `bin/Release/igi1conv.exe`, open the file browser to `D:\IGI1\missions\location0\level1\models` (or wherever the level's `.mef` files live), set Settings > Animation > Objects.qsc to that level's decompiled `objects.qsc`, right-click a `.mef` known to be referenced by a `Building` task with a `LightmapInfo` child, choose "Apply Lightmap", and confirm:
- The status log shows `[INFO] Apply Lightmap: <file> <- N lightmap file(s)...`
- The model in the 3D view shows visibly different shading than before applying (lightmap multiply effect), OR if no binding exists for the chosen file, the log shows `[INFO] Apply Lightmap: no lightmap binding found...` and the model still loads normally.

- [ ] **Step 8: Commit**

```bash
git add igi1conv/gui_main.cpp
git commit -m "feat: add Apply Lightmap right-click on .mef with shader blend"
```

---

### Task 6: Full Release build and final test pass

**Files:** none (build verification only)

- [ ] **Step 1: Full clean Release build**

Run: `cmake --build build --config Release`
Expected: `0 Error(s)` in the MSBuild summary.

- [ ] **Step 2: Run the full test suite against the real corpus**

Run: `bin/Release/igi1conv_tests.exe --game-path="D:\IGI1"`
Expected: no new failures versus the pre-existing baseline (some tests may `SKIP` depending on corpus contents — that's expected and not a failure).

- [ ] **Step 3: Confirm no changes were pushed**

Run: `git log --oneline -8` and `git status`
Expected: `git status` shows a clean working tree relative to local commits; `git log` shows the 5 commits from Tasks 1-5 plus this task has no commit of its own (build-only, nothing to commit).
