# Actual UserFile fixture migration

Status: both fixture translation units compile successfully with LLVM23 (exit0). Full commands and per-source digests are in `compile-results.json`; linked execution is pending the parent-owned Xmake lane. No Game/compat production changes, Xmake operations, or commits were performed during this fixture migration.

## Why the old harness was invalid

GameDataSession now constructs actual original UserFiles and GameDataHolders with their authored catalog dependencies; its constructor requires a GameResourceRuntime and shared ScenarioCatalogOwnership. The prior fixtures relied on a removed sparse host state map, fabricated aggregate totals, host copy/setter helpers, and one holder being both current and scene-start. They also assumed automatic story-progress5 seeding and required invalid original name/index calls to throw.

Those assumptions do not describe the original owners. The updated tests use a real disc catalog and process heap owner without a window or RuntimeContext. Current and backup are distinct original holders. Progress is0 on a fresh file, and a demo checkpoint is explicitly established with the original followStoryEventByName API. No SaveDataHandleSequence is fabricated.

## Preserved and strengthened coverage

GameDataStarStorageTests retains its full authored-catalog sweep: each valid authored star, repeated award stability, hidden stars, Grand Star classification/removal/re-award, and the total derived from individual galaxy bits. Coin maxima clamp at999; smaller later coin counts do not lower the record; visited state is independently stored. No out-of-range scenario or invented galaxy lookup is performed.

It now uses actual store_scene_start and UserFile binary load to prove deep copies rather than the removed host copy helper. A current reset clears the same original records without invalidating a previously obtained accessor and leaves both backup and another loaded UserFile unchanged. Explicit external catalog-handle retirement keeps the shared original catalog alive while sessions retain it. After the final session is destroyed, the catalog weak pointer expires, its active publication is absent, and current/backup GameDataFunction bindings throw again.

GameDataRealOrAbsentTests verifies all six selected-file slots against exact marioN identities, original current/backup UserFile associations, default story0, explicit story5 setup, stored Type_0 flags and VLE1 picture-book values. The original UserFile name copy receives its full eleven-element input buffer, avoiding the old too-short string-literal source. A bound holder does not make save-sequence-only user-name/system-config APIs available.

Both fixtures account for original PLAY semantics: loading a snapshot resets lives to4, while current lives and serialized Star Bits remain independent. Real-or-absent explicitly verifies nested current/scene-start pairs at different story milestones and exact original root-heap free-space recovery after each session. Native invalid slot0/7 construction remains tested because the host session owner explicitly validates these indices; original undefined table/index accesses were removed.

## Execution

Both linked fixtures require `SMGPC_REAL_DISC` set to the actual RVZ. Their source-only compile results do not establish a passing linked run or complete save persistence. The separate root-owned format adapter/chunk recovery cohort must be included in the link before execution.
