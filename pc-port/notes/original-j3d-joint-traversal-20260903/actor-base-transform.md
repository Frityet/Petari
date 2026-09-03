# Actor base transform and model scale

The native actor registry now retains the original **base TR matrix**, including
any basis the caller explicitly supplies. It does not decompose or normalize
that matrix. Separate `LiveActorModel::mBaseScale` defaults to `(1,1,1)` and is
seeded from `LiveActor::mScale` when the model binding is created.

Previously the generic PC `LiveActor::calcAndSetBaseMtx` built TRS, and native
`MR::setBaseTRMtx` multiplied its supplied matrix by `mScale`. Mario's existing
PC branch instead stored unscaled orientation/translation. That mixed contract
would double-scale generic actors when supplying the original joint traversal's
separate scale argument.

## Original source correspondence

- PC `LiveActor::calcAndSetBaseMtx` is now an exact copy of root
  `src/Game/LiveActor/LiveActor.cpp::calcAndSetBaseMtx`. It follows a taken actor's
  taking matrix when available; otherwise it selects `makeMtxTransRotateY` for
  zero X/Z rotations or `makeMtxTR`, then calls `setBaseTRMtx`. The obsolete
  native `make_trs_matrix` was removed. The shared matrix providers remain the
  existing compatibility implementations.
- Root `LiveActor::calcAnmMtx` sets the model base scale from `mScale` before
  invoking the virtual base-matrix method. PC restores that ordering through
  `MR::setBaseScale(this, mScale)` and its retained model provider.
- Root `MR::setBaseTRMtx` in `src/Game/Util/LiveActorUtil.cpp` copies the supplied
  matrix only. The PC adapters now do the same, without pre-applying actor scale.
  Root `MR::setBaseScale` writes the model's three scale values only. PC's adapter
  writes the retained native model scale, leaving `actor.mScale` and base TR
  untouched. An explicit scale override therefore survives later TR assignments
  until an original animation-phase scale update replaces it.
- The restored taken-object branch required `MR::getTaken`, whose existing root
  body is copied exactly into `compat/GameActorSensorCompat.cpp`. It reads
  `pActor->mSensorKeeper->mTaken` when the keeper is present, otherwise returns
  null. `Game/LiveActor/HitSensorKeeper.hpp` is an exact root header copy required
  for the typed member access. No keeper was fabricated or partially constructed.
- Parent-owned Mario integration follows root
  `MarioActor::calcAndSetBaseMtx`: immediately after copying the selected matrix
  into `mBaseTransformMtx`, root writes `getJ3DModel()->mBaseScale = mScale`
  (`src/Game/Player/MarioActor.cpp:2334`). The existing PC branch now invokes the
  model-scale provider at that same boundary. No new Mario action logic was added.

`verify-actor-base.py` checks exact copied bodies/header and the native API
connection. It does not claim new original-compiler or runtime validation of the
large Mario base-matrix method. The parent owns builds and test execution.

## Rendering and joint queries

`LiveActorModel::drawImpl` passes retained scale in the renderer's `base_scale`
option. The supplied actor matrix is raw base TR. Both
`joint_world_matrix` and `refresh_resolved_joint_matrices` now call the parent's
`J3dModelRenderer::joint_matrix(name, frame, baseTR, baseScale)` and retain its
already calculated result. The previous external concatenation was removed.

Callers in the actor registry, `MR::getJointMtx`, and the PlanetMap joint-existence
query continue supplying the same stable registry matrix reference. They inherit
the corrected separate contract through `LiveActorModel`. Model-scale snapshots
are independent of animation-resource sharing: one actor borrowing another's
animation retains its own model scale.

The owner/renderer now receive ordinary J3D base-transform inputs. This avoids
the incorrect `diag(actorScale) * jointRotation` ordering in Softimage when the
original calculation requires `jointRotation * diag(actorScale)`, and restores
Basic's accumulated-scale cancellation branch with the real initial scale.

## Other matrix consumers

Binder receives the base-matrix pointer through the same registry storage as
before. Root `LiveActor::initBinder` passes `getBaseMtx()`, so raw TR is the
original contract; actor model scale must not be baked into that pointer.

The full native regression exposed an obsolete Binder compensation for the old
TRS contract: registered actors divided the supplied matrix Y axis by
`actor.mScale.y` and reconstructed it from other axes or actor rotation for
degenerate scale. With raw TR, negative actor scale incorrectly reversed the
Binder offset. That owner-only correction, its owner map, registration/release
calls, and unused header were removed. Every Binder now uses the existing
unowned path: normalize the actual supplied matrix Y axis. Separate actor model
scale, including negative and zero values, cannot alter this direction. This is
a cleanup of the existing native Binder provider, not a claim that its entire
collision algorithm has been restored from the original game.

The actor-bound `FixedPosition` constructor also uses `getBaseMtx()` in root
`src/Game/Util/FixedPosition.cpp`. Its local offset therefore follows raw TR.
For position `(10,20,30)`, Y rotation 90 degrees, actor scale `(2,3,4)`, and local
offset `(1,2,3)`, the original result is `(13,22,29)`. The previous PC test expected
`(22,26,28)` from the native TRS approximation. A matrix explicitly supplied by a
caller may still contain scale; the original `mNormalizeScale` behavior applies
to that actual matrix, without stripping or guessing its source.

`ProjmapEffectMtxSetter` uses the model's `mBaseTransformMtx` directly in root
`src/Game/LiveActor/MaterialCtrl.cpp:128`, including the local-offset variant.
The native inverse now receives that raw matrix, which is the correct contract.
Tests that explicitly install a scaled/sheared base matrix remain valid because
base TR is copied literally, rather than inferred from `mScale`.

`CollisionPartsCompat` already calls `MR::makeMtxTRS` explicitly when its caller
does not supply a matrix; that scale-bearing collision path is unchanged.
SceneScheduler snapshots, the generic player fallback, and returned Game base
matrix pointers now expose the original raw TR. Mario's typed camera target
continues using its own original base-matrix getter.

## Effects boundary

`RuntimeContext::live_actor_effect_matrix` forwards the registry matrix, replacing
its translation with the current actor position. Generic native effects therefore
no longer receive actor model scale implicitly through that matrix. The original
`MultiEmitter`/`MultiEmitterCallBack` APIs distinguish host SRT from host matrix
bindings, while the root `registerAutoEffectInfoGroup`/`addAutoEffect` bodies that
choose the binding are still absent. There is no source basis for reapplying
`mScale` universally. Effects were left unchanged in this tranche; authored
automatic-effect scale/binding selection remains an explicit compatibility gap.
