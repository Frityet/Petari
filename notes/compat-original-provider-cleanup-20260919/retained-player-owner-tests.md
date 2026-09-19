# Retained original player checks

Removing `MarioGatewayWalkTests` also removed the only target linking four useful test modules. Their checks need real original owners; they do not belong in the unrelated `GameDataPlayerStatus` PLAY-save test.

| Existing module | Required owners | New disposition |
| --- | --- | --- |
| `OriginalMarioStateTests.cpp` | Initialized `MarioActor` and its `Mario`; real canonical MarioState/Mario stack methods | Linked unchanged into the new original-process player-owner target. Tests base virtual behavior, Start/Notice/Update/Keep/Close order, recursion suppression, rejection, and linked-stack splicing. Restores both actual stack-owner pointers. |
| `MarioWalkParameterTests.cpp` | Actual player constants, movement state and map-code owner | Linked unchanged. Tests walking target modifiers, inertia priority, timer effects and ice/slip behavior, restoring all touched fields/constants. |
| `OriginalPlayerUtilTests.cpp` | Actual MarioHolder, player vectors/sensors, Swim and animation, NPC message resources, authored world KCL, scene allocation domain | Retains functional assertions with the test-only owner corrections below. Executes only on the terminal completed frame: its final original `onPlayerControl(true)` intentionally resets animation and states, followed by teardown with no additional gameplay frame. |
| `MarioCameraTargetTests.cpp` | Actual player, stage gravity and Binder; old fixture additionally installed a separate DemoSceneRuntime | Substantive camera assertions moved into new `OriginalProcessMarioCameraTests.cpp`, which borrows the actual active opening demo. No duplicate demo owner or synthetic demo metadata is constructed. |

`OriginalProcessPlayerOwnerTests.cpp` uses the existing explicit debug observer on the normal original process. It requests the real disc's Gateway scenario, runs 360 neutral frames, executes stack/walking checks after actual placement, verifies the same actor survives at least 100 ordinary frames after restoration, checks camera fields under the naturally active opening demo, executes terminal utility checks, and verifies original player retirement. No production code, fake route, actor factory or input bypass was added.

The new camera module preserves normalization, ground/gravity cache, Bee gravity selection, dead/clipped cache retention, live camera position, bound matrix orientation, airborne bound ground result, zero-up fallback, movement-timer wrap, unchanged timer suppression and non-mutating getters. The old explicit artificial demo-start/stop transition is not retained: the new bounded probe neither starts nor ends the real demo. Thus it does not claim outside-demo transition coverage.

New source files: `tests/OriginalProcessPlayerOwnerTests.cpp`, `tests/OriginalProcessMarioCameraTests.cpp`, `tests/OriginalProcessMarioCameraTests.hpp`. The parent owns the new `smg-pc-original-process-player-owner-tests` target registration. The shared build owner was given the frozen source and requested validation; no compilation/runtime result is claimed before that run.

## First actual-process run and fixture correction

`player-owner-360-result.json` records binary SHA-256 `06f96dceae52c19db672a6eed085ef1a98c398a13e982e8aa4cc2974d15b9485`, the exact invocation, real Korean disc environment, Metal backend, 180-second timeout and PID 5432. The process returned exit 1 after 9.71 seconds and was reaped. Original state-stack/walking assertions and all adapted real-demo camera assertions passed at frame 37; see `player-owner-360-run.log`.

The terminal utility module then failed with `SMG runtime context is not active.` Its old fixture acquired allocation through the retired alternate RuntimeContext scheduler and checked that facade's semantic trace, even though the real original scene was active. The test now acquires `current_scene_allocation_domain()` directly and rejects an absent scene owner. The obsolete alternate-facade trace assertion was removed. All PlayerUtil vector, translation, NPC float, actual KCL grounding and synchronous control-reset assertions remain unchanged. This is a fixture correction only; no production RuntimeContext was constructed or restored. Retest remains pending the coordinated build/GPU slots.

The next target-only build passed. Its run (`player-owner-fixed-360-result.json`, binary `1630bb76ddb7b4da2fa8ce9f2dc9659f00e434fb773c7189c4b7ad588e25410b`, PID 6785) reached the terminal utility module but rejected the old test's uninitialized TalkMessageCtrl node expectation and exited with SIGABRT after its reported failure. Canonical `TalkMessageCtrl::createMessage` creates the node; constructing the controller alone does not. The test now borrows an actual placed NPC with its initialized message/node owner, saves and restores its position and queried talk-state/type fields, and constructs no substitute talk controller.

The retained float-height test inputs also predated the corrected canonical `NPCUtil::calcFloatOffset` direction. The original computes NPC position minus player position and requires a positive dot with player up. The test therefore places the player below the NPC for its positive-rise case (`-100`), uses `-200` for its excluded strict boundary, and `+50` for the excluded negative-up-separation case. Expected rise/decay values, changed-up rejection, short-talk rejection and non-talking rejection are unchanged. The complete native function is byte-identical to the decomp donor; no production numerical change was made.

## Passing final execution

`player-owner-npc-build.log` records the target-only build passing in 3.9 seconds; the main executable was not rebuilt. `player-owner-npc-360-result.json` records the final binary SHA-256 `6b016be040e3231150077742cb2e9b802bdbf7ede7b335938afe218ff71c5768`, exact command/environment, PID 7323, exit 0, 7.045 seconds and completed process reaping. `run_player_owner.py` reproduces the bounded invocation and records the timeout path.

The final log establishes:

- Original MarioState lifecycle/stack and walking-parameter assertions passed at frame 38.
- All adapted real-demo camera assertions passed at frame 38.
- The same actual player survived more than 100 ordinary frames after restored test fields.
- Terminal frame 359 passed PlayerUtil vectors, authored NPC float-height predicates, position/sensor updates, actual world KCL rush grounding and departure behavior, and synchronous control reset.
- OriginalProcess reported exactly 360 completed frames, and the post-return assertion verified actual player retirement.

The unused `MarioCameraTargetTests.cpp/.hpp` pair was deleted after confirming no remaining callers; the new camera module carries the validated substantive assertions and explicitly excludes the former artificial demo-start/stop transition. This bounded test injects/restores test values to exercise API semantics; it is not controller-driven gameplay or a new proof of the complete Gateway story.
