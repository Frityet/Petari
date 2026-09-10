# Original GamePadUtil completion — 2026-09-10

The reference had three explicit holes: `MR::getPlayerStickX`, `getPlayerStickY`, and the scalar overload of `calcWorldStickDirectionXZ`. They were recovered in `decomp/src/Game/Util/GamePadUtil.cpp` following the already-read decompilation guide, compiled for Wii against retail, then the complete source was copied identically into `src/Game/Util/GamePadUtil.cpp`. The existing reference/native headers were already identical and required no changes. No GamePadUtilCompat removal, Xmake edit, commit, or global build was performed; parent owns activation.

## Retail behavior

- Player X/Y getters sample channel-zero's processed original WPadStick component, return it when nonzero, otherwise use a zero-initialized fallback. Retail still contains a zero-versus-zero check and unused trig-shaped fallback arithmetic. The recovery preserves these branches and their positive-zero return rather than replacing the functions with simple forwards. No unsupported controller or motion behavior was invented from the otherwise unreachable code.
- World XZ direction copies the inverse camera view matrix once, extracts and separately normalizes horizontal X/Z axes, negates the Z direction, then samples processed WPadStick X and Y in order. It scales the two axes, adds and normalizes the resulting direction, and writes X then Z. No raw Aurora stick lookup remains in this original function.

## Evidence

- `retail-byte-proof.json` checks all instruction bytes of the three recovered functions against the original DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`; compact retail listings are saved alongside it.
- `wii-results.json` contains baseline and recovered complete-TU compiler commands/results, all exit 0. Baseline objdiff had the three symbols absent. Final `objdiff-matrix-set.json` contains all **63** original MR/WPadFunction API symbols.
- Final match scores: X **99.53704%**, Y **99.55556%**, scalar world-direction **90.0641%**. The X/Y instruction sequences differ only in compiler-generated constant relocation labels. The world-direction control/data flow agrees; its remaining difference is inline TVec3 copy/add versus retail calls, plus constant labels. Its final candidate size is 328 bytes versus retail 312. An earlier semantically equivalent aggregate matrix copy emitted an integer loop; explicit original matrix `.set()` recovered the retail paired-single copy.
- `native-compile.json`: complete native TU compile exit 0 using LLVM 23. `source-manifest.json` records byte-identical source hashes and unchanged matching headers.

This is source/compile/retail-comparison evidence, not a newly linked input-owner or gameplay test. Parent will run the original-owner integration fixtures after companion WPad providers land.

## Link and retirement closure

The native object requires the original WPadButton button/trigger/release accessors (26 getters), WPadAcceleration::getAcceleration/isBalanced, existing WPadPointer getters, WPad::getRumbleInstance, MR::getWPad/getCameraInvViewMtx/normalizeOrZero, and JMath tables/atan helper. The current record-only WPadButton provider has constructor/update but does not replace its complete getter TU. Acceleration is being recovered independently. GamePadUtilCompat must retire at activation, and `MR::isSubPadSwing` in OriginalSceneCounterQueries.cpp must retire too because the complete original GamePadUtil owns it.
