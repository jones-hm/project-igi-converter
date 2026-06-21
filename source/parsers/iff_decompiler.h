#pragma once
//
// iff_decompiler.h
//
// Native C++ decompiler that splits an IGI 1 IFF binary into a human
// readable .IFF text file (skeleton + animation list) plus a folder
// `anims_<id>/` containing one small per-animation .IFF text file per
// clip.
//
// This is the inverse of the writer in iff_writer.h; the on-disk layout
// mirrors what the reference `IGI1_iff.py` + `reader_anims_data.py` from
// the dconv toolchain produced, so the round trip IFF -> decompile ->
// re-create -> IFF yields an identical binary (modulo IFF chunk-size
// reconciliation that the C++ writer handles).
//
// IFF_Decompile(src.iff, out_dir) writes:
//   out_dir/<basename>.IFF         // skeleton + anim list (text)
//   out_dir/anims_<id>/anim_NNN.IFF // per-clip text file
//
// where <id> is the bare stem of the source file (e.g. "003" for
// "003.IFF") and NNN is the per-clip animation id.
//
// Returns true on success.  On failure, `err` (if non-null) is set to
// a human-readable message and the partial output is left in place.

#include <string>

namespace igi1conv {

bool IFF_Decompile(const std::string& srcIffPath,
                   const std::string& outDir,
                   std::string* err = nullptr);

} // namespace igi1conv
