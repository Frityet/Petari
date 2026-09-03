# Retained original player camera target

The native player service now owns a real `CameraTargetPlayer`, bound to
the initialized `MarioActor`. The previous per-frame helper rebuilt a
vector snapshot directly from Mario getters. That skipped the original
target's cached state and movement-timer behavior. The new helper only
constructs and binds the original object; its movement runs in the camera
phase.

`CameraSystemService::begin_frame` advances the selected player target
before calculating its controller. An event selecting a different target
suspends the authored player target. Director pause suspends target updates,
and repeated requests for the same frame cannot advance the target again.
This follows `CameraDirector::movement` -> `CameraTargetHolder::movement`
before camera-manager calculation, with the original SceneExecutor Camera
category before Player. It observes the preceding completed player movement
and the newly published controller input.

Pose reads call the original virtual getters without updating the target.
Attaching, replacing, detaching, or clearing a player withdraws the old
target. A new target publishes no snapshot until its first camera phase.
The camera keeps its last calculated pose while its player is absent.
The service binding is non-owning and shares the RuntimeContext lifetime;
standalone callers must clear the camera owner before destroying the player
service. Actor retirement is handled by `PlayerSystemService::detach_actor`.

`PlayerUtilCompat::getPlayerBaseMtx` can now obtain the actual actor matrix
through the typed player bridge. Mario supplies the original forced
`_EA5`/`_EA8` matrix branch and otherwise `MarioActor::getBaseMtx`; the
generic player service's existing matrix remains available to non-Mario
actors. No guessed bind orientation is introduced.

The actual `CameraParallel` and `CameraHeightArrange` continue to consume
their required fields through the scoped target adapter. The retained
original target also provides camera-area and grounding-triangle identity
for subsequent original camera-selection integration. This change does
not activate the full CameraDirector/CameraManGame catalog or selection.

## Dependency boundary

Original camera-target methods and missing accessors are extracted from
the root into compatibility units with source correspondence recorded
alongside this note. Six Game headers (`BckCtrl`, `XanimePlayer`,
`XanimeResource`, `WaterInfo`, `ResourceHolder`, and `ResourceInfo`) and
the JSystem `JKRFileFinder` header are unchanged root mirrors required by
the declaration closure. All seven were checked byte-for-byte. No Game
algorithm changes were needed for this integration.

The existing PC MarioAnimator initializes its Xanime pointers to null and
uses its current BCK playback path. Water-mode camera virtuals require the
real Xanime construction closure before they can run. Importing their
original query methods supplies the link dependency; it does not claim
that swimming, upper-body animation, or those camera modes now work.
The active parallel-camera path does not invoke those unavailable modes.

## Validation

The live Mario fixture exercises cached versus live getters, bound matrix
axes with a real sensor/Binder host, zero-up normalization, Bee gravity
through the stage gravity manager, and demo last-movement suppression
including u16 timer wrap. It restores all temporary state before walking.
The walk fixture checks that the target sees Mario's timer before Player
movement and that duplicate target advancement for that frame does nothing.

The stage camera fixture checks update counts through pause, event target
selection, no-target retention, detach, and replacement. Event-camera
requests must defer initial calculation until the next camera phase; an
unprimed original target must not be read during the request.

All seven selected targets built successfully on macOS arm64: original
camera runtime, stage-start camera, actor event camera, Mario walk,
showcase, Gateway spin checkpoint, and Gateway demo scene. Runtime results:

- Original camera runtime: passed.
- Stage-start camera: 14 cases passed, including the real-disc authored
  camera and player target phase ownership.
- Actor event camera: 7 cases passed, including an unprimed player target
  requested before the first camera phase and real-disc event resources.
- Real Gateway Mario walk: passed, with 14,521 KCL triangles, source prism
  4642 (`NoSlip`/`Lawn`), 325.684 units of movement, Wait -> Run -> Wait,
  twelve Run draw packets, and actor replacement/lease retirement.
- Real Gateway spin checkpoint: passed, selected progress 5 -> 10 -> 15,
  the existing borrowed-progress-10 fixture, prompt row 22, and three
  placement DemoRabbits. This is the bounded spin fixture, not proof of
  the entire bunny chase or Rosalina sequence.
- Gateway demo scene: passed with the real disc.

All real-disc runs explicitly set `SMGPC_REAL_DISC` to the supplied RMGK01
RVZ. Their outputs and `build.log` remain local in this notes directory.
`original-player-target-walk.png` was captured after frame 157 and visually
inspected: Mario is visible on the planet under the authored camera.
The existing rendering limitations are not an outcome of this camera test.

The source verifier passed 64 methods, with 62 exact copies and two recorded
compiler/character-byte adaptations. `git diff --check` also passed.
