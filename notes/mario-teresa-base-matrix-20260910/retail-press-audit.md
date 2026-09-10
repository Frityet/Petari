# Independent retail press-path review

Read-only review for the concurrently recovered `MarioActor::calcAndSetBaseMtx`, based on RMGK01 instructions in `notes/original-mario-base-matrix-20260910/retail-short.asm` and the original `.sdata2` constants from `MarioActor.s`. No native or reference source is modified by this review.

- Active press `_390 != 0` sets `_394 = 30` before dispatch (`0x802B4390–0x802B43CC`).
- Types0/2, timer>15: sample both ceiling distances; clamp ordinary ceiling/150 to[0.2,1]. When this lowers `_3B0`, assign and skip the matrix-position/_1E0 block. Otherwise clamp pressed ceiling/150, lower `_3B0` if necessary, overwrite the main matrix translation with mPosition, and set `_1E0=true` unless Mario::_960==27 (`0x802B43D0–0x802B4470`).
- Types0/2, timer<=15: `_3B0 = 0.2 + 0.8*(15-_390)/15`. Both subpaths subsequently add `0.02*_398` when `_398 != 0` (`0x802B4474–0x802B44DC`).
- Types1/3, timer>15: sample width, assign actor mPosition from Mario::mPosition, then clamp width/80 to[0.2,1]. Lower `_3B0` directly when possible; otherwise subtract0.01 then clamp `_3B0`. Timer<=15 uses the same0.2/0.8 ramp. Both subpaths add `0.02*_398` (`0x802B44E0–0x802B45B8`).
- Type4: divide `_390` by120, multiply byPI, multiply by0.5, evaluate original cosine table, then `_3B0 = 160*(1-cos(angle))/160` (`0x802B45BC–0x802B4638`). Preserve the original order and retained multiplication/division.
- When no active press and `_394 != 0`, decrement `_394` first. Types0/2 and1/3 have identical bounce recovery: `1+(0.15*_394/30)*MR::sin(2*(_394*PI)/15)`. Other types preserve `_3B0`. When `_394` was already0, reset `_3B0=1` unless numerical bit3 of original Mario movement HIGH_WORD is set (`0x802B463C–0x802B4768`).
- Finally decrement nonzero `_398`, blend the original prior-base snapshot with the working matrix if mBlendMtxTimer!=0, copy the working matrix to `_3EC`, apply scaleMtx if `_3B0!=1`, publish that same working matrix and actor scale to the actual model, then set `_EA5=false` (`0x802B476C–0x802B4858`).

The key matrix identities are old model snapshot at stack0x308, forced-pose snapshot at0x2D8, and the sole working angle/translation matrix at0x2A8. Normal correction is created at0x1E8, with correction translation assigned to joint1::_2C; the correction matrix is copied to `_E3C`, referenced by joint0::_64. Forced correction is inverse(working matrix)*forced-pose snapshot. This review awaits the sibling's frozen source before reporting source agreement.

## Frozen draft review

Reviewed the sibling draft `notes/original-mario-base-matrix-20260910/recovered-function.cpp`. One actionable discrepancy was found and sent to its owner before production publication: the type1/3 width-press fallback subtracts0.1f, while retail loads @79802=`0.01f` at0x802B4530. This is verified both in the assembly constant and the original object bytes: `.sdata2+0x74 = 3c23d70a`, retained in `retail-MarioActor-sdata2.txt`.

Normal/forced correction and final matrix ordering agree with retail. In particular, the project's MR::multMtx(out,A,B) wrapper expands to PSMTXConcat(B,A,out); the draft's apparently reversed forced correction consequently computes the correct inverse(working)*fixed, and its bee translation/rotation composition also follows the retail order. The other press cases, >15/<=15 split, ceiling-query order, conditional translation update,0.02 timer boost, cosine shaping, countdown-before-sine recovery, numerical HIGH_WORD bit3 guard, final blend and _EA5=false agree in the reviewed source. This is a static source/retail audit, not runtime validation.

## Published source and Bee branch review

The corrected production function now has0.01f in the width-press decrement and both same-direction thresholds. The normal, forced and press review has no remaining discrepancy in the inspected paths.

A separate concrete Bee eligibility discrepancy remains in the frozen source: the third isStatusActive test uses MarioStatus_Flip (enum0x14), whereas retail0x802B3C34 explicitly loads0x1B. Both current native/reference MarioState.hpp map0x1B to MarioStatus_Bury. This was sent to the owner and parent before any further source save; the ongoing parent build was left undisturbed.

The remaining Bee eligibility flags, LOW_WORD bit30 ground-angle selection, head-to-foot translations, cross-product argument ordering, delay acceleration, angular limit, over-length correction, speed cap, friction selection, orientation fallback, and stop/movement translation blending agree with the reviewed retail sequence0x802B3BF4–0x802B42E4. In particular, default friction uses table0x674 and the stationary/nonjumping branch switches to0x67C even though the existing field names suggest the opposite; the draft preserves those exact offsets. The working matrix is overwritten by the correct up/front construction before its retained translation offset and actor translation are applied. No additional source changes were made by this audit.
