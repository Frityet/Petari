# Native model animation clock regression

The real-disc Mario walk test initially passed host WPAD sampling, positive
stick speed, continuous real-KCL grounding, and animated-model submission. Its
enhanced failure report isolated the failure to model phase:

```text
saw_run=1;current_bck=Run;packets=12;triangles=4877;
display_list_bytes=61184;animated_packets=12;
bck_frame=0.000000;bck_frame_max=60;animation_advanced=0
```

This is an existing runtime integration regression, not a Metal compiler
problem. Commit `f91bec3c7` made the native model's BCK phase come from the
authoritative per-actor J3DFrameCtrl rather than elapsed global frames. Its
synchronization was added only to `LiveActor::calcAnmMtx`. Derived actors that
override `calcAnim` without calling that base function advance their controller
in normal movement but never publish the new frame to their model. The current
Mario slice has exactly this override shape.

The fix adds a general native-model publication step after the scheduler's
virtual CalcAnim call. `synchronize_actor_model_animation` copies the current
BCK controller phase/rate/state and refreshes already-retained joint matrices
using the actor's computed base transform. It neither increments time nor
selects an animation, and it honors dead/no-calc flags. Actors already using the
base implementation see an idempotent publication. No reconstructed Game C++
file changes or actor-name conditions are involved.

The existing LiveActorUtil real-disc test now covers an ordinary Tico model in
a generic derived actor overriding CalcAnim. It checks phase publication,
stable retained joint identities, the no-calc phase/joint freeze, and explicit
phase publication while automatic animation advancement is stopped.

Final runtime verification passed: the real-disc Mario walk test,
extended LiveActorUtil test (6/6), and title/file-select route test. Their
complete logs are in this directory.

The root agent's first post-fix Mario walk run passed: 325.685 units of
grounded travel, Wait -> Run -> Wait, 12 Run packets, release/idle through frame
271, and successful actor recreation. The new derived-actor fixture initially
remained in LiveActor's default dead state; adding its normal
`makeActorAppeared()` lifecycle call makes it eligible for scheduler execution.
The separate no-calc and stopped-animation assertions remain unchanged.
