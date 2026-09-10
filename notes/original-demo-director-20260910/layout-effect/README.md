# Original layout effect resume routine

Recovered `MR::pauseOffEffectAll(LayoutActor*)` in original `decomp/src/Game/Util/LayoutUtil.cpp`, following `decomp/AGENT_DECOMP_GUIDE.md`. The declaration already exists in LayoutUtil.hpp. This closes the actual DemoStartRequestUtil link dependency.

Retail `LayoutUtil.s` at **803D9090–803D9108** (124 bytes) loads LayoutActor::mEffectKeeper, iterates PaneEffectKeeper::mEmitters using its active signed count, skips null or invalid MultiEmitter entries, and calls `pauseOff(-1)` for each valid emitter. It does not synthesize a keeper or accept an absent actor/keeper; original caller preconditions remain intact.

Full original Wii LayoutUtil TU compilation succeeded. This one recovered routine scores **98.22581%** against the retail object. See `proof.json` for the exact command and bounded symbol result, and `objdiff.json` for instruction comparison. This percentage describes the recovered routine, not the entire partially decompiled utility TU.

The exact body was mirrored to native `src/Game/Util/LayoutUtil.cpp`. That TU remains excluded, so the same body is published by existing `src/layout/LayoutUtilCompat.cpp`, beside the original layout isRegisteredEffect provider. Its one necessary MultiEmitter include was added. No Game logic or heap ownership is substituted; active calls use the real PaneEffectKeeper and MultiEmitter graph.

One isolated full native LayoutUtilCompat TU compile succeeded, using the current compilation database flags. See `native-proof.json` for source hashes and command. Parent owns the combined link and smoke run; this note does not claim runtime proof. No root Xmake, commits, or expanded tests were run here.
