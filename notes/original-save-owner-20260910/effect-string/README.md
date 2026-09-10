# Effect attribute base-name recovery — 2026-09-10

The live walking replay crashed in `MR::extractString` with `num=4294967288`, reached from `EffectKeeper::syncEffectBck` and `makeAttibuteEffectBaseName("RunSmokeAttrWater")`. See the parent directory's `crash-backtrace.log`, frames around line 660. The erroneous reference subtraction returned -8 as an unsigned count, leading to the observed enormous `strncpy`.

Only `decomp/src/Game/LiveActor/EffectKeeper.cpp` and its native mirror were edited. The guide was read before recovery, and reference compilation preceded the native copy. The corrected function is byte-identical in both source files. Surrounding native debugging and architecture adjustments were preserved.

Retail calls `strlen(tag)` first (0x801621FC), retains it in r31, calls `strlen(pSrc)` (0x80162208), then executes `subf r5,r31,r3` (0x8016220C). Therefore the intended count is the complete source length minus the tag suffix length. The recovered local `u32 tagLength` preserves that evaluation order as well as the exact subtraction. `RunSmokeAttrWater` now yields `RunSmoke`, with count 8.

The retail `MR::extractString` at 0x803FEAD4 genuinely ignores its fourth argument. It calls `strncpy(dst,src,num)` and writes `dst[num]=0`. The actual native provider in `src/compat/XanimeQueryCompat.cpp` already agrees. No capacity clamp, prefix truncation policy, or other StringUtil change was made.

Evidence:

- `retail-byte-proof.json` verifies every instruction byte of both functions against the actual original DOL, SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`. Small instruction listings are saved alongside it.
- `wii-results.json`: full EffectKeeper translation unit compilation passed for baseline and final recovery. Exact helper objdiff improves from **99.67742% to 100.0%**, with the final retail size 124 bytes. The baseline's high score concealed a real subtraction-operand error. An intermediate direct expression was semantically correct at 91.77419%; the final explicit local also preserves retail call order and matches completely.
- `native-compile.json`: the complete native EffectKeeper translation unit compiles with LLVM 23, exit 0.
- `exact-helper-results.json`: ASan/UBSan compile and run both exit 0; six cases exercise the observed input, absent tag, tag at the start, empty suffix, repeated tags, and retail's ignored capacity argument using an ample real destination. This harness extracts the actual helper and actual string-copy function bodies byte-identically; it is explicitly not a fully linked EffectKeeper fixture or gameplay proof.
- `source-manifest.json` records both edited source hashes and the unchanged native string provider hash.

Production is frozen. No Xmake build or commit was performed by this subtask; root owns integrated replay validation and publication.
