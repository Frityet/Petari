# MarioActorSensor retail audit

The parent observed the live chase crash in `MultiEmitter::create`, called by the native `MarioActor::trampleJump` action after PunchingKinoko contact. This audit independently checks source drift; the parent owns the crash and chase provenance. No alternate emitter names, fallback registration or null-emitter bypass was introduced.

## Retail verification

`verify-retail.py` compiles the unchanged canonical donor using the configured MWCC command without the dependency-output flag, preserving unrelated decomp/NPCUtil.d. `MarioActorSensor-match-summary.json` records the704-byte trampleJump function at99.77273% fuzzy match and complete4312-byte text at96.46382%. The entire reference text agrees with the actual Korean retail DOL after masking only230real ELF relocation fields. Retail DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`. Detailed objdiff/command/compile outputs remain alongside.

All seven changed trample resource strings are verified directly at the instruction and string addresses in `retail-fields-strings.json`. The real routine selects `ジャンプふみ1`, `ジャンプふみ2`, `ジャンプふみ3`, `ホッパーふみジャンプA`, `ホッパーふみジャンプB`, sound `声踏み`, and effect `ふみつぶし`. The previous native implementation used different animation names, sound `ジャンプ` and effect `踏み`. For example, retail0x802BF4D0 loads r31+0x199, resolved from base0x805B8BB8 to0x805B8D51; those CP932 bytes decode to `ふみつぶし`.

## Additional substantive field/constant drift

- `updateHitSensor`: the1000-unit eye radius is controlled by Mario movement flag `_F`, not `_2F`. Retail0x802BECFC loads Mario+0x8, then0x802BED00 extracts bit15. The previous native code used a flag in the next word. Canonical function match99.19643%.
- `doTrampleJump`: the early rejection flag is `_1C._6`, not `mMovementStates._1A`. Retail0x802BF07C loads Mario+0x1C and0x802BF084 extracts bit6. These are separate original words/flags.
- `doTrampleJump`: `_402` receives `mAirWalkTimeTornado` at constant-table offset0x3A4; the old native body used `mTrampleBegomaOpenTime` at0x138. Retail0x802BF0C0 is `lhz r0,0x3a4(r4)` and the next instruction stores Mario+0x402. Canonical doTrampleJump match99.41606%.

The remaining functions' strings and tuning constants were compared in the normalized diff. No further substantive drift was established. Numeric sensor constant ACTMES_TAKEN equals donor0x1F; the explicit updateScouter sine table index0xE3 matches retail table offset0x718 divided by8, and its angle/cosine expression represents the same five-degree tangent. Scouting target thresholds, combo counts, attack dispatch branches and setup sensor radii agree with donor.

The parent restored the entire canonical translation unit, differing only by the explicit CP932 include,26literal wrappers and the required LP64 numeric-overload correction `startPadVib(0UL)` to `startPadVib(0U)`. `native-source-equivalence.json` records exact normalized equality. This also removes stale native source variations without inventing replacements. The test-only action probe is separately described below once executed.

## Native build boundaries

The first focused build exposed the missing original default zero-vector argument on native initStarPointerTarget, used by the newly imported canonical Butterfly. That declaration was restored by the actor owner without changing its callsite. The second build found an LP64 overload ambiguity in canonical `startPadVib(0UL)`: Wii unsigned long is32bits and selects u32, whereas native unsigned long is64bits and integer zero is also a null pointer constant. The parent changed the suffix to0U to select the intended existing u32 overload. Both initial build logs remain preserved; no error was hidden or routed to an alternate action.

## Actual-owner action probe

`tests/OriginalProcessTrampleJumpTests.cpp` runs the actual original process and uses its natural Mario, animator, model, EffectKeeper, EffectSystem and JPA owners. It injects exactly one public trampleJump action with the original constant-table speeds; this is deliberately distinguished from actual sensor-driven contact. It requires the authored registered inactive emitter before the action, the original first-trample animation/combo afterward, actual live particle allocation, the final movement flag resets, subsequent normal frames and normal owner retirement.

Initial120-frame fixture failed its synchronous animation/combo expectation during the early opening, after the action itself returned. PID74490 was reaped with exit1 and no crash (`injected-action-process.json/log`). The fixture did not record the animation lock then, so the precise gate is not inferred from that log. Inspection found that original MarioModule/MarioAnimator refuse ordinary animation changes while `_B90` is set; the first fixture had not required that natural action prerequisite.

The revised fixture uses ordinary opening A-button input, waits until at least frame1800 with actual demo inactive, statusNone and the original animation lock clear, and leaves all those gates untouched. It records combo, authored animation, actual BCK and emitter state immediately and after the following normal frame. Its process bound is2100frames, with at least ten following normal frames required. All previous compile/fixture failures remain preserved. Validation result follows separately.

## Final injected-action result — passed

`injected-action-process2.json/log` records2100 completed frames, exit0,48.688seconds and PID75358 reaped without a timeout. Binary SHA256 `aaa0a27c84258e624fe64155c8615199ff2fb801b0efa27fc335471c8a64bd9c`. At frame1800 the real actor's animation lock, demo flag and status were all zero, and its combo counter was zero. The single public action selected the original first-trample animation, advanced the combo to1, activated the existing authored MultiEmitter and allocated a real live JPA-backed particle emitter.

On the following ordinary frame1801 the original animation remained active at animation frame1, the combo remained1 and the effect remained valid. The rest of the normal frame loop completed; actual player/effect-system retirement assertions passed. This validates the corrected public action under real owners and resources. It does not itself reproduce the sensor collision that triggered the original live crash or establish later story progression.

Final focused executable links are in `focused-builds4.json` (trample after shared header rebuild) and `focused-builds5.json` (revised test-only fixtures). A further Butterfly-only test field typo was fixed by its owner; that actor's projection fixture and actual placement result remain separately owned evidence. No production behavior was changed to satisfy these diagnostics.
