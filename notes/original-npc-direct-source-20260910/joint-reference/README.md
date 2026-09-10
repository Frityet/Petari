# JointController reference restoration — 2026-09-10

Reconciled the earlier native JointController candidate into the decomp reference only after checking each missing function against retail. The decomp guide was read for this task. No native Game files, shared build configuration, index, or commits were changed.

## Fresh validation

The partial reference began with four functions. The restored reference now has all ten original functions, including five additional explicit methods and the compiler-emitted `J3DModel::getAnmMtx(int)` accessor. **Every function matches 100%; .text is 684/684 bytes and .data 16/16 bytes at 100%.** The full baseline and restored Wii TUs compile0; exact commands and comparison results are saved in `wii-results.json`, `objdiff-before.json`, `objdiff-recovered.json`, and `objdiff-summary.json`.

`retail-byte-proof.json` separately verifies all ten functions' actual instruction bytes against `decomp/build/compat-math-oracle/main.dol` (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`). The authoritative reconstructed assembly/object is `notes/gateway-audit-20260907/restoration/retail/{asm,obj}/Game/Util/JointController.*`. This confirms the older note's match claim with a fresh current-reference build; the old note alone was not treated as proof.

The restored `JointController.cpp` and `JointController.hpp` are byte-identical to the existing native files; no native edits were necessary. J3DModel.hpp already has the required accessor inline, so no SDK header change was needed. `source-manifest.json` records exact hashes and `reference.patch` records the four changed reference paths.

## Retail callback contract

- `registerCallBack()` writes this controller to the actual joint's callback user-data and installs the static callback.
- Pre-child (`calcJointMatrixAndSetSystem`, 0x804067E4): get the model's animation matrix for the joint's unsigned16 joint number; copy to a local matrix; call the virtual handler with info `{this, actualJoint}`. Only a true result commits the local matrix to both the model animation buffer and `J3DSys::mCurrentMtx`. False discards local edits.
- Post-child (`calcJointMatrixAfterChildAndSetSystem`, 0x80406878): the same copy/callback pattern commits a true result only to the model animation buffer. It does not update `J3DSys::mCurrentMtx`.
- Static callback (0x804068FC): null joint or null callback user-data returns0 before any work. Timing0 invokes pre-child and retains registration. Timing1 invokes post-child and then **unconditionally clears both callback and user-data**, including when the virtual handler returns false. Other timing values do neither and retain registration. Return is always0.
- Both parameter overloads resolve the joint first (name or unsigned16 index), then resolve the actor's actual J3DModel and store the pair. No additional validation, fabricated matrix, or cleanup policy was introduced.
- `getAnmMtx` (0x80406A34) follows the model's matrix buffer, then its animation matrix array, and adds `jointIndex * 48` bytes, matching the existing typed accessor.

## Typed-info consumer reconciliation

The old `JointControllerInfo` misdescribed slot0 as an integer and slot4 as an artificial partial joint struct. They are now `JointController* mController` and `J3DJoint* mJoint`, matching the two pointer stores in retail. The header also declares the actual unsigned16 parameter overload, without a legacy alias.

Two reference consumers still used the partial spelling. With explicit parent authorization, `SkeletalFishBaby.cpp` and `SkeletalFishBoss.cpp` now use `rInfo.mJoint->mJntNo` and include the actual J3DJoint header. Each baseline TU was compiled against the captured old header in `baseline-headers/`; each final TU was compiled against the restored typed header. **All code/data sections are 100% identical before versus after; zero functions changed in either TU.** See `consumer-compile-results.json` and the two `*-before-vs-typed.json` files. Separate comparisons against retail are recorded in `consumer-retail-summary.json`.

Neither of these two boss source/header pairs exists natively, so no native consumer change was needed or made. The four-file reference cohort is frozen. Parent owns native activation, linked fixtures, and publication; no runtime or full-decomp-build success is claimed by this isolated compiler proof.
