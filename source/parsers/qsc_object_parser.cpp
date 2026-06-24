#include "qsc_object_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace igi1conv {

namespace {

// ─── Small CSV / tokeniser ───────────────────────────────────────────────
//
// The "Task_New(...)" call in the decompiled QSC uses comma-separated
// arguments.  String literals can contain commas, parentheses, dots,
// digits, and spaces but cannot contain a raw '"' (the engine escapes
// them via backslashes).  The tokeniser respects string boundaries and
// nested parentheses, which is the only context where commas survive
// inside a string literal.  It returns a flat list of tokens with
// strings still wrapped in their "..." quotes so the caller can
// inspect the raw value.

enum class Tk {
    Number,   // integer or float literal, lexeme = the literal text
    String,   // "..." (quotes kept)
    Bad,      // not parseable
    End
};

struct Token {
    Tk kind;
    std::string text;
};

static bool isNumberStart(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+' || c == '.';
}

static bool isNumberCont(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == 'e' || c == 'E'
           || c == '-' || c == '+' || c == 'x' || c == 'X'
           || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Tokenise a parenthesised argument list `"(a, b, c)"` into a vector
// of {kind, text}.  The leading '(' and trailing ')' are NOT included.
static std::vector<Token> tokenizeArgs(const std::string& src, size_t& pos, std::string* err) {
    std::vector<Token> out;
    // Hard cap so a malformed file can't blow up memory.  The deepest
    // IGI1 calls have ~200 args (AnimTask).  Anything beyond 1024 is
    // almost certainly a parse error or a file we don't understand.
    constexpr size_t kMaxTokens = 1024;
    // skip '('
    if (pos >= src.size() || src[pos] != '(') {
        if (err) *err = "expected '(' at offset " + std::to_string(pos);
        return out;
    }
    ++pos;
    while (pos < src.size()) {
        if (out.size() >= kMaxTokens) {
            if (err) *err = "Task_New arg list exceeded " + std::to_string(kMaxTokens) + " tokens";
            return out;
        }
        // skip whitespace
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) ++pos;
        if (pos >= src.size()) break;
        if (src[pos] == ')') { ++pos; return out; }
        if (src[pos] == ',') { ++pos; continue; }

        // string literal
        if (src[pos] == '"') {
            size_t start = pos;
            ++pos;
            while (pos < src.size() && src[pos] != '"') {
                if (src[pos] == '\\' && pos + 1 < src.size()) ++pos; // skip escape
                ++pos;
            }
            if (pos >= src.size()) {
                if (err) *err = "unterminated string starting at offset " + std::to_string(start);
                return out;
            }
            ++pos; // consume closing "
            out.push_back({Tk::String, src.substr(start, pos - start)});
            continue;
        }

        // number literal
        if (isNumberStart(src[pos])) {
            size_t start = pos;
            ++pos;
            while (pos < src.size() && isNumberCont(src[pos])) ++pos;
            out.push_back({Tk::Number, src.substr(start, pos - start)});
            continue;
        }

        // nested parentheses: a Task_New call can be a sub-expression
        // of another Task_New (e.g. AIGraph wrapping HumanSoldier).
        // Consume the whole balanced paren group as one Bad token so
        // the outer arg count doesn't blow up on inner calls.
        if (src[pos] == '(') {
            size_t start = pos;
            int depth = 1;
            ++pos;
            while (pos < src.size() && depth > 0) {
                if (src[pos] == '(') ++depth;
                else if (src[pos] == ')') --depth;
                // Honour string boundaries so a ')' inside a string
                // doesn't decrement the depth.
                else if (src[pos] == '"') {
                    ++pos;
                    while (pos < src.size() && src[pos] != '"') {
                        if (src[pos] == '\\' && pos + 1 < src.size()) ++pos;
                        ++pos;
                    }
                }
                ++pos;
            }
            out.push_back({Tk::Bad, src.substr(start, pos - start)});
            continue;
        }

        // identifier / unknown -- consume as a single token
        size_t start = pos;
        while (pos < src.size()
               && !std::isspace(static_cast<unsigned char>(src[pos]))
               && src[pos] != ',' && src[pos] != ')' && src[pos] != '(') {
            ++pos;
        }
        out.push_back({Tk::Bad, src.substr(start, pos - start)});
    }
    if (err) *err = "missing ')' in argument list";
    return out;
}

static bool parseInt(const std::string& s, int32_t& out) {
    if (s.empty()) return false;
    try {
        size_t p = 0;
        long long v = std::stoll(s, &p);
        if (p != s.size()) return false;
        out = static_cast<int32_t>(v);
        return true;
    } catch (...) { return false; }
}

static bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    try {
        size_t p = 0;
        double v = std::stod(s, &p);
        if (p != s.size()) return false;
        out = v;
        return true;
    } catch (...) { return false; }
}

static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

} // namespace

QscObjectSet QscObjectSet::parse(const std::string& qscText, std::string* err) {
    QscObjectSet set;

    // Hard upper bound on how many Task_New occurrences we'll walk
    // through - protects against pathological inputs.
    constexpr size_t kMaxCalls = 50000;
    // Walk through the source linearly.  Whenever we see
    // `Task_New(` followed later by `"HumanSoldier"` and a closing `)`,
    // tokenise the arg list and pluck the fields by index.
    const std::string needle = "Task_New";
    size_t searchFrom = 0;
    size_t callsScanned = 0;
    while (true) {
        if (callsScanned++ > kMaxCalls) {
            if (err) *err = "Task_New scan exceeded " + std::to_string(kMaxCalls) + " occurrences";
            break;
        }
        size_t p = qscText.find(needle, searchFrom);
        if (p == std::string::npos) break;
        // require word boundary before the call name
        bool leftOk = (p == 0)
                      || (!std::isalnum(static_cast<unsigned char>(qscText[p - 1]))
                          && qscText[p - 1] != '_');
        if (!leftOk) { searchFrom = p + needle.size(); continue; }
        size_t parenPos = p + needle.size();
        // skip whitespace
        while (parenPos < qscText.size()
               && std::isspace(static_cast<unsigned char>(qscText[parenPos]))) ++parenPos;
        if (parenPos >= qscText.size() || qscText[parenPos] != '(') {
            searchFrom = p + needle.size();
            continue;
        }

        std::string localErr;
        size_t pos = parenPos;
        auto tokens = tokenizeArgs(qscText, pos, &localErr);
        if (!localErr.empty()) {
            // The argument list wasn't terminated before EOF or hit
            // the safety cap.  Either way, advance search from just
            // past the current "Task_New" keyword so that nested
            // Task_New calls (which the cap'd tokenizer may have
            // skipped) are still picked up by the next find().
            searchFrom = p + needle.size();
            continue;
        }

        // Layout from the engine's Task_DeclareParameters("HumanSoldier"):
        //   [0]  Task ID (int)
        //   [1]  Class name "HumanSoldier"   <-- the trigger token
        //   [2]  Object name (string)
        //   [3]  Position X (float)
        //   [4]  Position Y
        //   [5]  Position Z
        //   [6]  Gamma (float, radians)
        //   [7]  Model id (string)
        //   [8]  Team (int)
        //   [9]  Bone Hierarchy (int)
        //   [10] Stand Animation (int)
        //
        // We require [1] to be the string "HumanSoldier" - this is what
        // makes the call unique, since the file may have other Task_New
        // calls (e.g. doors, lights, triggers) right next to it.
        if (tokens.size() >= 11
            && tokens[1].kind == Tk::String
            && stripQuotes(tokens[1].text) == "HumanSoldier") {

            HumanSoldierEntry e;
            if (tokens[2].kind == Tk::String) e.name = stripQuotes(tokens[2].text);
            double x=0, y=0, z=0, g=0; int32_t team=0, bh=-1, sa=-1;
            if (parseDouble(tokens[3].text, x)) e.posX = x;
            if (parseDouble(tokens[4].text, y)) e.posY = y;
            if (parseDouble(tokens[5].text, z)) e.posZ = z;
            if (parseDouble(tokens[6].text, g)) e.gamma = g;
            if (tokens[7].kind == Tk::String) e.modelId = stripQuotes(tokens[7].text);
            if (parseInt(tokens[8].text, team)) e.team = team;
            if (parseInt(tokens[9].text, bh))   e.boneHierarchy = bh;
            if (parseInt(tokens[10].text, sa))  e.standAnimation = sa;
            // Some entries use the integer -1 to mark "no animation" -
            // keep them so the GUI can show "(no anim)" for that model
            // but never try to play a -1 clip id.
            set.entries.push_back(std::move(e));
        }
        // Continue scanning from just past the "Task_New" keyword of
        // THIS call - not from after the closing ')'.  Skipping past
        // the whole call would miss nested HumanSoldier Task_New
        // calls buried inside the current call's arg list (e.g. an
        // AIGraph / ConditionalContainer that wraps HumanSoldier).
        // Setting searchFrom = p + needle.size() lets the next find()
        // pick up the inner Task_New occurrences.
        searchFrom = p + needle.size();
    }

    if (err) *err = "";
    return set;
}

std::vector<std::string> QscObjectSet::modelIds() const {
    std::set<std::string> uniq;
    for (const auto& e : entries) {
        if (!e.modelId.empty()) uniq.insert(e.modelId);
    }
    return std::vector<std::string>(uniq.begin(), uniq.end());
}

std::vector<const HumanSoldierEntry*>
QscObjectSet::entriesForModel(const std::string& modelId) const {
    std::vector<const HumanSoldierEntry*> out;
    for (const auto& e : entries) {
        if (e.modelId == modelId) out.push_back(&e);
    }
    return out;
}

std::vector<QscObjectSet::AnimRef>
QscObjectSet::animationsForModel(const std::string& modelId) const {
    std::set<std::pair<int32_t, int32_t>> uniq;
    for (const auto& e : entries) {
        if (e.modelId == modelId && e.boneHierarchy >= 0 && e.standAnimation >= 0) {
            uniq.insert({e.boneHierarchy, e.standAnimation});
        }
    }
    std::vector<AnimRef> out;
    out.reserve(uniq.size());
    for (const auto& p : uniq) out.push_back({p.first, p.second});
    return out;
}

// ─── LightmapBindingSet ──────────────────────────────────────────────────────
//
// Unlike QscObjectSet::parse (which only looks at one fixed Task_New schema),
// this walks the FULL nested tree of each top-level Task_New(...) call to
// find: (a) every quoted string anywhere in the tree (candidate model ids),
// and (b) a nested Task_New("LightmapInfo", ..., "<logical_id>") call. Every
// model id found in a tree that also contains a LightmapInfo child is bound
// to that tree's logical lightmap id.

namespace {

struct TaskCall { size_t openParen; size_t closeParen; };

// Locate the matching ')' for the Task_New call whose "Task_New" keyword
// starts at `taskNewPos`. Honours string boundaries and nested parens.
bool FindCallSpan(const std::string& s, size_t taskNewPos, TaskCall& out) {
    size_t p = taskNewPos + 8; // strlen("Task_New")
    while (p < s.size() && std::isspace(static_cast<unsigned char>(s[p]))) ++p;
    if (p >= s.size() || s[p] != '(') return false;
    out.openParen = p;
    size_t depth = 1, i = p + 1;
    while (i < s.size() && depth > 0) {
        if (s[i] == '"') {
            ++i;
            while (i < s.size() && s[i] != '"') {
                if (s[i] == '\\' && i + 1 < s.size()) ++i;
                ++i;
            }
        } else if (s[i] == '(') {
            ++depth;
        } else if (s[i] == ')') {
            --depth;
            if (depth == 0) { out.closeParen = i; return true; }
        }
        ++i;
    }
    return false;
}

// Quoted strings that are direct (non-nested) arguments of the call
// spanning [openParen, closeParen].
std::vector<std::string> ExtractDirectStrings(const std::string& s, size_t openParen, size_t closeParen) {
    std::vector<std::string> out;
    size_t j = openParen + 1;
    int depth = 0;
    while (j < closeParen) {
        if (s[j] == '(') { ++depth; ++j; continue; }
        if (s[j] == ')') { --depth; ++j; continue; }
        if (depth == 0 && s[j] == '"') {
            size_t st = j;
            ++j;
            while (j < closeParen && s[j] != '"') {
                if (s[j] == '\\' && j + 1 < closeParen) ++j;
                ++j;
            }
            out.push_back(s.substr(st + 1, j - st - 1));
            ++j;
            continue;
        }
        ++j;
    }
    return out;
}

// Recursively walk every Task_New(...) call within [from, to), collecting
// candidate model ids into `modelIds` and the first LightmapInfo logical id
// found into `lightmapId`.
void CollectModelIdsAndLightmapId(const std::string& s, size_t from, size_t to,
                                   std::vector<std::string>& modelIds,
                                   std::string& lightmapId) {
    size_t i = from;
    while (i < to) {
        if (i + 8 <= to && s.compare(i, 8, "Task_New") == 0 &&
            (i == 0 || (!std::isalnum(static_cast<unsigned char>(s[i - 1])) && s[i - 1] != '_'))) {
            TaskCall call;
            if (FindCallSpan(s, i, call) && call.closeParen < to) {
                auto direct = ExtractDirectStrings(s, call.openParen, call.closeParen);
                bool isLightmapInfo = std::find(direct.begin(), direct.end(), "LightmapInfo") != direct.end();
                if (isLightmapInfo) {
                    if (lightmapId.empty() && !direct.empty()) lightmapId = direct.back();
                } else {
                    for (auto& d : direct) modelIds.push_back(d);
                    CollectModelIdsAndLightmapId(s, call.openParen + 1, call.closeParen, modelIds, lightmapId);
                }
                i = call.closeParen + 1;
                continue;
            }
        }
        ++i;
    }
}

} // namespace

std::optional<std::string>
LightmapBindingSet::logicalIdForModel(const std::string& modelId) const {
    for (const auto& [model, logicalId] : bindings) {
        if (model == modelId) return logicalId;
    }
    return std::nullopt;
}

LightmapBindingSet LightmapBindingSet::parse(const std::string& qscText, std::string* err) {
    LightmapBindingSet set;
    constexpr size_t kMaxCalls = 50000;
    size_t scanned = 0;
    size_t i = 0;
    while (i < qscText.size()) {
        if (scanned++ > kMaxCalls) {
            if (err) *err = "Task_New scan exceeded " + std::to_string(kMaxCalls) + " occurrences";
            break;
        }
        size_t p = qscText.find("Task_New", i);
        if (p == std::string::npos) break;
        bool leftOk = (p == 0)
                      || (!std::isalnum(static_cast<unsigned char>(qscText[p - 1]))
                          && qscText[p - 1] != '_');
        if (!leftOk) { i = p + 8; continue; }

        TaskCall call;
        if (!FindCallSpan(qscText, p, call)) { i = p + 8; continue; }

        auto rootDirect = ExtractDirectStrings(qscText, call.openParen, call.closeParen);
        bool rootIsLightmapInfo = std::find(rootDirect.begin(), rootDirect.end(), "LightmapInfo") != rootDirect.end();

        std::vector<std::string> modelIds;
        std::string lightmapId;
        if (rootIsLightmapInfo) {
            if (!rootDirect.empty()) lightmapId = rootDirect.back();
        } else {
            modelIds = rootDirect;
        }
        CollectModelIdsAndLightmapId(qscText, call.openParen + 1, call.closeParen, modelIds, lightmapId);

        if (!lightmapId.empty()) {
            for (auto& m : modelIds) set.bindings.push_back({m, lightmapId});
        }
        i = call.closeParen + 1;
    }

    if (err) *err = "";
    return set;
}

} // namespace igi1conv
