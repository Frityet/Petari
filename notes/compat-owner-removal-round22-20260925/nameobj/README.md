# Actual NameObj lifecycle and borrower notifications

Round22 lane A, 2026-09-25. Fifteen existing owner files changed, all initially clean. `owned-manifest.json` records the Git baseline and before/after hashes; the baseline supplies exact pre-edit contents. No compat wrapper, map, or renamed state class was introduced.

NameObj now owns its generation, native owner claim, and intrusive lifetime links. Generation queries search actual live nodes without dereferencing a possibly stale argument; address reuse receives a fresh generation. NameObjHolder keeps its original registration/lookup arrays. Constructor registration failure unlinks the new identity before unwinding. Existing original movement flag behavior is unchanged.

The agreed static mark/snapshot/suffix APIs preserve construction order and reverse rollback. Snapshots allocate only host vectors. Early retireNativeLifetime is idempotent and does not delete raw Game array elements: it unlinks identity and dispatches borrow retirement to actual live objects. The NameObj destructor retains scheduler disconnection and original holder detachment.

Notifications use stack-local traversal cursors linked only while dispatching. Every unlink repairs all active next pointers before object storage disappears, supporting nested retirement, deletion of the next borrower, and self-removal. A generation cutoff excludes objects created during a notification. No notification snapshot or allocating callback registry exists.

NameObjGroup and LiveActorGroupArray compact their own real member arrays. TalkDirector, DemoDirector, DemoExecutor and BaseMatrixFollowTargetHolder expose their existing exact-signature cleanup as virtual overrides. Borrowing owner destructors unlink before freeing owned child state, preventing later child retirement from revisiting a partly destroyed owner. MsgSharedGroup cancels a queued message when its actual sender sensor retires. Lane B supplies LiveActor and HitSensorKeeper forwarding through the new NameObj sensor notification method.

NameObjCategoryList now queries actual NameObj generations and keeps its callback clone's actual `JKRHeap::Handle` until after clone deletion. It uses retainCurrentNativeLifetime and the actual clone allocation's heap fallback. Existing callback routing and self-replacement/clear lifetime guards remain intact; no current-heap scope is added around callback execution.

## Shared interfaces

The shared NameObj.hpp API was published before implementation and reported to root/lane B. It contains NativeRegistrationMarker{mNextGeneration}, generation/claim methods, mark/snapshot/newest/was-registered methods, retireNativeLifetime, and the two borrow notification virtuals. The heap lane confirmed Handle, retainCurrentNativeLifetime(), and heap->retainNativeLifetime(). Root owns all other caller/fixture migrations and removal of ActorRuntimeRegistry/JkrAllocationDomain pairs.

## Verification boundary

Only bounded source review and whitespace/retired-API searches were performed. All fifteen owned files pass `git diff --check`; none references either retired registry/domain API. No builds, tests, new fixtures, Git staging or commits were performed by this lane.
