# Original MarioActor gravity update recovery

Restored `MarioActor::updateGravityVec(bool, bool)` in root `src/Game/Player/MarioActorGravity.cpp`. The function was a commented placeholder. It now contains the complete recovered RMGK01 routine at **0x802B9A9C, size 0x930**, using the existing Mario, gravity-query, matrix, and vector interfaces. The PC translation unit is not changed or activated by this task.

This is an explicitly **nonmatching functional recovery**, supported by original-compiler comparison, checked retail operands/control flow, and independent source review. It is not a native Mario gravity/jump execution claim. No animation, state-object, or collision dependency is bypassed to activate the function.

## Verification result

- The source compiles with the configured **GC/3.0a3** compiler flags, **RMGK01 / VERSION=0**, and configured **sjiswrap v1.2.2**. Compilation uses the real root source and include hierarchy. The only diagnostics are the two existing nontrivial-union-member warnings in `MarioActor.hpp`.
- **objdiff-cli v3.6.1: 87.323130%** against the real RMGK01 split object. Compiled size is **0x960**, versus retail **0x930**. This is below the guide's matching aim and is reported without claiming a higher adjusted score.
- All **160 critical control-flow events** agree, including the complete branch topology, integer/floating comparison instructions, tested bit masks, condition combinations, and non-vector helper call order. The comparison retains each branch's destination in the common event graph.
- The graph compares JMA acos at its original library-call boundary because the current root header expands it in this TU. It omits vector copy, assignment, negation, addition, scale calls, and register save/restore helpers. Consequently graph equality alone does **not** prove numerical arguments, field selection, memory aliasing, or the inlined math; those are covered by the separate source/binary review and reference checks.
- Every original referenced scalar constant and animation string is present with the same bytes. The additional four constants are the inlined JMA acos constants: -1, pi, 1023.5, and pi/2. The external double used for unsigned integer conversion is checked against the real DOL data split.
- The animation argument is verified from its **actual retail `lis`/signed `addi` pair**, and both gravity-ratio initializers are verified from their **actual SDA2 load/store pairs**, with the selected registers and field offsets checked. Merely finding a matching string or float elsewhere in the DOL is insufficient.

`compiler-evidence.json` retains hashes, exact addresses and words, the raw objdiff result, graph event counts/hash, literal values, and the ratio writer inventory. The full objdiff, aligned instructions, annotated assembly, relocated bytes, compiler command/log, and control graph remain under ignored `build/mario-gravity-restoration-20260903/`.

## Recovered behavior

The two input flags retain their observed roles: the first skips transition reactions and resets the direction-lock timer; the second uses the position sample in place of a separate center sample. Gravity is queried at the actor position, center, and a point 100 units along Mario's front. The existing Foo query can override the samples. When position/center directions differ sufficiently, comparison to the preceding air-gravity direction chooses the sample. Both directions are normalized through the existing original helper.

The routine preserves the original inclusive 30/72/120-degree transition thresholds, jump-vector component removal/scaling, short-jump animation request, orientation transport, two zero-gravity transitions, ground-normal fallback, Bee-state adjustment, and final Mario gravity write. `_24C` retains the sampled vector before zero-gravity fallback; `_240` retains the actual gravity supplied to Mario.

The tail computes the actor offset from the real head/ground vectors and original flags, preserving 70/50-unit heights, the inclusive 59-degree blend condition, 0.1/0.5 blend rates, two 15-frame timers, and the strict vertical-speed >160 positional correction. The correction projects `_30C * 80 - mHeadVec * 80` onto `_240` and passes the resulting positive gravity component to `Mario::push2`.

See `review.md` for the independent check of every section, exact bitfield masks, vector signs/cross-product order, timer conversion/order, and the corrected animation argument. The review caught an HA/LO signed-address error before freeze: the retail pair resolves to **0x805B86D8**, whose Shift-JIS text is **`ショートジャンプ`**, not the different animation at 0x805C86D8. The reproduction script now verifies the pair directly.

The only required declaration correction was coordinated with the Mario source-restoration agent: root `Mario.hpp` had `isUseFoolSpecialGravity`; it now declares `isUseFooSpecialGravity`, matching the existing `MarioSpecial.cpp` body and actual retail call at 0x802F3294. `MarioActor.hpp` required no changes.

## Gravity-ratio correction in the parent checkpoint

This recovered routine does **not** write `mGravityRatio`. The earlier jump audit attributed that missing initialization to this routine; the retail evidence instead identifies an existing root decompilation error in `MarioActor::init2`.

| Function | Load / store | Effective constant | Stored field/value |
| --- | --- | --- | --- |
| `MarioActor::init2` | 0x802AF4E0 / 0x802AF4F0 | SDA2 -1784 = **0x806BF528**, bits **3f800000** | actor r29 + **0x374**, **1.0** |
| `MarioActor::initMember` | 0x802BB1E4 / 0x802BB1F4 | SDA2 -1316 = **0x806BF6FC**, bits **00000000** | actor r31 + **0x374**, **0.0** |

The ratio getter at 0x802B9670 reads actor+0x374 directly. The script also inventories direct `stfs` instructions with displacement 0x374 in Mario-named functions; the two above are the only such hits. That bounded scan does not rule out arbitrary indexed or indirect writes elsewhere.

The parent corrected root `MarioActor::init2` and its PC counterparts to the source-backed 1.0 value, leaving `initMember` at zero. That correction is separate from this root-only restoration, and its PC validation is recorded by the parent. This work adds no stage-specific ratio, environment condition, or new gameplay rule.

## Remaining differences and limits

The raw mismatch includes JMA acos inlining versus the original external call, vector scale/`operator*=` inlining versus calls, different temporary stack locations/register allocation, and constant-pool relocation placement. No math-library/header edits or compiler-flag changes were made to improve the score. The current header's acos implementation and shared math providers remain their existing dependencies; this task does not establish complete native numerical parity.

The recovered behavior requires the original Mario movement state, animation, gravity, and collision lifecycle. It has not been linked into the PC player slice or exercised in a live jump. The existing undecompiled `updateBeeStickMode` remains a separate source frontier; no body or fallback was added for it.

## Reproduce

From the repository root:

```sh
python3 pc-port/notes/mario-gravity-restoration-20260903/verify.py --compile
```

Required inputs are the verified `build/compat-math-oracle/main.dol` (SHA1 **25c5959534b3c21246c6c7e42021b916b41fb578**), configured original compiler, wibo, sjiswrap, dtk, objdiff-cli, and Homebrew LLVM objcopy/objdump. The script uses `dtk dol split --no-update` with the checked-in RMGK01 symbols/splits and a generated local config. It does not alter production decomp configuration, build the PC targets, or stage any game binary. Running without `--compile` verifies the existing compiled object.
