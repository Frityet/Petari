# Actual NameObj registration and scene helper removal

Baseline: round15 root 16d91cbb0 and Aurora 11a54a1. This lane ran no builds, tests or git operations.

Deleted src/scene/SceneNameObjRegistry.cpp/.hpp and NameObjChildOwner.cpp/.hpp. The former's process-global nested binding list, fallback allocated holder and borrowed-holder registry are gone. The latter had no production consumers outside its own implementation before removal; only seven test files used it.

NameObj now records the actual NameObjHolder that received its original registration. Its constructor invokes the existing NameObjRegister::add when the original register and holder exist; standalone SDK objects retain their native metadata without inventing a scene. NameObjHolder::add preserves fixed capacity and duplicate checks, then sets the exact back-reference. Native single-object retirement erases the primary entry and lookup cache in surviving order. clearArray and the actual holder destructor clear remaining back-references without deleting borrowed NameObjs. Native snapshots allocate on the host. NameObj destruction and the real process prepass detach through the recorded holder, requiring neither a live controller scene nor a search through global aliases.

SceneExecutionBinding no longer constructs or borrows a NameObj registry. Its discarded NameObjHolder argument was removed. Actual NameObjRegister selection retains original GameSystem timing. The existing OriginalSceneControllerFixture now creates/selects the actual NameObjRegister and releases it, eliminating its previous implicit fallback registration.

The unused capture exclusivity singleton/class and postpass delegation fields/APIs were removed from ActorRuntimeRegistry after inspecting all source consumers. Live production registration markers, ordered snapshots and explicit owner claims remain: SceneObjHolder still uses them for real constructor rollback. No replacement child capture helper or renamed service was created.

## Existing tests

- Retired SceneNameObjRegistryTests.cpp and its synthetic nested-registry contracts. Root removes its target from tests/xmake.lua.
- ActorRuntimeRegistryTests drops the old exact-size/no-native-member contract and the three invented construction capture/delegation groups. Remaining unrelated checks are preserved; no execution claim.
- DemoStartRequestHolder and original execution tests retain real owner assertions and remove only test-only rollback invocations.
- Shadow-line and sensor probes use direct unique_ptr ownership of actual LiveActors. Actual actors own their shadows/sensors/collision resources. Explicit dependent-before-endpoint retirement stays intact.
- Collision agent owns the corresponding unique_ptr migrations in OriginalProcessCollisionAreaTests and NameObjFactoryPlacementTests, alongside its collision edits.
- Existing SceneExecutionFixture callers mechanically drop the unused original_names argument; no new test framework or new cases.

## Coordinated integration

Collision agent owns OriginalSceneSupport.cpp: remove registry include; use _names->snapshotNativeObjects(), object->detachNativeHolder(), and the three-argument SceneExecutionBinding constructor. Those edits belong to the collision manifest, not this one. Root owns all xmake changes. Source files use existing globs; remove the deleted test target and no new source additions are needed.

owned-manifest.json, before/ and patches/ record every path owned by this lane. Production and fixtures are frozen pending root integration.
