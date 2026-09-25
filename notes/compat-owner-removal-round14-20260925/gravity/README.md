# Canonical gravity, follower and rail ownership — round 14

Deleted `GlobalGravityOwnership.cpp/.hpp` without introducing a replacement graph capture service, counters, TLS owner, or type-dispatch switch.

## Actual owners

- `GlobalGravityObj` deletes its actual polymorphic creator. The ten original factory functions use a local unique owner until creator allocation succeeds, so a failed creator allocation destroys its registered actor cleanly. The successful factory result and all original gravity initialization/calculation bodies remain unchanged.
- Each concrete `GravityCreator` destroys its field; the Wire creator also destroys its `RailRider`. `PlanetGravity` records its actual registering manager and unregisters on destruction. The manager clears these backreferences if it retires first, preserving safe teardown in either order. Removal compacts the existing priority-ordered list without recalculating or changing priorities. Registration rejects duplicate owners/capacity overflow before modifying the list.
- General `RailRider`, `BezierRail` and `RailPart` destructors release their own graphs. The rider/Bezier classes adopt allocations immediately into unique members, including iterator, part array and cumulative-coordinate array, so partial construction unwinds without special Wire knowledge. Rail calculations, original sampling count and authored table borrows are unchanged.
- `BaseMatrixFollowTargetHolder` owns followers and targets directly. Follower destruction owns its link record; each target owns an independent copy of its three-value link identity. Several followers can therefore share a target after the first follower retires, without borrowing its link allocation. Target actor/matrix/validator inputs remain borrowed.
- Follower insertion prepares a complete target before publishing the follower, and `MR::addBaseMatrixFollower` retains the pointer across lazy-holder construction. Failures leave no uncaptured suffix. Original follow binding/matrix calculation/update behavior is unchanged for live actors.
- `releaseNativeReference` on the actual follower holder removes a retiring object's followers and clears borrowed target actor/matrix/validator references. The existing NameObj retirement scan calls this method before removing the object's registry identity. Gravity following is invalidated when its target retires; no stored dead target is dereferenced.

The obsolete factory adoption hook is removed. Existing gravity tests retain original manager rules, exact ten creator mappings, real authored rail sampling, follower binding, matrix calculations and queries while the actor owns the field. Retired contracts cover service counters, missing/overlapping fake ownership, suffix capture of expired external objects and continued field use after deleting the actor. No new cases or framework were added.

## Shared integration and preservation

Root owns `SceneObjHolderCompat.cpp` and `SceneObjHolderRuntime.hpp`. Remove the old include, constructor allocation, reclaim/reset calls, member, forward declaration, friend and `current_global_gravity_ownership` accessor; no replacement scene object or API is required. Root performed that removal during this lane's source review.

ActorRuntimeRegistry also contains the effect agent's coordinated mechanical migration: actual EffectKeeper include and delete/null instead of `release_actor_effect_keeper`. That agent owns EffectKeeper production implementation.

The manifest records exact snapshots and hashes for 21 changed/deleted paths, including the initially dirty GlobalGravityOwnership.hpp, NameObjFactory.cpp and GravityRealOrAbsentTests.cpp. Existing unrelated edits in the latter two were preserved. The unchanged GravityCreator.cpp was inspected and snapshotted separately. `scoped.patch` is generated against these before-copies, without Git operations.

No explicit source additions are needed. Regenerate the compat source glob after the pair deletion. No builds, tests or runtime checks were run by this agent, following the requested reduced verification.

## HEAD-only staged test artifact

`staged-GravityRealOrAbsentTests.cpp` is generated from the exact `git show HEAD:tests/GravityRealOrAbsentTests.cpp` snapshot saved beside it as `HEAD-GravityRealOrAbsentTests.cpp`. Stage the artifact bytes, not the working test. Its SHA-256 is `3505ddc5b2d4a082fa45333ac67cf041b2fa29639f6b8e70e87316ea4ad206c2`.

This removes the retired sidecar include, capture helper, counters, and three service-only groups (adoption rejection/duplicate rules, artificial failed suffix capture, deferred field plus expired external follower scope). Remaining actual actors call original `init` directly. The existing follower fixture now transfers a heap object to its actual owning holder; the existing stack creator destroys its own field. All unrelated HEAD fixture structures, synthetic BCSV/rail builders, preflight case, manager/query rules, standalone GravityScene and CLI remain unchanged. The baseline dirty working-tree actual-process rewrite is not imported.

`staged-GravityRealOrAbsentTests.patch` and `.json` record this separate HEAD delta, source/artifact hashes and the unchanged working-file hash. No build/test was run and no index or working test was modified. Any pre-existing limitations of the untouched HEAD standalone scene/synthetic placement fixtures remain outside this isolated sidecar adaptation.
