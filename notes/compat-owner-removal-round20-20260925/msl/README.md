# Round20 MSL formatted-string owner

The existing implementation now belongs to Aurora's MSL C runtime: `lib/MSL_C/printf.cpp`. Deleted `src/compat/MetrowerksPrintf.hpp` and `MslPrintfCompat.{cpp,hpp}`. No compatibility service, namespace, old path, or old exported symbol remains in this lane.

`include/MSL_C/stdio_api.h` declares concrete C-linkage `__msl_sprintf`, `__msl_snprintf`, `__msl_vsprintf`, and `__msl_vsnprintf`. `include/MSL_C/stdio.h` supplies the original-client standard stdio declarations with compiler assembler labels, then includes native stdio. It supports C and C++ callers, preserves the Darwin leading symbol prefix, and matches Linux C++ stdio's noexcept declarations. This redirecting header must precede other stdio declarations. It does not redirect FILE/stdout logging.

The provider includes only declaration-only `stdio_api.h` and native `<cstdio>`. Its numeric/extension calls remain actual host `std::snprintf`/`std::vsnprintf`, so it cannot alias them to its own implementation. The independent `aurora-msl` Xmake / `aurora::msl` CMake target has no Game dependency or forced include. Common drops the previous local provider and publicly links aurora-msl. Game drops the obsolete provider exclusion and comment. The prefix-removal lane owns Game/test forced includes and deletes MetrowerksStdCompat.hpp; it has the same header contract.

All previous implemented behavior is retained: null strings, Pascal strings and embedded NULs, byte precision and original padding/sign handling, low-byte wide-string conversion, bounded writes and return length, `%n`, native pointer/long/size/ptrdiff vararg widths, and whole-call fallback for native positional/locale extensions. No formatter algorithm change was introduced.

The two existing formatter test sources change only direct include paths and exported function names. Assertions are untouched; no fixture, test target, or new coverage was added. No builds, tests, or git operations were performed. `source-validation.txt` records source-only equality after the intended export/include changes and Aurora clang-format.

`before/`, `after/`, hashes in `owned-manifest.json`, and `lane-only.patch` preserve this lane's exact scope. Game/xmake.lua is shared with the prefix-removal lane; its saved after image deliberately applies only the formatter provider exclusion/comment removal to this lane's before snapshot. The separate Aurora/root manifests identify submodule publication boundaries.
