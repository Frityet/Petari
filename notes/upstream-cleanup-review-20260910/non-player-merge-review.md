# Independent non-Player merge review — 2026-09-10

Reviewed `native-dispositions.json` against the applied root diff. The clean merges and imported non-Player source were checked for changed initialization, object lifetimes, matrix/field types, and native callers of renamed fields. Sky and SwitchWatcherHolder policy decisions remain with root/depth's separate retail review.

## Concrete integration defects found

The renamed/private NameObjGroup fields broke the existing native retirement owner in ActorRuntimeRegistry and several fixtures. The owner still needs to compact every occurrence of a retiring borrowed child, preserve survivor order, clear vacated slots, and update the original group's count. Removing that behavior would leave dangling original group members after native factory rollback or scene teardown.

Additional stale field callers were reported to root: WarpPod and StopSceneController used the previous group count API, excluded LiveActorUtil had the previous getter, CameraRepulsiveAreaUtilCompat read the old cylinder radius field, and area fixtures used old sphere/cylinder fields. Root then applied its resolved upstream overlap cohort, restored complete original AreaObjUtil, removed redundant area compat providers, and took the area fixture renames.

## Authorized group caller adjustment

This agent changed only these assigned paths after reporting the review findings:

- `src/Game/NameObj/NameObjGroup.hpp`: a forward declaration and a friend declaration for the existing `smgpc::compat::release_name_obj_runtime_state(const NameObj*)` function. Upstream fields remain private and the original Game algorithms are unchanged.
- `src/compat/ActorRuntimeRegistry.cpp`: the existing retirement algorithm now names `mObjArray` and `mObjNum`. Its stable compaction, tail clearing, and count update are unchanged.
- `tests/NameObjGroupLifetimeTests.cpp`: use original `getObjNum`/`getObj`; preserve duplicate retirement, member order, cleared slots, derived-group retirement, factory rollback/retry, and repeated registry-baseline assertions.
- `tests/OriginalSceneWipeOwnerTests.cpp` and `tests/SphereSelectorRealOrAbsentTests.cpp`: use the original count getter. Wipe identity and ordering checks remain intact.
- `tests/AuroraNativeTests.cpp`: rename the PartsModel fixture's owned `mFixedPos` access to `mFixedPosition`, preserving cleanup and root's independently migrated collector calls.

Fixtures no longer inspect the now-private capacity integer. No additional public getter, gameplay removal method, or replacement collection was introduced merely for tests. The lifetime assertions use the existing original public interface.

## Collector and remaining review

The restored archive collector's `MR::copyString` target exists in OriginalArchiveString.cpp and has the same `strncpy(dst, src, num)` body as the reference StringUtil. This restores the original 64-byte copy semantics; it does not retain the former compat-only null acceptance or 63-byte truncation. Reviewed active archive call sites pass real names, and native factory/count consumers were migrated to `getArchiveNum`. No new collector allocation or lifetime owner was introduced.

The reviewed PartsModel/FixedPosition changes preserve matrix storage and composition order while adopting named fields and the original matrix pointer type. LightDirector retains its native resource destructor and initialization ownership. Reviewed audio/header and NW4R changes were formatting, inline accessors, or original declarations; no additional concrete ownership or semantic defect was identified in this bounded review.

Static checks: scoped `git diff --check` passed; a fresh source/fixture search found no remaining references to `mObjectCount`, `mObjectNumMax`, `mObjects`, `getObjectCount`, `mFixedPos`, `mCalcOwnMtx`, or `mRotDegrees` after root's parallel migrations. No compilation, global build, decompilation edit, index mutation, or commit was performed by this agent. Root owns post-merge compile/link/runtime validation.
