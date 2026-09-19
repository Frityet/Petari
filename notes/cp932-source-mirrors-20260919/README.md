# Explicit CP932 source boundary verification — 2026-09-19

The user explicitly allowed Game literal wrappers while asking to remove the build-time literal preprocessor. Seven native source-mirror suites now use tests/SourceMirrorEncoding.hpp. It removes only one exact leading `#include "compat/Cp932Literal.hpp"` line and CP932 wrappers whose arguments consist solely of ordinary string-literal tokens. All other source bytes (including literal spelling, comments and surrounding whitespace) must remain identical. Existing narrowly documented base-virtual/GCC attribute/retail-branch allowances stay unchanged.

A lexical scan keeps comments, raw/wide/character literals and user-defined literal suffixes opaque. Arbitrary expressions, nested/malformed wrappers and extra includes do not normalize away. tests/SourceMirrorEncodingTests.cpp and its Lua counterpart exercise 31 positive/negative cases. The independent Lua helper scripts/source_mirror_encoding.lua supplies the same narrow transformation to the source-closeness audit; encoding-only differences are classified `compile-only`, never `exact-source`.

## Evidence

- Direct Clang20-mode compilation with `-Wall -Wextra -Werror -pedantic` passes, with no Game/engine dependency. No native Xmake build was run by this agent.
- Both C++ and Lua: all31 adversarial boundary cases pass.
- Both independent normalizers recover all330 migration snapshot files at notes/explicit-game-encoding-20260919/before/src/Game byte-for-byte. This checks all3580 wrapper insertions against pre-migration native files, not merely selected donors.
- Full read-only source audit runs over1678 files:1154 exact-source,242 compile-only,275 compat-temporary,7 decomp-needed. All237 migrated files that previously matched their decomp donors exactly are recognized as explicit-encoding-only. The other existing differences are still reported.
- GameSourceMirrorTests and PlayerSourceMirrorTests compile/run but do not pass overall:14 Game mismatches and113 Player mismatches/stale source-exclusion assertions predate migration. Their complete pre/post migration output is byte-identical (mirror-baseline-comparison.json); none was bypassed. See game-source-mirror*.log and player-source-mirror*.log for the exact list. These focused changes do not claim to repair those old suites or prove gameplay.

## Changed files

New strict C++ helper/unit test, Lua helper/unit test; seven equality suites (GameSourceMirror,PlayerSourceMirror,AreaObjCore,AreaObjRealOrAbsent,SaveConfigRealOrAbsent,StorySequenceRealOrAbsent,LodCtrlCompat); scripts/source_closeness_audit.lua; a final standalone tests/xmake.lua target `smg-pc-source-mirror-encoding-tests`. Earlier hunks in tests/xmake.lua belong to other work and were left intact.
