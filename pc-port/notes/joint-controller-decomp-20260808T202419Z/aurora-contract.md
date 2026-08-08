# Recovered two-phase joint callback contract

Registration installs `JointController::staticCallBack` and the controller as
user data on the selected `J3DJoint`.

## Timing 0: before children

1. Resolve the model animation matrix by `J3DJoint::mJntNo`.
2. Copy it into a temporary `TPos3f`.
3. Pass that matrix and `{ JointController*, J3DJoint* }` to
   `calcJointMatrix`.
4. Only when the callback returns `true`, copy the result to both the joint's
   animation matrix and `J3DSys::mCurrentMtx`. The second write makes the
   modified transform the traversal basis for descendants.

## Timing 1: after children

1. Copy the selected joint's animation matrix into a temporary `TPos3f`.
2. Pass it and the same typed context to `calcJointMatrixAfterChild`.
3. Only when the callback returns `true`, write it back to the joint animation
   matrix.
4. Clear both callback and user data. Registration is therefore one traversal,
   not a permanent callback.

Null joint or user-data inputs are ignored and return zero. The two
`MR::setJointControllerParam` overloads bind the controller to the actor's real
model and resolve a joint by either name or `u16` index.

## Remaining generalized Aurora boundary

The current host renderer evaluates joint model matrices as immutable values on
demand. It does not yet expose:

- stable per-model joint identities with callback/user-data registration;
- a mutable animation-matrix palette indexed by joint;
- deterministic before-child and after-child traversal phases;
- a mutable current traversal matrix whose timing-0 write affects descendants;
- cache invalidation when a callback mutates a matrix;
- a real `J3DModel`/`J3DJoint` compatibility facade bound to
  `LiveActorModel`/`J3dModelRenderer`.

`NPCActorRuntimeCompat.cpp` consequently still rejects joint-controller
registration explicitly. The honest closure is a generalized joint-evaluation
hook in the renderer/Aurora compatibility boundary with tests for phase order,
writeback, descendant inheritance, one-traversal lifetime, and null handling.
The exact `Game` implementation should then be mirrored unchanged rather than
replaced by an actor-specific workaround.

The exact root `.hpp` and `.cpp` are now present under `pc-port/src/Game/Util`
and protected by the source-mirror test. `pc-port/src/Game/xmake.lua` removes
`Util/JointController.cpp` from the host build until the generalized boundary
above exists; the copied source is therefore provenance, not a fake provider.
