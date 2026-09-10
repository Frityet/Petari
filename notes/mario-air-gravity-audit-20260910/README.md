# Mario airborne gravity audit and original operand recovery — 2026-09-10

The airborne gravity vector is queried and published through the original owner, but `MarioActor::init2` initialized its acceleration multiplier to zero. The retail operand is **1.0f**. Only this one literal was corrected, in decomp first and then in the same native source span.

## Exact original evidence

- Retail `MarioActor.s:585–590`: `0x802AF4E0` loads `@78097`, then `0x802AF4F0` stores it at MarioActor offset `0x374` (`mGravityRatio`). The pool definition at line 8326 is `.float 1`.
- Constructor member initialization still sets zero, correctly. `init2` subsequently establishes the nonzero active multiplier; no other production assignment changes it.
- `MarioJump.cpp:1338` multiplies `gravityScale` by `mActor->getGravityRatio()`. The retail call and multiply are `0x802E54E8–0x802E54F4` in `MarioJump.s`. Ordinary airborne acceleration adds `_240 * jumpGravity * gravityScale` at source line 1369. A zero multiplier suppresses acceleration even when the direction is correct.

## Direction and ownership trace

`MarioActor::updateBehavior` invokes `updateGravityVec(false,false)` before original Mario movement. `MarioActorGravity.cpp` queries the actual scene `PlanetGravityManager` at actor position and center `_2A0`, selects/normalizes the result, publishes `_24C`, then calls `Mario::setGravityVec` and stores `_240`. `setGravityVec` writes `mAirGravityVec`. The compatibility query uses the original manager, requester identity, normal-gravity mask, authored priority, range, and inverse behavior.

Point gravity computes `center - queryPosition`; the manager normalizes the selected sum. Original jump launch adds **minus** `_240` times jump height; subsequent acceleration adds **plus** `_240`. No direction sign was changed or bypassed.

The mixed-input finite-speed run at `notes/preview-fps-crash-20260910/wasd-finite-speed.log` did not alone prove an inverted direction. Some input may have come from physical keys in addition to the scripted ranges. This patch fixes the confirmed missing acceleration operand; live validation after integration remains the parent's responsibility.

## Separate initial-ground defect reported for recovery

The same run showed translation while original Mario velocity remained zero at initial grounded ticks. Read-only comparison found that `Mario::checkGround` had lost the retail gate around its snap block (`0x802D4ADC–0x802D4B0C`): the original requires `(jumping && hitCount != 0) || (grounded && distanceToGround >= 5)`. The incomplete source entered for every nonzero hit count and replaced the shadow snap target with a perimeter ground probe. The parent and initialization agent own runtime confirmation and recovery of that separate function. This gravity patch does not alter its probes or snapping.

## Validation and publication

| Check | Result |
| --- | --- |
| Full MarioActor.cpp baseline Wii compile | Exit 0 |
| Full MarioActor.cpp corrected Wii compile | Exit 0 |
| Original `init2` baseline comparison | 99.66824%, 1688 bytes |
| Original `init2` corrected comparison | 99.68009%, 1688 bytes |
| Native MarioActor.cpp isolated LLVM 23 compile | Exit 0 |

The bounded percentage describes `init2`, not the full translation unit. Exact commands, compiler output, function comparisons, source hashes, and publication proof are retained beside this note. Existing compiler warnings are preserved; no root build was run by this agent.

Codex commit `91bf46ce4786465a260fb2cfcf82e8708bc396c5` contains only the one-literal correction. Author and committer are `codex <codex@openai.com>`. Push to `origin/pcp-decomp` and remote SHA verification succeeded. The native source was changed only at the corresponding literal, preserving all other current initialization hooks.
