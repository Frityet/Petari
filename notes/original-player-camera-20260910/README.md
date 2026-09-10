# Original player and camera ownership — 2026-09-10

## Result and source fidelity

This follows the upstream merge at root `827bf39741f55c4c7952a425d54557fd94c57434`, which integrated SMGCommunity/Petari `16906807c69697fdea5d7efbb4642313c1b14204` and reference `ef44fad5dbc7def9d25cf149b823e1cf512bd3e2`. No further reference recovery was necessary for this checkpoint: both complete utility translation units already existed in the merged decompilation.

The native target now compiles the complete original **PlayerUtil** and **CameraUtil** translation units. The archive audit finds **278 unique strong MR providers** there: 173 player functions and 105 camera functions, with no competing definitions. Removed 57 duplicated player wrappers and 86 duplicated camera wrappers across compatibility files. This is provider ownership evidence; unused original functions can still be dead-stripped, so it does not prove every newly available API is fully linked or exercised.

PlayerUtil.cpp is byte-identical to the reference. CameraUtil.cpp retains only a debug-only missing-owner diagnostic and the compile-only null-to-enum correction to GX_PERSPECTIVE (the same numeric zero). MarioAccess adds only a debug-only owner diagnostic. The remaining Game edits restore the original RushEndInfo::_20 member spelling, add an existing original ObjUtil declaration, copy the original MirrorCamera header, and enable the two original source files. There are no new movement or camera algorithms in Game.

Actual MarioHolder/MarioActor/Mario state now owns control, position, velocity, gravity, grounding, visibility, animation and entitlement. Removed the host's parallel hidden/control/swing flags and deferred reset queue. In particular onPlayerControl(true) now executes the original resetCondition synchronously; setPlayerPos updates the original actor/internal/camera/sensor fields; velocity and gravity queries borrow the original selected storage. Native attachment, synchronization and retirement no longer override gameplay state.

Camera projection/unprojection and matrix/basis queries now read the actual original CameraContext, including shake, depth and console dimensions. CameraDirectorRuntime readiness observes the original sorted chunk holder. Native CANM byte retention remains at the host resource boundary. The missing mirror actor remains an explicit unavailable-owner boundary; it is not replaced with a fabricated matrix. Original GCapture's exact query is retained separately until its complete actor TU is activated.

## General compatibility fixes

Aurora `7f93df56a1053deb9b2572a1af8fd4044c963078` corrects WPADProbe to return signed SDK results (0 for connection, -1 for absence) and the actual core/not-found device codes. Five new regressions fail against the previous boolean implementation; all eight standalone input tests pass after the correction. No keyboard mapping or extension-device simulation was added.

The spin fixture exposed two real scene/process lifetime bugs. Retained semantic trace vectors/strings and WipeService history/current names were being allocated from the active Game heap. They now allocate under the generalized host allocation scope, so scene retirement cannot invalidate process-owned history. LLDB first-throw stacks identified both faults. A dedicated regression destroys the original scene and root heaps before inspecting and reusing retained wipe data. The actual-player regression also checks trace storage under a selected Game heap.

Mario-puppetable demos now preflight the actual MarioHolder actor against the attached native player before replacing any existing demo, then use original off/on control APIs. The broader demo adapter remains a future source-restoration target.

## Validation

The optimized debug showcase is `89b80b085aa6d0f9949e3db3e26b7c817e9bb182e12402ea1fceef2a586a2488`. A real-disc 1280x720 replay completed 960 ticks and exited 0 at **59.9846 FPS**. **All 13 checks pass:** zero idle drift, each WASD key and release accepted, both jumps land, and all movement/camera values remain finite. These are synthetic SDL input events through normal input publication, not physical keyboard testing or direct Mario state changes. See demo-run.json and demo-validation.json.

Five original camera/player owner suites pass. DemoSceneRuntime reports 20/20 tests passed, with its optional real-disc definition check skipped in that invocation (the separate prompt/spin tests use the actual disc). The retained wipe allocation regression passes. The actual bounded spin checkpoint passes, including original prompt timing/story unlock and clean actor/scene/process teardown. The actual PlayerUtil regression passes in two fresh processes, exercising original vector/grounding/translation/control-reset semantics, draw-buffer ownership and complete scene retirement. InformationObserver passes its exact prompt/guard/fresh-A/story-unlock checks and strict scene/process teardown checks. In total, all ten focused executables selected for this checkpoint pass; see validation-summary.json for the distinct runs and their limits. The test fixtures are being migrated to actual original startup, scene execution lists, clipping rules, model draw buffers and scene-owned raw children; obsolete synthetic ownership assumptions are not production contracts.

Full reference CameraUtil compiles with the Wii compiler. Among 25 sampled context/projection methods, 22 match 100%; the other existing reference scores are 99.87342%, 99.93827% and 84.5098%. This is not a claim of whole-TU perfect matching. Final independent source/provider/ownership review found no new material regression in this production cohort; see final-production-review/.

## Remaining work and limits

- The full Gateway bunny chase and Rosalina appearance remain incomplete. The spin checkpoint is a bounded scene fixture with an explicit progression setup, not proof of the full original sequence.
- Original GameSequenceProgress::startScene is not yet the native startup owner. Original Mario's constructor permits swing; the full game locks it from story state at scene start. Focused locked-spin fixtures therefore establish that precondition through the original MR API. Recreating a host entitlement flag would hide this missing sequence integration.
- Original WPad/GamePadUtil still needs complete KPAD/Nunchuk records and the missing original WPadStick stores. WPADProbe alone does not complete that migration.
- Sky's unavailable SpaceInner/MirrorReflectionModel children and native LightDirector/resource ownership remain restoration candidates identified by the upstream review.
- Three unused PlayerUtil imports still require real owners/recovery when activated: findNamePosOnGround, resetChasingStarPiece and the named actor stopSound overload. They have no no-op replacements.
- The default legacy Mario walking fixture still contains obsolete renderer-packet probes and same-scene replacement assumptions. Its focused --player-util route passes; the separately validated SDL replay provides current walking/jumping/camera evidence. The default route is not claimed to pass.
- The broad Aurora native suite is not green. It still contains stale scene/scheduler/model fixtures and stops at a model-less FixedPosition/PartsModel test (C_MTXCopy destination 0x34). The focused tests and live replay above do not hide or establish a pass for that larger suite. Detailed failures and the stopped stack are retained locally.

User-staged documentation deletions, the unrelated sequence-publication note and untracked package_walking_demo.py remain outside this checkpoint. Commits use codex for both author and committer; Aurora is published before the root gitlink. The movement demo is refreshed from the exact replayed binary without rebuilding.
