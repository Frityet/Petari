# Remaining duplicate native scene wipe consumers

Read-only audit after original SceneWipeHolder activation. No migration of these consumers has been saved during the shared build freeze.

The new17 Game-facing scene APIs use the actual holder. The old native scene WipeService still owns an independent timer and must be retired. It currently normalizes every negative duration to30, whereas original ring and Koopa have their own90-frame defaults and GameOver is driven by its real BRLAN. Its state/event stream is therefore not evidence that an original wipe ran.

Active production consumers:

- `src/compat/DemoSceneRuntime.{hpp,cpp}`: optional WipeService constructor injection; fallback to RuntimeContext::scene_wipe; Impl retains its pointer; first-step DemoWipe rows dispatch four kinds to the timer service. This should dispatch unchanged row names/raw frames to the actual WipeHolderBase/SceneWipeHolder open/close/force operations. Keep the original unknown-kind no-op; do not permit an unknown named wipe to fabricate a successful state.
- `src/scene/GatewaySpinCheckpoint.{hpp,cpp}`: constructor injects/stores WipeService and try_rosetta_trigger calls its close method for the existing route's fade handoff. This is already a bounded route fixture, not general Game logic. Replace that call with actual scene wipe owner dispatch and remove the service member/constructor parameter. The existing90-frame route handoff wait remains a separate route action, not proof of fade duration.
- `src/showcase/Showcase.cpp`: passes runtime.scene_wipe to GatewaySpinCheckpoint; remove that argument after the constructor migration.
- `src/runtime/RuntimeContext.{hpp,cpp}`: owns `_scene_wipe`, exposes const/nonconst scene_wipe, ticks it every host frame, and includes its synthetic state in semantic sequence traces. Remove this scene field/API/tick and report actual SceneWipeHolder availability/name/original predicates in traces. No estimated remaining frames should be emitted as if observed from original animation state.
- `src/runtime/ParityTrace.cpp`: serializes scene and system WipeService into the same state/duration/event shape. Replace scene branch with actual typed holder observations; keep unavailable explicit when no real scene owns a holder. Existing process/system state remains a separate pending owner closure.

The generic WipeService class and WipeState/WipeEvent types still have an active **process/system** consumer: RuntimeContext::_system_wipe plus `src/compat/ScreenSystemAndCaptureCompat.cpp`. The latter is the unchanged remaining old native system-wipe/capture API relocated out of Game during duplicate-symbol correction. Retiring the scene field must not silently claim to have restored the actual original SystemWipeHolder or capture graph.

Tests requiring migration:

- `tests/DemoSceneRuntimeTests.cpp`: two WipeService constructions, one action-order fixture injection, and one standalone arbitrary-name/raw-frame dispatch fixture. Use actual WipeHolderBase/WipeFade owners for a typed CPU dispatch seam or move scene-owner integration to the actual-RVZ wipe fixture; never retain a fake successful arbitrary-name timer as a production provider. The no-owner fixture must continue to reject a dispatchable row.
- `tests/GatewaySpinCheckpointTests.cpp`: passes scene_wipe and checks its event log. Those assertions must inspect actual original owner/name/transitions or source dispatch evidence; the old event stream cannot demonstrate rendering/nerve execution.

Suggested smallest migration boundary: a native dispatcher accepting an actual WipeHolderBase reference, forwarding the four original kinds to its actual virtual methods. Production resolves the active SceneWipeHolder; focused CPU fixtures can instantiate the real original base holder with real WipeFade children and arbitrary authored names within its capacity. This keeps actual Game objects and behavior as the only state owners while allowing independent parser/action tests. Remove the native timer seam rather than mirror events into both systems.
