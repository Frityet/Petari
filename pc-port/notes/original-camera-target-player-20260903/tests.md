# Original CameraTargetPlayer regression fixture

`tests/MarioCameraTargetTests.cpp` executes the actual imported `CameraTargetPlayer::movement` and original getters against the live, initialized Mario from `MarioGatewayWalkTests`. It does not replace gameplay code or construct a substitute player.

The root hook is `smgpc::tests::verify_original_mario_camera_target(*actor, runtime.dvd(), scene.demo_runtime())` after Mario init and placement finalization. Add the helper CPP alongside `MarioWalkParameterTests.cpp` in that target.

The scoped fixture saves/restores all touched actor flags, mode, movement timer, rush flag, up/camera-position/last-move vectors, Mario orientation/air-gravity/ground/shadow vectors, movement-state bitfield, active state pointer, and rush sensor pointer. It does not rewrite the live base matrix. The `_934` bound path uses a test-local real LiveActor/HitSensor/Binder relationship with no ground contact, so `getGroundingPolygon` returns null. No unavailable MarioWait object or fake Mario state is constructed.

Assertions distinguish cached fields from live getters: dead/clipped Mario retains orientation, ground/gravity, and timer; camera position remains the current actor camera position. Only up is normalized, and a zero up uses the original Y-axis fallback. The bound path must use the published original player matrix columns instead of normal getter vectors.

For demo suppression, a temporary `DemoSceneRuntime` overlays the current scene using a single test DemoGroup that references one already loaded real time sheet. It has no registered actor casts and is never advanced, so sheet actions/cameras/player rows and gameplay callbacks are not dispatched. The fixture starts/stops the clock only to exercise `MR::isDemoActive`, and destruction restores the previous runtime stack. It verifies unchanged timer suppresses stale last move, u16 wrap is recognized as movement, repeated getter queries do not mutate the movement flag, and a second movement call with the same timer clears it. That last behavior is why the owning service must advance exactly once per camera phase.

`tests/CameraTargetTestSupport.hpp` supplies an explicit data-backed `CameraTargetObj` for existing non-Mario camera integration fixtures. It is owned by `PlayerSystemService`, records movement count, and requires explicit ground/gravity. ActorEvent, StageStart, and Gateway sentinel fixtures install it and perform initial advance(0).

No builds are run by this subtask. Parent task serializes compilation/runtime verification and records results separately.
