# Original Mario ground-result recovery — 2026-09-10

`Mario::updateGroundInfo` discarded `checkGround()`'s return and copied unrelated movement bit `_1F` into the grounded flag `_1`. The recovered statement now assigns the actual return directly. This is the only production change in this cohort, recovered in `decomp/` first and then mirrored into the port.

## Original evidence

- `notes/gateway-audit-20260907/restoration/retail/asm/Game/Player/Mario.s:5240` contains the original `updateGroundInfo` at `0x802ADAFC`.
- At `0x802ADB48`, the function calls `checkGround`. The following `rlwimi r4,r3,30,1,1` at `0x802ADB54` inserts its Boolean return from `r3` into the grounded bit, followed by the store to movement flags at offset 8. It does not read bit `_1F`.
- The exact correction is `mMovementStates._1 = checkGround();`. Native source line 1369, decomp source line 1369.

The existing check remains responsible for collision geometry, gravity, normals, ray probes, and ground corrections. No host code forces landing or unlocks controls.

## Startup and input consequence

The original constructor initializes movement flags to zero. The initial `ステージインA` animation maps to `StageStartGround` in `MarioAnimatorData.hpp`; its label does not itself set or clear grounded/jumping state.

`Mario::update` runs `updateGroundInfo` before movement dispatch. When `_1` is false, `mainMove` reaches `tryDrop` (`MarioJump.cpp:931`), which sets jumping and selects the falling animation. The normal jump path invokes `doLanding` when the ground flag becomes true; `doLanding` clears jumping at line 2242. `checkKeyLock` (`Mario.cpp:387`) can then clear the original input lock once Mario is no longer jumping and the initial 75-tick/nerve restrictions permit it.

The parent's old-code SDL replay independently demonstrated the stuck condition: at ticks 80–95, a focused window with free camera disabled received W, Aurora reported raw stick `(0,1)`, and the original Mario accessor returned `(0,0)`. Grounded stayed 0, jumping stayed 1, and movement `_22`/input-disable stayed 1. See `notes/wasd-live-trace-20260910/wasd-baseline-summary.json`. The trace also contains an independent NaN walk-speed defect under investigation; this patch does not claim to fix that or establish playable movement before an integrated run.

## Validation and publication

| Check | Result |
| --- | --- |
| Complete original Mario.cpp baseline Wii compile | Exit 0 |
| Complete original Mario.cpp corrected Wii compile | Exit 0 |
| `updateGroundInfo` baseline retail comparison | 98.5034%, 588 bytes |
| `updateGroundInfo` corrected retail comparison | 99.96599%, 588 bytes |
| Native Mario.cpp isolated LLVM 23 compile | Exit 0 |

The corrected ground-result instructions match the retail sequence. The only remaining bounded instruction difference reported by objdiff is a constant relocation operand later in the function, already present in the baseline. These percentages describe `updateGroundInfo`, not the whole translation unit. Both Wii compilations retain two existing nontrivial-union warnings; the native compilation retains three existing missing-override warnings.

Exact commands, exits, hashes, and full bounded comparisons are in `baseline.wii.json`, `fixed.wii.json`, `native-compile-command.json`, `function-comparison.json`, and `source-manifest.json`. The pre-edit source snapshot and full objdiff outputs are retained beside them. The repo decomp guide was followed; no root Xmake build was run by this agent.

Decomp commit `24136fc354b01f3a8de694380067d43e34ef3afd` contains only the ground-result statement correction. Author and committer are `codex <codex@openai.com>`. It was pushed to `origin/pcp-decomp` and its remote SHA verified, recorded in `decomp-publication.json`. Native integration, combined runtime validation, and root publication belong to the parent task.
