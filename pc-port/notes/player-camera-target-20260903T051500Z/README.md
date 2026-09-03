# Live player camera targets

The event-camera player target now queries the attached actor's camera capability when sampled. The previous path reconstructed the target from `PlayerSystemService::base_matrix()` and its general velocity snapshot. Those fields can differ from the actual Mario camera position and last move, and the path omitted the jump flags consumed by the newly restored original `CameraHeightArrange`.

## Changes

- `PlayerActorBridge` replaces the narrow entitlement-bridge name and adds `read_camera_target`. `PlayerSystemService::camera_target_state()` returns unavailable unless its current actor has that capability. The callback is evaluated for each query; it does not retain a second stale camera snapshot.
- `EventCamera` uses the camera position, independent side/up/front axes, last move, available ground/gravity vectors, and jumping/fast-rise/fast-drop flags. Player targets with only a base matrix fail explicitly. Detachment removes the callback and retained event targets fail on their next calculation.
- The concrete Mario adapter uses the original camera translation/orientation/getLastMove and jump-state getters. Its up vector follows `CameraTargetPlayer::movement` normalization and zero-vector fallback. Ground position now uses `getShadowPos`, and gravity uses `getGravityVector(&out)`. The latter out-parameter getter returns air gravity; it is distinct from `getGravityVec()`.
- The exact out-parameter gravity accessor is supplied in `MarioStateAccessCompat.cpp` while its larger original translation unit remains excluded. Production Mario owners install the typed callback at attachment. Generic test actors install explicit fixture capabilities; no actor-name detection or generic grounded-player fallback was added.
- `StageCameraTargetState` can retain an independent side vector for animated cameras. Simple external target fixtures that omit it continue to derive side from up cross front.

## Original correspondence

`src/Game/Camera/CameraTargetObj.cpp`: `CameraTargetPlayer::movement`, `getPosition`, `getLastMove`, and jump/rise/drop getters. `src/Game/Player/MarioActorCamera.cpp`: separate camera translation, up, front, side, last-move getters. `src/Game/Player/MarioActor.cpp`: `getGroundPos` returns `mMario->mGroundPos`, while `getShadowPos` returns `mMario->mShadowPos`. `src/Game/Player/MarioActorGravity.cpp`: out-parameter `getGravityVector` reads air gravity. `src/Game/Camera/CameraHeightArrange.cpp`: `updateJump`, `checkState`, `updateUpper`, and `updateLower` consume these published flags.

## Validation

`ActorEventCameraTests.cpp` adds an explicit live camera fixture and tests unavailable capability despite a render matrix, actor camera translation/orientation distinct from render state, independent side-axis use by the following CANM frame, preserved geometry/jump flags, fresh callback sampling, and detachment rejection. Existing CANM/real-disc event tests now install explicit capabilities. Existing original-controller height tests cover different jump/fast-rise/fast-drop trajectories.

The final Apple Silicon build and six ActorEventCamera test cases passed with
the supplied RVZ on 2026-09-03, including the controlled same-XZ re-request
regression. The real Gateway walking fixture also passed with the concrete
Mario callback and distinct shadow/ground, normalized up, and side assertions.

## Remaining boundary

This is the normal, unbound Mario target-data boundary. Original `CameraTargetPlayer` lifecycle still includes retained orientation while dead/clipped, bound-player matrix selection, Bee gravity queries, and demo movement-timer suppression of stale last-move values. Those behaviors require a real retained target owner and are not represented as completed by this callback tranche. Ground triangles/area selection and additional camera modes remain separate compatibility work.

## Re-requesting the active XZ event

A repeated start for the same active static XZ chunk now retains its original controller and pose. It validates the target before mutating active or retained target state, updates the request fields, and waits for the next movement frame to calculate. `CameraManEvent::start`/`checkReset` request a reset only when the current chunk or camera type changes. This does not implement FIFO priorities or interpolation.

The optional real-disc ActorEvent camera case clones its loaded catalog and controls one XZ fixture's round/height parameters. Two actual original controllers run matching height chase and right-pad rounding trajectories; one receives a repeated start. Its immediate pose must remain unchanged and subsequent frames must equal the uninterrupted control. A failed replacement player target must leave both current pose and retained target intact. The retail catalog is not modified.

## Follow-up target ownership

The callback boundary described above was subsequently replaced by a persistent, service-owned actual `CameraTargetPlayer` in the same work session. See `../original-camera-target-player-20260903/` for source correspondence and live lifecycle regression coverage. The original normal/unbound-only limitation above describes the interim callback checkpoint, not the final retained target owner.
