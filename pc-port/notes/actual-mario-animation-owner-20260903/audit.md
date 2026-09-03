# Actual Mario animation owner: source and runtime frontier

Read-only audit on 2026-09-03, root checkpoint
`a9560169e02f226a3087844326e1f137073a4685`, while the separate Aurora merge is
being verified. Starting evidence is
`../original-binder-native-20260903/mario-grounding-audit.md`. No production
source, build configuration, or tests were changed; no compiler or runtime test
was run for this audit. Existing source is distinguished below from a fresh
retail verification.

## Finding

The next missing component is a retained **actual model and animation owner**,
not another animation-name query implementation. The original XanimePlayer
transition, frame, and query functions are already decompiled. The original
XanimeCore lifecycle, blend, and matrix functions are now present in PC Game.
The native renderer nevertheless owns a separate joint tree and one-track core;
it does not construct a J3DModel or either of Mario's XanimePlayers.

The smallest coherent production owner must retain typed model data and a real
J3DModel, typed motion resources, the authored XanimeResourceTable, and both
original lower/upper XanimePlayers. Its model/joints/matrix buffer must be the
same ones used by rendering and joint queries. A separate player used only to
answer movement predicates would permit game state and the rendered pose to
diverge. Closing this owner is wider than a constructor-only import with the
current SDK dependencies; the bounded first implementation step is identified
below.

## Why grounding reaches it

- Root `MarioSlope.cpp:25`, `Mario::isUseSimpleGroundCheck`, calls
  `MarioAnimator::isLandingAnimationRun` on its normal flat-ground path. The
  current root reconstruction discards the return value; that detail should be
  checked against retail when importing the floor group, rather than inferred
  from the helper's name. The call itself still requires the real player.
- `MarioSlope.cpp:63`, `checkGroundOnSlope`, queries the authored animation
  `崖ふんばり`. Root `Mario.cpp:1617`, `getGravityVec`, queries `ハード着地`
  when grounded and selecting gravity.
- `MarioAnimator.cpp:227` implements the landing-group queries; its final
  branch also uses `MR::isBckPlaying`. `MarioModule.cpp:105` queries the lower
  XanimePlayer and, while `_6C` is set, the upper one. The query for a null name
  compares the actual current/default animation pointers.

Thus the original floor/gravity methods need valid current/default/group and
frame state, including transitions. The PC Run/Wait string and a single
J3DFrameCtrl do not supply those contracts.

## Original construction and movement order

1. `MarioActor.cpp:282`, `init2`, calls `initDrawAndModel` before constructing
   MarioAnimator. `MarioActorDraw.cpp:94` initializes separate `Mario`/`Luigi`
   model and `MarioAnime` animation archives through the ModelManager, obtains
   an actual J3DModelX, and prepares its extra matrix buffers and rendering
   state. Full restoration of this actor method also includes the special-model
   and draw-adaptor systems; it should not be silently represented as complete
   by the normal-model owner.
2. `ModelManager.cpp:339`, `initModelAndAnimation`, constructs its actual model
   or initial XanimePlayer depending on the animation holder's motion count.
   `ModelManager::getResourceHolder` at line 307 returns the animation holder
   when its original XanimeResourceTable exists. `getJ3DModel` returns the
   player's model when a player exists. These are distinct from the model
   archive holder. Root `ModelUtil.cpp:33,39,584` supplies `newXanimePlayer`,
   `newXanimeResourceTable`, and `newJ3DModel`.
3. `MarioAnimator.cpp:32`, `init`, constructs XanimeResourceTable with the
   exact `marioAnimeTable`, aux/offset tables, single/double/triple/quad tables,
   and optional Luigi swaps. `MarioAnimatorData.hpp`'s `基本` group contains
   WalkSoft, Walk, Run, and Wait with authored initial weights `(0,0,0,1)`.
   The constructor initializes the callback table, constructs the lower player,
   selects/defaults `基本`, enables joint transforms, installs the lower player
   in `mActor->mModelManager`, constructs the upper player, selects its default,
   and shares joint-transform storage.
4. Original `LiveActor.cpp:110`, `movement`, invokes `ModelManager::update`
   before gravity, nerve/control, and Binder. The manager calls the lower
   player's `updateBeforeMovement` followed by `updateAfterMovement` once.
   Original `MarioActor::movement` may subsequently call MarioAnimator::update
   when `mDrawStates._1A` is set. Animator update separately advances the upper
   player only when `_6C` is active. Putting both player updates in an arbitrary
   renderer draw call would change frame and landing timing.
5. `MarioAnimator.cpp:403`, `calc`, installs the lower/upper calculators at
   root/Spine1/Head as required, then performs original model calculation twice
   with the lower core's `_6` equal to 1 and then 2. It clears the installed
   calculators afterward. The PC branch at line 443 currently only calls
   `require_actor_model`; the native rendering traversal does not execute this
   two-pass Mario sequence.

## Current native ownership

| Current component | What it actually owns | Missing connection |
| --- | --- | --- |
| `compat/ActorRuntimeRegistry.cpp:39` | Native BCK/BRK J3DFrameCtrls and animation names, plus LiveActorModel | No original ModelManager or XanimePlayer |
| `render/live_actor/LiveActorModel.cpp:65,476,749` | Archive-name selection, current native BCK summary, renderer, retained scale, joint-query cache | It sets a single BCK on its renderer and passes a host-managed sample frame |
| `compat/OriginalJ3dJointTree.cpp:55,68,197` | Actual J3DJointTree, J3DMtxBuffer, joint objects, matrix arrays, one original XanimeCore | No J3DModelData/J3DModel owner; `calculate` installs a temporary one-track animation and restores its basic calculator |
| `resource/J3dTransformAnimation.cpp/.hpp` | Genuine constructed Key/Full animation objects and retained native-endian channel/value arrays | Decoder is usable for resource tables, but is not registered as MarioAnime's original motion table |
| `compat/J3DModelDataCompat.cpp` | Actual default ModelData and embedded table constructors/destructors | No complete authored BMD/BDL typed resource loader or model instance |
| `compat/ResourceHolderCompat.hpp:25` | A native archive pointer and resolved path in a different global ResourceHolder class | Incompatible with Game/System/ResourceHolder's ResTable fields; must not be passed to XanimeResourceTable |
| PC `MarioAnimator.cpp:37,443,1118` | Null original resource/lower/upper pointers; Run/Wait selection; native model presence check | All three owner/update/calc replacement branches remain |

The native Game resource and core sources are useful completed prerequisites,
not evidence that the live Mario animator is active. No definitions of the
actual `MR::getJ3DModel`, `getJ3DModelData`, `getResourceHolder(LiveActor*)`, or
`calcJ3DModel` were found in the current compat providers. Their root bodies
already exist in ModelUtil/LiveActorUtil and depend on the real manager.

## Existing root code versus recovery work

| Group | Existing root source | Native/recovery status |
| --- | --- | --- |
| XanimePlayer | `Game/Animation/XanimePlayer.cpp`, both constructors through XanimeFrameCtrl constructor | Complete named player lifecycle/transition/query group exists; PC has header plus only `isRun/getSimpleGroup` extracts. Import must remove those duplicate extracts. |
| Player frame reporting | `Game/Player/MarioAnimator.cpp:1308`, `tellAnimationFrame` | Keep one provider when importing Player; it is not missing decompilation. |
| XanimeResourceTable and HashSortTable | `Game/Animation/XanimeResource.cpp`, `Game/Util/HashUtil.cpp` | Original resource unit and required hash helpers already native. Earlier notes' missing-core list is stale after the matrix-calculation checkpoint. |
| XanimeCore | `Game/Animation/XanimeCore.cpp` | Entire restored core unit now native. Existing verifier/test notes cover prior work, not a new claim here. |
| Core transform accessor | PC `Game/Player/Mario.cpp:2528`, `getJointTransform` | Contrary to the earlier model-closure note, the current root Mario.cpp has no body. The PC-only null-check/index body exists; root header only declares it. Verify/recover root `0x802AEC3C`, size `0x20`, before treating that provider as an authoritative original import. Preserve one native definition. |
| MarioAnimator callbacks | `Game/Player/MarioAnimationEfx.cpp:49,74` and remaining callback bodies | Root exists, PC unit remains excluded. Init/change also reach real BAS, blink, and special-mode animation methods in MarioSound, MarioActorEye, and MarioActorParts. |
| ModelManager and ModelUtil | `Game/LiveActor/ModelManager.cpp`, `Game/Util/ModelUtil.cpp` | Root construction, getters, update, and calc exist; native uses its separate registry/model owner. |
| ResourceHolder | `Game/System/ResourceHolder.cpp` | Root constructor/mount/table dispatch exist. Full construction requires real archive/heap operations, typed loaders, BckCtrl, and material-animation support. |
| MaterialAnmBuffer | Header only at `include/Game/Animation/MaterialAnmBuffer.hpp` | No root constructor body found. Constructor is retail `0x800183F4`, size `0xCC`; its helper group is listed at `0x800184D0..0x80018798`. This is a real recovery dependency of original holder/manager material-animation construction. |
| J3DModel | `JSystem/J3DGraphAnimator/J3DModel.cpp`; destructor in `Game/Player/J3DModelX.cpp` | Root virtual bodies exist. None of the actual model virtual table is currently supplied natively. `calcNrmMtx` is declared/called, with no standalone root definition/symbol found; establish its retail inline sequence before adding a helper. |
| J3DVertexBuffer | Header in `libs/JSystem/include/JSystem/J3DGraphBase/J3DVertex.hpp` | `init`, `setVertexData`, destructor and `setArray` lack root bodies. `frameInit` is already inline. |
| Matrix/deformation closure | J3DMtxBuffer.cpp, J3DSkinDeform.cpp, J3DCluster.cpp | Existing root bodies, but only bounded matrix-buffer initialization is currently native. Envelope paired-single math needs verified portable support; optional deformation wrappers must retain their real behavior when closing model virtuals. |

This is a bounded dependency audit, not an exhaustive claim that all downstream
J3D material/shape/loader methods are decompiled or linkable. Source availability
also does not substitute for the original-compiler/retail verification required
before importing historical Mario animator/update logic.

## Owner contract and replacement plan

Retain one actual typed model owner per model instance, with its model data,
joint/material/shape/vertex resources, actual matrix buffer and full model
virtual contract. Retain each motion archive and loaded transform object until
all resource tables and players using it retire. Migrate the archive-only native
ResourceHolder type so that the global Game class cannot have two incompatible
definitions; do not reinterpret its archive pointer as a ResTable.

The authored Xanime table constructor mutates group and single-animation tables
with pointers and hashes. Multiple actor/model lifetimes must either retain the
matching resource owner for those global tables or use owned copies with all
internal table links correctly retained. Lower and upper cores share their joint
and optional transform lists but own separate tracks, so native cleanup must
delete shared storage once. The original empty destructors should remain intact.

The renderer should consume the actual model's calculated matrices and selected
calculators. Its current private one-track Core must not overwrite an installed
lower/upper calculator. Native scope restoration must also cover `j3dSys.mModel`
once an actual model is selected, in addition to the matrix/calculator/scale
globals already preserved by the tree owner.

When that foundation is available, import the complete original XanimePlayer,
bind the real ModelManager/ResourceHolder accessors, and restore MarioAnimator
init/update/calc together with their concrete callback dependencies. Advance
the lower player at the original LiveActor manager phase, upper at the original
Animator phase, and let original calc choose the sampled frame and two matrix
passes. Only then replace the PC grounding/update and post-Binder shadow overwrite
with the recovered original player phase. BAS/audio support remains a declared
separate capability if not provided; animation names or termination predicates
must not be invented to bypass it.

## One bounded implementation tranche to start now

Recover and import the **actual J3DVertexBuffer lifecycle and data attachment**:

| Method | RMGK01 address | Size |
| --- | --- | --- |
| `setVertexData` | `0x80423874` | `0x48` |
| `init` | `0x804238BC` | `0x40` |
| Destructor | `0x804238FC` | `0x40` |

Use root `src/JSystem/J3DGraphBase/J3DVertex.cpp` first, verify with the configured
original JSystem compiler and current RMGK01 DOL, then mirror unchanged bodies
into a dedicated native SDK provider. Keep the original field layout and
allocation ownership; establish what the destructor does from the binary.
The existing genuine J3DVertexData constructor supports a meaningful fixture:
construct actual data/buffer, attach retained position/normal/color arrays,
exercise the original inline `frameInit` and pointer-swap methods, and destroy
without fabricated objects or virtual slots. This closes a real prerequisite of
even J3DModel's default constructor/destructor and its `entryModelData` path.

Completion of this small tranche must not enable MarioAnimator's original branch
by itself. The subsequent owner milestone remains full model/typed-resource
construction plus the existing original Player lifecycle. A new test for that
milestone should use retained retail Mario/MarioAnime resources, query authored
landing/hard-landing groups before and after original frame transitions, exercise
four-track basic motion and upper-body override, and verify the same actual
matrices reach both renderer and camera/actor joint queries. Those are future
acceptance criteria, not results of this audit.
