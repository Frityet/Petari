# Collision movement owner prerequisites

This cohort closes the shared geometry update boundary before activating original CollisionParts update controls. It does not yet claim moving-platform gameplay: original `_CD`/`_CE`/`_D4` policy, LiveActor::calcAnim matrix submission, and CollisionDirector category timing still need their owner wiring.

## Original source proof

- `MR::calcVelocityMovingPoint`: new reference recovery at RMGK01 0x803E2960, 184 bytes, 99.891304% fresh Wii match. The only reported mismatch is the zero constant's symbol identity; operation/call order matches. It compares current/previous, writes zero if equal, transforms the current world point using inverse(current), transforms that local point using previous, and subtracts from the input. Native source is copied unchanged into OriginalCollisionGeometry.cpp.
- `MR::calcAreaMoveVelocity`: new reference recovery at 0x804009D4, 288 bytes, 96.52778%. Looks up the actual AreaMoveSphere, returns zero/false if absent, obtains its center/up, normalizes center-minus-point, removes that radial component from up, normalizes the tangent, then multiplies by Obj_arg0 (only -1 defaults to 10). Native body and its original inline getAreaIn helper are copied into OriginalAreaMovement.cpp. The actual passive sphere factory and manager are registered with retail order 54/capacity 0x10.
- `CollisionCategorizedKeeper::movement`: new reference-only recovery at 0x80173C04, 224 bytes, 97.14286%. Iterates zones/parts, skips inactive parts, calls updateMtx only for its own category, preserves zone bounds update conditions, then clears `_A1`. This source is not activated natively yet.
- `MR::createAreaPolygonList` already exists in reference. Its retail keeper and part assembly are retained here. The native provider uses the already tested original array path with exactly its two endpoint values. Both keeper paths form the same world bounds/order; the two-point part sends inverse-transformed endpoints to checkArea3D, which itself calls createBoundingBox on them. Thus the array path's intermediate local bounding-box sort is idempotent. It does not turn the input into eight corners of a world AABB. Reversed endpoints, affine transforms and collapsed local axes retain the original behavior.

All reference recoveries use normal original source conventions and the existing MW compiler, without inline assembly or pragma tuning. Full retail disassemblies, fresh objects, compiler commands/logs and objdiff reports are retained here.

## Native transform boundary

StageCollisionService::update_registered_transform accepts a live registration and its owner's exact committed current/previous transforms. It validates them, prepares all transformed prism geometry before publication, preserves triangle identities and borrowed matrix addresses, updates the source matrix, inverse and previous together, refreshes a previously decoded KCL's broad-phase radius, then rebuilds the existing BVH without changing source/zone order. Disabled registrations stay disabled; released/unrelated registrations cannot gain authority. Existing static registration remains unchanged. Registration and refit share the same local-to-world geometry calculation (vertices, four normalized source normals, geometric normal, slab thickness and BVH bounds), avoiding two competing implementations.

Triangle::calcForceMovePower now applies the original inverse(previous)->current formula to real native source matrices. It remains distinct from MR::calcVelocityMovingPoint. Original `mParts` instances keep the original method path. `Triangle::isHostMoved` awaits the original matrix counter owner; no substitute inferred boolean has been added.

All persistent refit allocations run under the shared HostAllocationScope and the next original allocation regains the caller's Game heap. The update preserves the stable-source cache used by original all-hit/area KCL traversal and by borrowed Triangle transform pointers.

## Verification

Focused native syntax checks pass for the changed service/providers/AreaObj registration and fixtures (see manifests; the initial missing test include was corrected). The line, AreaObj runtime, area polygon and collision registration targets are the focused validation set. Runtime results are recorded separately after coordinated builds.

## Remaining owner closure

The existing CollisionPartsCompat only snapshots input matrices and retains no original update flags. Native LiveActor::calcAnim currently omits retail setCollisionMtx after calcAnmMtx. The correct next step is to preserve the original virtual base-call submission point and consume it at the CollisionDirector movement category. An immediate one-shot update must also preserve the original forced makeMtxTR, validate/reset behavior, `_CE` handling, and `_D4` counter; simple enable toggles would be incorrect. Original CollisionParts::updateMtx/makeEqualScale/resetAllMtx bodies already exist and should supply that policy with their true scale flags and bound-matrix distinction. Water/sunshade collision categories are separate owner work; no regular-map query is repurposed for them.

Final coordinated results: line collision, stage collision registration and area polygon targets all build/run 0. The initial sphere assertion counted only the octree leaf entry; the real BVH contains all three physical prisms. Corrected that fixture expectation without changing production behavior. AreaObj runtime compiles but strict-link fails on five existing HUD/StarPointer provider gaps listed in test-results.json; its new movement fixture remains unexecuted. Exact tested and pending native manifests are separate.
