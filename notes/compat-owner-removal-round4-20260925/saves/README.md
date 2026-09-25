# Original save, story, and staff-roll owners

Baseline: port `3b37917cdcc04ac7e2ce190cab9f9835fe2b95fa`; current decomp `1a126cb5da311fedff662f53fb31c5aeaf851408`.

## Production changes

- Enable the complete canonical `Game/System/GameDataFunction.cpp`; its body is exactly the current donor. The sole native addition is `<cstring>` for `strlen`.
- Remove `GameDataFunctionCompat.cpp/.hpp`, `OriginalSceneCounterQueries.cpp`, and `OriginalPlayerEventUtil.cpp`. All their remaining methods belong to the restored owner. Current/scene-start/save-system/stage-count queries now use the actual original `GameSystem -> GameSequenceDirector -> SaveDataHandleSequence` or `GameDataTemporaryInGalaxy` ownership chain. The thread-local substitute current/backup binding and StageSession count fallback are gone.
- Remove `GameDataSession.cpp/.hpp` and `GameDataOwnership.cpp/.hpp`. They had no remaining production consumers. The former created independent host-selected UserFiles and published overrides; the latter was its construction/destruction helper. Actual process save initialization/backup is already implemented by original owners.
- Restore the exact current donor implementation of `StorySequenceExecutor::addDynamicDemoSequenceInfo` in its owning TU. It appends the record to the executor's existing eight-entry fixed array; it does not invent a host sequence or runtime allocation policy.
- Restore the full canonical `Game/Screen/StaffRoll.cpp` and current donor header. Only Japanese literal encoding wrappers differ from the donor cpp; the header is byte-identical. This implements the actual `startInfo`, `isPauseOrEnd`, and `MR::getStaffRoll` alongside the complete owner, removing the final exception placeholders from `StorySequencePlatformCompat.cpp`.
- Delete `StorySequencePlatformCompat.cpp`, for a total of nine compat files and 843 lines removed.

No decompilation was needed: every restored method is already in the current merged decomp. No changes to SystemUtil, FileUtil, FileLoader, FunctionAsyncExecutor, xmake, staging, or commits were made by this lane. `OriginalSystemConfigAccessors.cpp` defines SDK SC functions, not overlapping save functions, and is unchanged.

## Regression fixture migration

The five obsolete session/override fixtures now use `OriginalStageResourceProcessFixture` to initialize real Gateway process owners from the actual supplied disc:

- `GameDataRealOrAbsentTests`: real current/backup UserFile identity, original system configuration queries, current-only story and event flags, and `backupCurrentUserFile` changing just-event semantics.
- `GameDataStarStorageTests`: every authored star across the original scenario catalog, hidden stars, duplicate awards, derived Grand Star flags, coin maxima, visit bits, deep current/backup separation, binary round-trip, and original PLAY deserialization resetting loaded lives to four.
- `OriginalSceneCounterOwnerTests`: actual temporary Star Bit/one-up storage, restart identity, original 64-entry done-flag owner existence, life/miss counter boundaries, and scene coin result clamping. Removed host StageSession isolation/phase assertions. The old synthetic-placement done-flag setup test is not retained; it assumed host zone metadata in place of actual StageDataHolder identity.
- `StorySequenceRealOrAbsentTests`: stable per-executor dynamic records, original prologue route for a fresh Mario file, saved milestone selecting Gateway, and Luigi selection via actual serialized UserFile name. Removed source-text-only checks and expectations that calls outside the original process throw.
- `TalkRealOrAbsentTests`: actual TalkDirector, authored Gateway flow, independent controller advancement/reset, invalidation gate, BMG balloon text/reveal, and controller borrow retirement. Existing scene controls are preserved; the original invalidation flag is restored after the probe. Original scene teardown remains the process's responsibility.

`OriginalSaveDataSnapshot.hpp` is a test-only local serialization snapshot of a real UserFile. It does not publish or replace pointers. It saves game/config bytes and names, username, corruption/player flags, and a value copy of the entire PLAY object (including private life supply), then restores them before the observer returns. Temporary stage count scalars are likewise restored locally. Snapshots live under the process observer's host allocation scope and never outlive their original UserFiles. Each process fixture uses a fresh temporary save directory.

`OriginalParticleResourceOwnerTests` also moves to the real process: it asserts `MR::getParticleResourceHolder()` is the actual `GameSystemObjHolder` field, verifies all 3,327 particles, 225 textures, 2,591 auto-effect rows and 612 case-insensitive groups (614 case-sensitive), tests all names/numbered MR queries, and retains weak JPC/JMap observers to verify native backing retirement after process shutdown. The old host-only ParticleResourceOwnership minimum-budget/failure-injection tests are removed: that wrapper no longer supplies the original MR facade, and its standalone constructor no longer has the required original FileLoader process. No production ParticleResourceOwnership edit is included here.

## Wiring and evidence

Root coordinates build configuration. Remove the GameDataFunction exclusion; StaffRoll is found by the existing Game source glob. The six migrated test targets need `smg-pc-app` and the same Aurora application dependency as the existing process fixtures.

`before/`, `before.patch`, and `before-status.txt` preserve the scoped baseline. `manifest.json` lists the owned changed paths. `static-validation.json` records source comparisons. Targeted `git diff --check` passed; a whole-tree check still sees unrelated pre-existing whitespace in `notes/gateway-wakeup-demo-20260912/first-run.log`.

No build/test was run by this lane, per root coordination. Root reported that the integrated app built successfully and started a 600-frame Gateway check; focused migrated fixtures were still awaiting root execution when this note was written. Those results must not be inferred from these static checks.

Remaining limit: the active native `SceneObjHolderCompat` has no StaffRoll factory case; the excluded canonical `SceneObjHolder.cpp` has the original case. Full StaffRoll source restoration removes fake method implementations, but actual endgame scene creation/playback remains unvalidated and requires completing that scene-owner integration. This does not change the Gateway intro prerequisite. No alternate ending behavior or placeholder was introduced.

## Root validation feedback and fixture corrections

Root reported the migrated GameData, star-storage, scene-counter and particle fixtures passed, including the actual particle teardown assertion. Initial story and talk probes found invalid fixture assumptions; production was not changed:

- Original `cDemoPrologue` begins with type 8 (`Prologue`) and ends with type 12. `hasNextDemo()` specifically detects type 0 events, so it correctly returns false for this sequence. The fixture now inspects the actual sequence record instead.
- Deleting a controller while an active balloon borrows it in a live scene violates the existing native TalkDirectorLifetime contract. The fixture now invokes the original balloon `kill()` before deleting the second controller. It does not claim to simulate scene retirement inside a still-running process.

Root separately assigned one clean J3D transform fixture repair. `OriginalJ3dTransformAnimationTests.cpp` is snapshotted in `before/` and included in the manifest. The donor `J3DAnmKeyLoader_v15::load` (decomp lines 240-271) and `J3DAnmFullLoader_v15::load` (lines 97-128) use block size only for `getNext()`; after the final iteration that cursor is never dereferenced. `setAnmTransform` forms table/pool pointers from their offsets independently of the cursor size. Current unchanged `resource/J3dTransformAnimation.cpp` therefore correctly bounds referenced data by the retained file while requiring a usable non-final cursor. The stale rejection assertion now proves an oversized final cursor retains the expected animation sample, an oversized non-final cursor rejects, and an invalid referenced table still rejects even with an unused oversized final cursor. Existing truncated-file, invalid channel extent and invalid sample-time checks remain. No parser or J3D production edits were made.

The second Talk run still terminated because exception unwinding from an earlier assertion deleted the active controller before normal cleanup. Four read-only LLDB runs of only the built Talk executable (authorized by root) located the actual failure and verified the resource layout: `Text00` has `Pane::typeInfo`, while its `TxtText` descendant has `TextBox::typeInfo`, 21 authored text units, and the expected nonempty Korean BMG text. The old fixture incorrectly assumed the recursive container itself was a TextBox. The fixture now asserts the real `TxtText` pane and has a local RAII `kill()` guard before controller destruction, so any future assertion reports its own message instead of violating the active-borrow contract while unwinding. No Game/layout production change was warranted. See `talk-assertion-lldb.log` and `talk-pane-types-lldb.log`; the intermediate raw cast in `talk-balloon-fields-lldb.log` was diagnostic speculation and its zero-length result is invalid for the non-TextBox container, superseded by the verified runtime types.

Root subsequently confirmed corrected Talk and J3D transform tests pass. All six migrated save/talk/particle targets and the additional transform fixture have now passed through root validation.

Final independent read-only review used `GIT_INDEX_FILE=/tmp/petari-round4.index` against `3b37917`, not the working-tree patch. FileLoader and FunctionAsyncExecutor cleanup retains the deleted owners' drain/join/child/heap order; the MR save and SystemUtil facade routes now use original process objects. An index-wide src/tests include scan found no references to deleted compat headers. Targeted staged diff whitespace checks passed. The review flagged one staged RuntimeContextConstruction fixture relying on initially dirty, unstaged removal of default headless scene owners; root acknowledged and assigned its narrower failure-path replacement. No other blocking correctness issue was identified in the reviewed owner/SDK migrations. This review did not build or run a clean checkout of the temporary index, and full endgame StaffRoll behavior remains outside the validated Gateway demo.
