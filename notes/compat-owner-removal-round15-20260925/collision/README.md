# Round 15 collision ownership

Baseline: `878278a2ffd451dbffc0957b7c604ca49521dfb5`. This lane ran no builds or tests and made no Git/index changes. Root owns integrated validation and wiring.

Deleted `CollisionPartsCompat.cpp/.hpp` and `HitInfoCompat.cpp/.hpp`. There is no replacement actor-to-parts registry. `LiveActor` owns every created `CollisionParts` (including secondary and generated parts), and its native retirement releases them before sensors/model/archive resources. The existing actor teardown prepass calls the actual actor method. The vector is emptied by swap so heap retirement also releases its host allocation.

`CollisionParts` owns its server/map parser, decoded KCL, original ResourceHolder borrow, generated allocation, category lifetime, and registration. Resource bounds, placement/category/scale validation, committed-matrix publication, generated-geometry publication, and original zone membership remain. Part/server allocations retain client allocation routing. The canonical existing original collision algorithms remain in the same owner.

`Game/Map/HitInfo.cpp` now contains the complete donor implementation from `decomp/src/Game/Map/HitInfo.cpp`. The native changes guard an actual part through a weak lifetime token, without fabricated geometry-only triangles or diagnostic attribute caches. A retired Triangle becomes invalid; host/sensor access returns null, attributes are empty, and operations requiring owner storage fail explicitly. Original normal, vertex, matrix, attribute, and classification algorithms are preserved. Triangle heap finalizers release weak references when original heap-owned arrays retire. Its copy constructor and four existing copy sites preserve lifetime identity: the explicit assignment in MarioActor, plus Binder, MarioSlope, and MarioHang's expanded copies.

The forced CollisionPartsCompat header was removed. ModelUtil includes its actual ObjUtil declaration directly. Root enables canonical HitInfo with the existing collision FP policy; compat glob automatically drops both removed CPPs. No new target or test was added.

Existing fixtures use real actor/part ownership, actual Triangle::fillData, and actual scene-holder/domain accessors. Manual deletes of a part's now-owned server/parser were removed. Old retired-address side-map assertions were dropped while actual keeper membership, weak Triangle validity, runtime retirement, and archived/generated geometry assertions remain. No fixture was run here.

`owned-manifest.json`, `before/`, `after/`, and `lane-only.patch` identify every scoped delta. Initial dirty providers are intentionally deleted by the requested closure, with their complete prior contents preserved. Existing dirty test rewrites were not reverted or folded into this lane's patch. `OriginalProcessMapObjectTests.cpp` was initially untracked; its exact prior bytes are also preserved. `source-validation.json` records all donor HitInfo methods present and zero retired API references in current source/test C++ files.

Scene-holder integration: remove old `release_scene_collision_parts` hook; retire owned LiveActors through `release_actor_runtime_state` before deleting CollisionDirector/keepers. The scene lane owns this change. The actual part destructor removes enabled membership while its captured scene holder is current; the actual actor release is idempotent.
