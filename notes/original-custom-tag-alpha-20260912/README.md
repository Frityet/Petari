# Original CustomTagAlphaCtrl recovery

Recovered all five original functions from RMGK01 addresses80350A4C..80350C98 in `decomp/build/RMGK01/asm/Game/Screen/CustomTagProcessor.s`. Corrected alpha-controller fields to signed delay/endDelay/frame/characterIndex/waitFrames, unsigned length, active flag and two float rates. The original object is0x24bytes; former header incorrectly described three bytes and a GXColorS10.

The controller starts inactive/opaque, supports delayed sequential character alpha, accumulated wait frames, capped frame advancement, and inactive immediate completion. A zero frame-alpha step disables the controller while preserving other fields. Alpha clamps the minimum first with the exact ordered/unordered branch direction, then clamps at zero and truncates255*alpha. No new finite guards or timing special cases were introduced.

Added the missing standard min/max templates to the reference MSL algorithm header so the original compiler can express reference-returning minimum/maximum selection. Native code uses the actual standard library.

`wii-build.json`/log prove original Metrowerks compilation; `function-proof.json` reports matches: constructor66.08%,init98.68%,alpha91.53%,update99.50%,isEnd85.44%. Constructor differs in equivalent zero-store scheduling; other lower matches retain the decoded arithmetic, field layout and control flow. This is a functional recovery, not a claim of every function meeting90% fuzzy match.

`alpha-native-probe.cpp` compiles the exact recovered five bodies and exact corrected alpha class declaration in isolation from the still-incomplete surrounding processor. `native-proof.json` records source/header hashes, native compile/run success, and layout/delay/per-character/wait/end/disabled/unordered-clamp checks. Integrated processor rendering is owned by the process-services follow-up and is not established by this isolated proof.
