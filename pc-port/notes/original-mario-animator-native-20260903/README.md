# Complete original MarioAnimator staging and owner frontier

The complete root `MarioAnimator.cpp`, `MarioAnimator.hpp`, and authored `MarioAnimatorData.hpp` are staged under `build/original-mario-animator-native-20260903/`. The whole animator translation unit compiles with the actual `smg-pc-game` compile-command flags taken from `Game/Animation/XanimePlayer.cpp`, using the parent's staged complete ModelManager headers and the other worker's staged complete J3DModelX header. This task makes no production source or build-file changes and performs no shared xmake/GPU or live animator run.

The original `MarioAnimationEfx.cpp` also compiles unchanged, supplying all callback methods, the actual callback table, and Luigi swaps. The remaining two Animator methods in `MarioWait.cpp` compile when extracted verbatim into an isolated translation unit. The entire MarioWait source currently fails because `Mario::checkSpecialWaitAnimation` is defined `void` but declared `bool` in root and native Mario.hpp. That unrelated state-source inconsistency was not hidden by changing a signature or supplying a replacement method.

## Mechanical adaptations, with original compiler evidence

Only two changes are necessary to compile/use the staged main Animator source on this host:

1. Add a lexical block around the local declarations/rotation branch in `updateJointRumble`, so its earlier gotos do not bypass initialized locals. This is the existing PC compilation accommodation, applied to the original body without its native animation replacements.
2. Replace the raw `(u8*)joint + 0x54` store in `calc` with `joint->mMtxCalc = nullptr`. Retail `0x802CC9FC` is `stw r5,0x54(r3)`, after loading zero at `0x802CC9F0` and indexing the actual J3DJoint array. The original compiler verifies `offsetof(J3DJoint,mMtxCalc)==0x54`; native Clang verifies `0x68`. A raw +0x54 store would corrupt the native joint's bounds, not clear its calculator.

`prove.py` compiles the unmodified complete root TU and adapted complete TU with real GC3.0a3 `cflags_game`, using real root headers. Both affected methods have identical instruction bytes and identical relocation types, positions, target functions, and constant bytes. Only generated local constant label numbers are normalized for this source-to-source comparison. This proves the adaptations preserve the current root algorithm on Wii; it does not claim all inherited root source already matches every retail method.

A diagnostic objdiff of all 42 original Animator methods is retained in `inherited-root-objdiff.json`: init 97.02%, update 93.12%, calc 89.81%, rumble 90.94%. Some other inherited methods are lower. For example, `getUpperJointID` returns the same unsigned joint lookup but the current source emits a stack frame and redundant 16-bit clear instead of the retail tail call (the actual lookup already clears its result to 16 bits). The typed field change is not responsible for those existing differences. `adaptation-evidence.json` records the bounded equivalence proof and hashes; no complete fresh decompilation of this 1,300-line class is claimed.

## Complete class link result

`link-probe.py` retains the entire adapted Animator object, the entire original callback object, and both exact Animator Wait methods, with dead stripping disabled. It links against the existing showcase SDK/Game/walking-slice libraries. **No MarioAnimator method remains undefined.** There are **26 direct unresolved owner/provider dependencies**, and 202 unresolved symbols when the pre-existing transitive whole-object gaps of the walking slice are also retained. This latter number is a compiled-library diagnostic snapshot, not a count of newly introduced or undecompiled functions.

`complete-class-link-frontier.json` records each missing symbol and the actual referencing Animator method. The earlier main-TU-only snapshot has 30 direct edges, including callback methods and Luigi swap data subsequently supplied by the original callback TU.

| Dependency group | Actual root source / owner | Status |
| --- | --- | --- |
| Model/resource/joint access | ModelUtil.cpp `getJ3DModel`, `getJ3DModelData`, `getResourceHolder`, `calcJ3DModel`; JointUtil.cpp `getJointIndex`; LiveActorUtil.cpp `isBckPlaying` | Existing root bodies. Require the actual attached ModelManager, same retained model/data, actual lower player, and original recursive MutexHolder gate. |
| Rotation/blending math | MtxUtil.cpp `tmpMtxRotYRad`, `orderRotateMtx`, `blendMtx` | Existing original helpers absent from the compiled native link; retain their temporary-matrix and arithmetic semantics. |
| Eye pattern | MarioActorEye.cpp `setBlink` and normal update lifecycle | Existing source; reads actual animation ResourceHolder BTP table and mutates the original pattern's frame. |
| Special-mode animation | MarioActorParts.cpp `changeSpecialModeAnimation`, `updateSpecialModeAnimation` | Change exists. **Update body is missing root**: `0x802BCD08`, size `0x1E0`. It is called even on the Animator normal update path and must be recovered, not replaced by an empty special-mode hook. |
| Carried/null model | MarioActor.cpp `changeNullAnimation`, `clearNullAnimation`, `isStopNullAnimation`; MarioActorParts.cpp `offTakingFlag`; MarioActorTakeMsg.cpp `rushDropThrowMemoSensor` | Existing root bodies. Require the actual null-animation actor and carry/sensor lifecycle on those original branches. |
| BAS animation | MarioSound.cpp `startBas`, `skipBas`, `isRunningBas` | Existing root bodies. `startBas` checks mSoundObject; **skipBas dereferences it unconditionally**. An absent sound object is insufficient for every normal walk transition. Preserve original audio-animation state even when audible output is omitted. |
| Movement information | MarioWall.cpp `checkStickFrontBack`; MarioCollision.cpp `getWallNorm` | Existing source; real player wall/contact state required. |
| Talk and face targeting | DemoUtil.cpp `isNormalTalking`, `getTalkingActor`; MarioActorMatrix.cpp `getFaceLookHeight` | Existing source. Talk helpers explicitly return absence if TalkDirector does not exist; face lookup uses actual actor JMapInfo `_FC8`, falling back to150 only when the authored key is absent. |
| Chest joint constant | declaration in MarioAnimator.cpp; retail `.sdata` at `0x806B2288` | Missing root definition. Verified retail pointer is `0x805C3FDA`, bytes `Spine1\0`; must restore actual data ownership, not introduce a different joint name. |

The current native actual XanimePlayer, XanimeResourceTable, XanimeCore, typed J3D animation, resource holder, and SDK model methods resolve their direct Animator symbols in this probe. A successful link would still need the actual model/player/actor owners initialized before calling any Animator method; nothing in this test constructs an incomplete model or forged actor.

## Original owner and execution sequence to retain

1. `MarioActor::init2` (`MarioActor.cpp:282`) invokes `initDrawAndModel`, allocates its joint matrices, and then constructs MarioAnimator. Actual ModelManager construction keeps separate model and MarioAnime resources. Its `getResourceHolder` returns the animation holder when its original Xanime table exists. Model/data/motion lifetimes must cover both lower and upper players.
2. Animator init builds the exact authored resource table, including aux/offset/single/double/triple/quad tables and Luigi swaps. The `基本` group is **WalkSoft, Walk, Run, Wait**, initially weighted **0,0,0,1**. It initializes callbacks, constructs the lower player, sets/changes its default `基本`, enables joint transforms, installs that lower player in the real manager, constructs the upper player, selects its default, and shares joint transforms. No native Run/Wait branch is present in the staged source.
3. The authored arrays are mutable globals: XanimeResourceTable writes their resource pointers, hashes, weights, and auxiliary links. They are not interchangeable per-model scratch tables. Native ownership must retain their referenced resources through every player using them and avoid cross-lifetime mutation hazards. Existing allocator/domain ownership must also retain the manager's original initial player/model when the Animator replaces the manager's player pointer.
4. Original LiveActor movement calls ModelManager update first. It advances lower `updateBeforeMovement` then `updateAfterMovement`. MarioActor's movement invokes Animator update when `mDrawStates._1A` is set and consumes that flag (`MarioActor.cpp:829`). Animator update retains callbacks, original walk/squat/ice/braking choices, tilt/homing/hands and special-mode processing; it advances the upper player only when `_6C` is active. The renderer must not add another lower-player tick.
5. Animator calc installs the lower/upper calculators at the actual root, Spine1 and Head joints. With no upper layer it clears the Spine1 calculator through the typed field. It sets lower core `_6=1`, calls `MR::calcJ3DModel`, then `_6=2` and calls it again, finally clearing the installed calculators. `MR::calcJ3DModel` directly locks the original gate and invokes the same actual model's `calc`; it is **not** ModelManager::calc, which itself installs/clears animation and would interfere with this sequence. The renderer and camera joint queries must consume these resulting model matrices, rather than install another one-track calculator.

Subsequent root-only work has now restored the special-mode update method and chest data with current-retail proof; see `../original-mario-special-animation-20260903/README.md`. The compile/link frontiers above retain their earlier snapshot, before that restoration and before any native import. The next coherent native activation remains attached full ModelManager/model ownership plus the original actor callback/BAS/blink and movement dependencies above, followed by restoring original init/update/calc together. The staged class must not be activated piecemeal merely because it compiles.

## Reproduce

```sh
python3 pc-port/notes/original-mario-animator-native-20260903/prepare.py
python3 pc-port/notes/original-mario-animator-native-20260903/prove.py
python3 pc-port/notes/original-mario-animator-native-20260903/link-probe.py
```

The expected baseline and full MarioWait failures are recorded deliberately. Adapted Animator, callback TU, and exact Animator Wait extraction must compile. The link probe reports unresolved external owners and is never run. Parent/gateway staged ModelManager/ModelX headers and current compiled showcase libraries are required; every output remains under the ignored build directory. No shared build or production change is performed by these scripts.
