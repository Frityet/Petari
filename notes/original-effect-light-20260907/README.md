# Original effectLight audit — 2026-09-07

The low 12.272727% objdiff score for MultiEmitterCallBack::effectLight does not
indicate missing lighting behavior. The original retail object contains an
eleven-instruction, 44-byte function. It saves the stack/link register, loads
`mEmitter->_2C` and the default near-zero epsilon, calls `MR::isNearZero`, restores
the frame, and returns. There are no color writes, light lookups, or emitter
updates in the original method.

The current C++ body returns `MR::isNearZero(mEmitter->_2C)`. A fresh original
Metrowerks compilation produces four instructions, 16 bytes, which load the same
inputs and tail-call the same function. Both ELF constants contain exactly
`0x3a83126f` (the float representation of 0.001). The difference is call-frame and
tail-call generation; the observable input and returned value are identical.

`effect-light-proof.json` records every instruction on both sides, object hashes,
and independently decoded epsilon values. The original object is
`notes/gateway-audit-20260907/restoration/retail/obj/Game/Effect/MultiEmitterCallBack.o`.
The current source was recompiled with the command and successful result in
`wii-compile-results.json`; the full diff is `MultiEmitterCallBack.objdiff.json`.

The actual `decomp/AGENT_DECOMP_GUIDE.md` was read. Its completion requirement
allows a functional 1:1 match or high fuzzy match, so adding artificial code to
raise the score is unnecessary. No source or declaration was changed. This
method does not supply a lighting algorithm to recover, and inventing one would
reduce fidelity to this retail binary.
