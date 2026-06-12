#pragma once
#include <string>

namespace MefCompiler {

// Compile a text-based MEF (ASCII) file to a binary ILFF/MEF file (Type 0 rigid).
// Returns true on success, false on any parse or write error.
bool Compile(const std::string& textMefPath, const std::string& outBinaryPath);

} // namespace MefCompiler
