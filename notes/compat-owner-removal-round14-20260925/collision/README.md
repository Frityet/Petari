# Actual collision owner lifetime

Baseline `e7b214efc`. Removed `compat/CollisionDirectorOwnership.cpp/.hpp` rather than retaining the captured allocation lists under another name.

`CollisionDirector` now deletes its actual `mCategoryKeeper` array, four keepers and `mCode`. The keepers are claimed with the existing NameObj ownership API as soon as they are constructed, so the SceneObj construction transaction observes them without also adopting them. Constructor rollback deletes partially built children before the base NameObj unregisters. Each `CollisionCategorizedKeeper` deletes its own zones and hit buffer; `CollisionCode` and `CodeTable` delete their own tables and arrays, including constructor failure cleanup. Original query, movement, table and encounter-order algorithms remain unchanged.

Each auxiliary keeper owns its actual native category publication service. Primary category0 continues borrowing the existing active stage service; no duplicate primary collision owner was introduced. Registered CollisionParts retain a shared auxiliary-category owner until their registration and original part are retired. This preserves category metadata lifetime when the SceneObj director retires before the remaining native parts state. All six keeper query entry guards and all six CollisionParts guards remain, including generated geometry publication rejection before broad-phase culling.

The native API is `CollisionCategorizedKeeper::nativeService()`, `retainNativeService()` and `requireNativeGeometryPublished()`. The deleted sidecar had no production need to capture these allocations: they are all present directly on the original owners.

## Root integration

Root owns `compat/SceneObjHolderCompat.cpp` and `scene/SceneObjHolderRuntime.hpp`. Remove the sidecar include, forward declaration, friend/accessor, member, allocation, retirement/reclaim and rollback prepare/reclaim calls. The factory returns `new CollisionDirector()`. Existing NameObj ownership rollback handles children; no new collision-specific transaction is needed. Preserve `release_scene_collision_parts` and the existing scene-unpublication order. No xmake additions or flags are required; the canonical owners are already compiled and deleted compat sources are selected by glob.

## Preserved dirty work and existing tests

Snapshots cover every owned file. `CollisionPartsCompat.cpp` was initially dirty only for an extra RarcArchive include and removal of two obsolete `build()` calls; `OriginalSphereQueryTests.cpp` was initially dirty for removal of one `build()` call. These remain untouched outside the recorded migration delta. The sphere fixture now accesses the actual keeper service; the collision-owner fixture drops its unused deleted-header include. `UpstreamComponentTests` drops manual table deletion because CollisionCode now owns it. No StageCollisionService, HitInfo, ActorRuntimeRegistry or shared scene-binding source was edited by this lane.

`owned-working-delta.patch` contains only this lane's changes. Source inspection confirms the geometry entry guards remain and retired references outside root's pending shared integration are absent. No builds, test runs or new tests were performed.
