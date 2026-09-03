# Original actor model authority retirement

This is a reviewable staged migration, **not production actor activation**. All
24 staged C++ translation units compile with the current native Game compiler
flags. No shared xmake target, production native source, or archive was changed.
The requested architecture corrections in root `src/Game/Player/MarioActorDraw.cpp`
are the sole production-source edit by this task. They remain for the parent to
review/commit. The native counterpart remains staged.

## Exact staged changes

`retirement.patch` contains the native changes and deletions; `files.json` lists
the complete set. `stage.py` regenerates them below
`build/original-actor-model-retirement-20260903/staged` without applying them.
`compile.py` builds the staged files only; `compile-summary.json` records their
current hashes and successful results. Full commands, object files, and logs
remain in that ignored build directory.

- `ActorRuntimeRegistry` removes `ActorAnimationRuntimeState`, the private base
  matrix, native `LiveActorModel`, independent frame controllers/animation names,
  native drawing, and every animation/model facade API. The retained state is
  `std::shared_ptr<ModelManagerOwner>` plus ownership of actual ActorAnimKeeper
  and ActorPadAndCameraCtrl objects. `retain_actor_model_owner(const LiveActor*)`
  returns a lease for scene draw prototype retention. The actual
  `actor->mModelManager` points at that owner's manager. Binder matrices now point
  at `actor->getBaseMtx()`.
- LiveActor's calc-animation matrix method, calc-view method and base-matrix
  accessor are copied from root. Its movement animation block calls original
  ModelManager/ActorAnimKeeper updates; the actual camera controller update is
  restored. Original model initialization follows its original scale, base calc,
  J3D calc and keeper creation sequence under scene allocation/GD scopes, with
  native ownership replacing raw manager allocation. Existing unrelated native
  lifecycle/sensor/effect registration remains; this is not a claim that the
  whole native LiveActor file is already the complete root implementation.
- Complete original ActorAnimKeeper, ActorPadAndCameraCtrl and FixedPosition
  files are staged. FixedPosition now consumes actual model/animation resource
  holders and original joint matrices. Its independent native resource-reading
  test helper and the existing `copyRotate` provider remain in the small native
  compatibility file. Root has no `FixedPosition::copyRotate` body.
- Complete original MarioAnimator is staged, retaining only the already-proven
  typed `J3DJoint::mMtxCalc` store and the lexical block that makes the original
  goto legal in Clang. Earlier original-compiler evidence for those adaptations
  lives in `notes/original-mario-animator-native-20260903`. The actual lower/upper
  animator ownership still requires the activation work below.
- Original MarioActor `calcAnim` and `calcAndSetBaseMtx` replace the two native
  model shortcuts. Other existing native MarioActor branches are preserved;
  their unresolved calls are inventoried instead of replaced with partial bodies.
- The five ready original provider units are `OriginalModelAccess.cpp`,
  `OriginalActorModelAccess.cpp`, `OriginalJointAccess.cpp`,
  `OriginalActorAnimationStart.cpp`, and `OriginalEffectBckNotification.cpp`.
  There is no `OriginalActorBckAccess.cpp`: its accessors are in
  OriginalActorModelAccess and its starts/actions in OriginalActorAnimationStart.
  The audio agent's `OriginalActorSound.cpp` now supplies original startBas;
  do not link the older OriginalActorBasStart unit alongside it.
- `OriginalModelBounds.cpp` imports literal model bounds, indirect/effect texture
  and collision-resource queries. LOD, NPC, PlanetMap and LiveActor utility files
  lose overlapping native providers. Player animation entrypoints use original
  MarioAccess calls. Runtime effect/player matrix snapshots copy the actual base
  matrix; StarPiece no longer rewrites getBaseMtx to the private matrix.
- The old LiveActorModel implementation/header, fake MaterialCtrl compatibility
  implementation/header and native LiveActorMatrix overload header are deleted.
  The temporary literal CSV-only OriginalActorResource extraction is also
  deleted; full ModelManager and OriginalActorModelAccess supply its methods.
- Generic native JGeometry receives the literal root dotX/dotY/dotZ and Euler
  setRotate methods plus ordinary componentwise TVec3 multiplication, allowing
  original FixedPosition/bounds code to remain unchanged. The CSV vector helper
  declaration comes from the parent's original CSV publication.

## Root MarioActorDraw architecture proof

`draw-root-architecture.patch` replaces three fabricated byte layouts with actual
J3DModelData/J3DJoint/J3DMaterial/DisplayListMaker fields. Root offset 0x28 is the
J3DJoint pointer table; offset 0x58 of its entry is **J3DJoint::mMesh**, not a
material's mpOrigMaterial. Original material-table/texture offsets become typed
mMaterialTable fields. The `_B80[2]` texture array uses array decay.

`prove-draw.py` compiles both complete translation units with actual GC3.0a3 Game
flags. Five affected methods have identical instruction bytes and matching
actual relocations, including referenced constants; generated local constant
label names alone are normalized:

| Method | Instruction bytes |
| --- | ---: |
| initDrawAndModel | 1184 |
| swapTextureInit | 868 |
| initFace | 288 |
| copyMaterial | 224 |
| changeDisplayMode | 192 |
| Total | 2756 |

The old `_B80` expression is not compilable with the current root array header.
For the baseline comparison only, the old same-address expression is explicitly
cast to JUTTexture**; `draw-baseline.json` records that normalization and original
Git blob/hash. This is a source-to-source algorithm-preservation proof. It does
not assert every inherited MarioActorDraw method is instruction-identical to
retail. `draw-adaptation-proof.json` contains relocations and exact commands.

## Required atomic activation work

1. Integrate the actual DrawBufferHolder scene owner, retained first-model leases,
   registration/allocation/activation ordering and light bindings. The separate
   scene agent owns this. Registry unregister runs before light/model destruction.
   No model may be replaced while registered; current initialization rejects it.
2. `remaining-native-consumers.txt` contains 14 old facade call sites: 11 in
   SceneScheduler and 3 in Showcase. Remove scheduler animation synchronization
   without adding a second calculation. Actual holder entry replaces independent
   actor view iteration; actual category drawing replaces both native model draw
   calls. Diagnostics should read actual model/frame/resource state. Showcase
   must inspect the actual J3DModel and original frame controller.
3. Retain a shared scene allocation cohort and own all actual MarioAnimator lower
   and upper players/cores/resource tables through teardown. The generic manager
   owner correctly retains its captured original model/player/core identities;
   it does not own the additional players authored by MarioAnimator::init. Restore
   or retire J3DSys, J3DMtxCalc traversal, material and queued GX references around
   scene phases before destroying their owners. Construction GD scope alone is
   insufficient for this scheduler boundary.
4. Import complete ModelManager/DisplayListMaker/MaterialCtrl/ModelX dependencies
   from the preceding reviewed package. Do not blindly link its probe Helpers.cpp:
   getJ3DModel, material accessors, getModelResourceHolder and getModelResName
   overlap this package. Keep only its nonoverlapping original helpers (including
   original initDLMakerProjmapEffectMtxSetter) or merge by exact signature.
5. Close full original Mario/effect/draw providers and existing native branch
   omissions before live activation. `link-frontier.json` inventories 232 missing
   non-standard C++ symbols against the current static libraries and actual staged
   manager/controller objects. It retains all functions, including unreachable
   ones, and is **not** the count of failures in a dead-stripped executable link.
   Staged audio and scene objects are not included in that comparison. There are
   zero duplicate strong definitions among this package's 24 compiled objects.
6. Update tests that assume the retired facade or expect named joints to be absent.
   The two-actor real-model removal test from the preceding package still supplies
   prior lifetime evidence; no additional isolated runtime showcase was added here.

Concrete animation/effect boundaries in the frontier include original
MarioAnimationEfx.cpp's `MarioAnimator::initCallbackTable`, `entryCallback` and
`runningCallback`; original MarioEffect.cpp's playEffect/RT methods; original
MarioActorShadow.cpp's initShadow; original MarioActorEye.cpp's initBlink; and
original MarioActorSensor.cpp's initForJump. These have root bodies to activate.
In contrast, `MarioActor::updateHand` and `createRainbowDL` currently have only
commented missing bodies in root MarioActorHand.cpp and MarioActorSpecialDraw.cpp.
They must be recovered if required by the final original path, not substituted
with a no-op or a different model path.

Reproduce from repository root:

```sh
python3 pc-port/notes/original-actor-model-retirement-20260903/stage.py
python3 pc-port/notes/original-actor-model-retirement-20260903/compile.py
python3 pc-port/notes/original-actor-model-retirement-20260903/prove-draw.py
python3 pc-port/notes/original-actor-model-retirement-20260903/inventory.py
```

No ROM/disc contents, credentials, image outputs, build binaries or shared build
configuration are part of the patch.
