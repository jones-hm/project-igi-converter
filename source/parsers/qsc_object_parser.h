#pragma once
//
// qsc_object_parser.h
//
// Reverse-engineered parser for IGI 1's decompiled `objects.qsc` (the
// output of `qvm decompile objects.qvm`) that extracts every
// `Task_New("HumanSoldier", "<name>", X, Y, Z, Gamma, "<model_id>",
// Team, BoneHeirachy, StandAnimation)` triple.
//
// The schema is fixed by the engine's
// Task_DeclareParameters("HumanSoldier", ...):
//
//   index 0  : Task ID (always 4xx in real data)  - skipped
//   index 1  : "HumanSoldier" class name           - skipped
//   index 2  : object name (free-form string)      - captured as `name`
//   index 3  : Position X (float, IGI world units) - captured as `pos`
//   index 4  : Position Y
//   index 5  : Position Z
//   index 6  : Gamma / yaw (radians)               - captured as `gamma`
//   index 7  : Model ID (e.g. "013_01_1")          - captured as `modelId`
//   index 8  : Team (1..N)
//   index 9  : Bone Hierarchy (file stem of common/ANIMS/<n>.IFF)
//                                                  - captured as `boneHierarchy`
//   index 10 : Stand Animation (clip id within that IFF)
//                                                  - captured as `standAnimation`
//
// The parser is intentionally robust to the IFF numeric / float /
// string formatting quirks of the decompiler: floats use '.17g' style
// (e.g. "24474926.0"), strings can contain spaces and embedded digits,
// and a handful of Task_New calls are spread across newlines.
//
// Usage:
//   QscObjectSet set = QscObjectSet::parse(qscText);
//   for (const auto& e : set.entries) { ... }
//
// The parser is decoupled from any GUI code and lives in source/parsers
// so it can also be reused by tests and the CLI.

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace igi1conv {

struct HumanSoldierEntry {
    std::string name;             // object display name (may be empty)
    std::string modelId;          // e.g. "013_01_1"
    double      posX = 0.0;       // raw IGI world units
    double      posY = 0.0;
    double      posZ = 0.0;
    double      gamma = 0.0;      // radians
    int32_t     team = 0;
    int32_t     boneHierarchy = -1;  // file stem of ANIMS/<n>.IFF; -1 = unknown
    int32_t     standAnimation = -1; // clip id within the IFF;  -1 = unknown
};

struct QscObjectSet {
    std::vector<HumanSoldierEntry> entries;

    // Group entries by modelId for fast lookup in the GUI.
    // The map is rebuilt by groupByModel() and is not populated by parse().
    std::vector<std::string> modelIds() const;
    std::vector<const HumanSoldierEntry*> entriesForModel(const std::string& modelId) const;

    // Returns the union of (boneHierarchy, standAnimation) pairs for
    // a given modelId, sorted ascending by standAnimation.  This is
    // the list that the Animation panel renders in its "Animations"
    // list box.
    struct AnimRef { int32_t boneHierarchy; int32_t standAnimation; };
    std::vector<AnimRef> animationsForModel(const std::string& modelId) const;

    // Parse the decompiled QSC text.  Returns an empty set on error.
    static QscObjectSet parse(const std::string& qscText,
                              std::string* err = nullptr);
};

// Maps a model id (any quoted string argument found anywhere inside a
// Task_New(...) tree, including nested child Task_New calls) to the
// logical lightmap id captured by a nested Task_New("LightmapInfo", ...,
// "<logical_id>") call in that SAME tree.  Used to resolve which
// obj00000_*.olm files belong to a given .mef.
struct LightmapBindingSet {
    // modelId -> logical lightmap id (e.g. "435_01_1" -> "obj00000")
    std::vector<std::pair<std::string, std::string>> bindings;

    std::optional<std::string> logicalIdForModel(const std::string& modelId) const;

    static LightmapBindingSet parse(const std::string& qscText,
                                     std::string* err = nullptr);
};

} // namespace igi1conv
