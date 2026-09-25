# Gateway middle-route donor fidelity audit

2026-09-25. Initially read-only; the supervisor subsequently authorized the exact EarthenPipe/NPCActor fixes below. No builds, tests or game/controller actions were performed by this lane. The supervised live executable predates these edits.

## Concrete discrepancies restored

1. **Pipe exit skipped the waiting-to-emerge interval.** `EarthenPipe::isNerveShowUp` checked ShowUp twice; donor checks WaitToShowUp **or** ShowUp. `exeTargetPipeShowUp` otherwise exits immediately while its partner is still waiting. Restored the donor expression. This is relevant to the later authored route: root pipe l_id6 connects through pair70 to l_id7, whose Obj_arg2=1 selects a hidden/emerging pipe. The destination's original WaitToShowUp step20 and ShowUp animation must finish before PlayerOut. Source: `decomp/src/Game/MapObj/EarthenPipe.cpp`, `isNerveShowUp`, `exeTargetPipeShowUp`, `exeWaitToShowUp`, `exeShowUp`; placement: original `source-placement-inventory.json` root LayerA rows4/5.
2. **Pipe entry used reversed swimming and wrong distance predicates.** Native auto-rush rejection was `isPlayerSwimming() && PSVECMag(playerPos)>50 && playerPos.dot(_98)<-5`. Donor rejects a **non-swimming** player when the gravity-projected horizontal displacement exceeds50 and displacement along pipe-up is below-5. Restored donor `diff`, projected `delta`, and `!isPlayerSwimming() && delta.length()>50 && diff.dot(_98)<-5`. This removes incorrect side/below-rim acceptance on ordinary pipes and incorrect rejection of swimmers. It is an original general sensor-entry rule, not a guaranteed cure for an observed later-route stall.
3. **NPC turn limits were ten times donor speed.** `NPCActor::turnToPlayer(f32,f32,f32)` supplied `f2*0.17453294f` to `turnQuatYDirRad`, and `turnToDefault` used `(f1*0.17453294f)/acos(dot)` for the clamped blend. Donor uses `0.017453292f` in both places. Restored both constants and removed the stale “probably a typo” comment. `NPCUtil::tryStartTalkAction/tryStartTurnAction` call these owners; Tico uses the ordinary NPC talk/turn path. This is a confirmed motion-fidelity error, not proof that a Tico dialogue previously could not complete.

Only `src/Game/MapObj/EarthenPipe.cpp` and `src/Game/NPC/NPCActor.cpp` were changed. Exact before snapshots are under `before/src/Game/`; the lane delta and hashes are in `mid-route-donor-fixes.patch` and `mid-route-owned-manifest.json`. No native lifetime, allocation, pointer-width, or encoding adaptations were replaced.

## Gates inspected beyond mere source presence

| Authored gate | Current actual behavior and result of donor review |
| --- | --- |
| Five yellow chips → SW1009 → second SuperSpinDriver | ChipBase item-get calls ChipHolder→ChipGroup noticeGet. Five real entries trigger ChipCounter's ordinary completion demo; its Complete/End layout animation completion calls noticeEndChipCompleteDemo, and ChipGroup writes A. BlackHole, ChipBase, YellowChip, ChipGroup and ChipHolder match the donor after encoding/nerve normalization. ChipCounter's differently organized nerve bodies retain these actions; no fake completion or missing switch write found. The actual layout animation/demo must still finish in the live route. |
| Small ChildKuribo → key → SW1125 → CapsuleCage | ExterminationChecker creates the original no-item Kuribo and watches the actual child group for dead actors; after30 steps it appears its KeySwitch. Key item-get kills it and writes A. CapsuleCage's B listener starts its original camera/open animation and then kills the cage. These owners, Kuribo/KuriboMini, ItemGenerator and the inspected WalkerState family match donor logic. |
| Rescued Tico → SW1127 → SpinDriver | Tico's ordinary message kill callback or talk-end enters Meta; Metamorphosis BCK completion calls NPCActor::kill, which writes SW_DEAD. NPCActor turning differs as corrected above; no no-op metamorphosis or manual switch replacement found. |
| Middle KuriboChief → key → SW1100 → cage/Tico → SW1008 → pipe | KuriboChief owns a KeySwitch when A is valid and appears it from its real kill path. Key/cage/Tico use the same owners above. The root pipe reads A1008 and its paired destination uses real emergence; the corrected waiting-state bug applies directly here. |
| Launch traversal | SpinDriver and SuperSpinDriver primary owner bodies match the current donor after encoding/nerve normalization, retaining capture, animation, trajectory and exit logic. This does not establish runtime trajectory/camera correctness. |

No remaining no-op or cut implementation was established in these gate bodies. Old `grand-star-route-20260925` missing-source/factory claims predate the full imports and should not be repeated. Empty `CapsuleCage::exeWait`, chip deactivated nerves and EarthenPipe::calcAnim are also empty in donor source/header; they are not missing implementations.

## Separate finding reported, not edited

`src/Game/Util/PlayerUtil.cpp` clears `0x0F000000` in six damage-ending bind helpers, where donor assigns `RushEndInfo::mFlags.mDamageType` after setting `0xC0000000`:

| Helper | Donor damage type |
| --- | ---: |
| endBindAndPlayerDamage | 1 |
| endBindAndPlayerFlip | 6 |
| endBindAndPlayerAcidDamage | 4 |
| endBindAndPlayerFreezeDamage | 3 |
| endBindAndPlayerFireDamage | 2 |
| endBindAndPlayerElectricDamage | 5 |

PlayerUtil remains owned elsewhere and unchanged by this lane. This was reported as an exact donor semantic difference, not as a proven blocker for the specified chip/key/pipe route. No broader utility audit was undertaken after the supervisor bounded this scope.
