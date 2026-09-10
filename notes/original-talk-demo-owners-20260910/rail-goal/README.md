# Original RailRider goal tolerance recovery — 2026-09-10

The newly restored NPC rail action fixture reached the forward goal but did not reverse. Its assertion expresses the original `startMoveAction` contract and is retained.

`RailRider::isReachedGoal` in the reference incorrectly passed explicit `0.0f` to the forward scalar `MR::isNearZero` call. The scalar helper compares `abs(value) < tolerance`, so the old predicate was never true, even at the exact endpoint. Retail instructions at `0x8016ADF0` and `0x8016AE24` load the same `@56212` constant in both directions. Its actual DOL bytes at `0x806BBCE8` are `3a83126f`, which represent `0.001f`. The recovered reference now omits the explicit argument, using the existing original declaration default `0.001f`; the identical one-line correction was then copied native. No shared math semantics, thresholds, rail coordinates, or fixture assertions changed.

All 192 bytes of the retail function disassembly match the actual `main.dol` (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`). Fresh full-TU Wii compilation passes before and after. Fresh function objdiff is 98.854164% before and after: the instruction fuzzy score does not distinguish this wrong constant-pool value. The remaining differences are existing register allocation and constant-pool symbol mapping; `compiled-tolerance-proof.json` resolves the actual before/after object symbol bytes independently. Native isolated LLVM compilation passes. The function bodies match exactly across reference/native; surrounding existing host adaptations remain.

Evidence: `retail-proof.json`, `isReachedGoal-retail.s.gz`, both `wii-*.json`/logs, `objdiff-*.json`, `native-compile.json`/log, both patches, and `source-manifest.json`. No global build, commit, or runtime was performed by this subtask. Parent owns linked regression validation and publication.

## Linked runtime confirmation

Parent subsequently rebuilt and ran `smg-pc-npc-actor-real-or-absent-tests` against the actual disc resources: all 6/6 fixture groups passed with unchanged assertions, including original reaction push/pop, forward rail goal reversal, and long/short talk actions. The linked run evidence is `../run-npc-tests.log.gz`. Rail correction was committed in decomp as `ec3fa56e7`; publication verification remains parent-owned. This is focused NPC/rail runtime coverage, not a completed Gateway bunny sequence.
