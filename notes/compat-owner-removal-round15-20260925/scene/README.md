# Actual SceneObjHolder ownership

Round15 baseline: 878278a2ffd451dbffc0957b7c604ca49521dfb5. No builds or test runs in this lane.

- Restored the complete original factory switch and constructor arguments in Game/Scene/SceneObjHolder.cpp, including original CP932 names. SceneObjAvailability.hpp explicitly selects the same 68 linked constructor cases as the removed provider; all 118 donor cases remain present. No source-exists inference or additional constructor enablement.
- The actual holder owns its retained Game allocation domain, constructed NameObjs, provisional slots and rollback transaction. It reserves metadata before adoption, claims each directly owned NameObj in the existing registry and retires in reverse registration order. Independently owned descendants remain observations in its construction graph. Native metadata allocates on the host; original factories preserve caller heap without holding a heap mutex across original async waits.
- MR::getSceneObjHolder reads only the actual GameSystem scene controller. Missing or retiring scenes remain absent. No alternate global holder, injected constructor callback, default arena or temporary scene republishing remains.
- OriginalSceneSupport retains the explicit holder pointer through controller unpublication, preserving talk/callback/effect/collision prepasses before native child teardown and executor destruction. Initialization and scheduler allocation scopes remain with the existing original scene support pending complete src/scene removal. Actual holder destruction is idempotent after its explicit early resource retirement.
- GameSceneBinding and NameObjChildOwner now use the holder's normal NameObj ownership claims, so rollback/teardown no longer depends on an active current-scene pointer. Late SwitchWatcher children are adopted by the actual holder. No collision-global cleanup replacement was introduced; actual LiveActor collision ownership handles its resources before CollisionDirector retirement.

Deleted production files: src/compat/SceneObjHolderCompat.cpp and src/scene/SceneObjHolderRuntime.hpp.

## Build wiring

Remove the Game/Scene/SceneObjHolder.cpp exclusion from src/Game/xmake.lua. The canonical source uses the existing Game glob. No new library dependency. Remove test targets smg-pc-sceneobj-holder-real-or-absent-tests and smg-pc-lodctrl-compat-tests (root verifies exact latter target name): their source files are removed because all groups depend on alternate holder publication or injected constructors.

## Existing fixture adjustments

Retained actual-process tests use Game/Scene/SceneObjHolder.hpp and nativeAllocationDomain(). SceneExecutionFixture now uses its supplied actual controller scene with a directly owned SceneObjHolder, and contains no injected factory or production publication wrapper. Existing postpass checks remain test-local. The NameObjGroup test retains actual membership/retirement checks and drops only constructor injection rollback. CameraContext and SphereSelector use the existing OriginalSceneControllerFixture. Mixed Aurora tests drop standalone switch publication and the standalone gravity-holder subsection.

The older OriginalSceneWipeOwner runtime branch and NPCActor testFloatOffsetAndBaseMatrix still lack a complete actual process fixture (preexisting; SceneInitializationBinding already required the original GameSystem). Their removed-header references compile against canonical owners, but no new runtime proof is claimed and no broad fixture rewrite was added. Parent owns test wiring and decides whether to retire those old standalone branches later.

Collision agent owns corresponding include/domain query migrations in OriginalCollisionPartsOwner, NameObjFactoryPlacement, OriginalProcessCollisionArea and OriginalSphereQuery fixtures. Avoid overlapping their manifests.

Every edited path has a before snapshot and exact patch. Initial dirty test rewrites remain in the working tree; parent should stage scoped patches or HEAD adaptations rather than whole dirty files. In particular GravityRealOrAbsent HEAD still differs from its working fixture structure after round14 isolated publication.

Publication update from root: the explicit instruction to remove all src/scene now adopts the preexisting scene-service deletions and their dependent working source/fixture migrations into round15. Root will stage those source/tests through its isolated index, so no additional HEAD-only fixture artifact is needed. Six unrelated route-note index entries, .vscode and older notes remain outside this lane. This is an incorporation of existing migrations, not newly tested coverage.

Build follow-up: the complete donor include list had 32 headers absent from the native checkout. All map exclusively to unavailable constructor cases. SceneObjAvailability is now included before constructor headers; all 47 headers used exclusively by disabled constructors share the exact constructor flags (including multi-class KameckBeamHolder and ChipHolder declarations). The include/declaration audit is saved in include-availability.json. No declarations imported, no source-presence enablement, and all 68 selected constructors are unchanged. Parent rebuilds; this lane ran no build or test.

Superseding user steering: all decomp code may be imported broadly (99.9% fuzzy match), with remaining unresolved functions permitted to be stubbed. Root requested full existing declarations over conditional header guards. The 32 absent headers are now imported byte-for-byte from decomp/include, with no missing transitive Game/SDK dependencies and no changes to existing native headers. All 47 temporary include guards were removed; include-availability.json records the diagnostic analysis only. imported-headers.json is the final import inventory. The constructor selection stays at 68 for this build; broader constructor activation belongs to the next coherent import/link closure. No stubs were needed in this lane.
