# Deferred event-camera request lifecycle

This checkpoint makes event requests establish ownership and select their target without calculating a camera pose during the request. It closes the first-player-event initialization failure introduced by retaining an actual `CameraTargetPlayer` whose cached fields are initialized in its camera movement phase.

## Source evidence

- Root `src/Game/Camera/CameraManEvent.cpp:47` requests the chunk and calls `CameraTargetArg::setTarget`; it does not call the controller.
- Root `src/Game/Camera/CameraDirector.cpp:116` calls target-holder movement before manager movement and camera calculation.
- Root `src/Game/Camera/CameraAnim.cpp:139` resets `mCurrentFrame` to zero. Its `calc` at line 148 samples the current frame before adding the playback speed.
- Native `RuntimeContext::begin_frame` publishes WPAD input, calls `CameraSystemService::begin_frame`, and then runs scene/player movement. No second scheduler connection is created for `CameraTargetPlayer`.

Previously, `EventCameraRuntime::start` called `calculate_active_pose` immediately. Installing an actual player target clears its initialized-frame marker, so a valid event requested before the next camera phase failed while trying to read uninitialized target state. The existing synthetic fixtures primed their target directly and therefore did not expose that sequence. Production Showcase installs the target after its setup `begin_frame` and before placement finalization.

## Changes

- Every event target kind now uses the same deferred pose lifecycle. Requests validate declaration, supported camera type, and target ownership/reference lifetime before mutating active state.
- Actor/matrix generation checks still reject destroyed objects and same-address reuse during requests and calculation. Missing player target capability still fails during the request.
- Target vectors are read and validated during calculation, after target movement. An installed but not yet advanced player target is therefore a valid request.
- The first event has no event pose until calculation; the game camera remains the visible pose in the meantime. Replacing an event retains its prior visible pose until the next calculation.
- Same-chunk XZ requests preserve the existing original controller and its accumulated state.
- CANM calculation now samples frame zero on the first camera phase, then advances the exposed cursor by playback speed. The request itself does not advance either target movement or the animation cursor.
- Parent integration keys game-camera suspension on active event identity, including an event whose first pose is pending.

## Verification added

`ActorEventCameraTests` now covers an unprimed player target, zero movement and zero target-vector reads during requests, pause before the first pose, frame-zero sampling followed by cursor advancement, retained-target replacement without visible-pose changes during the request, and pause/resume of a replacement event. Existing player/actor/matrix lifetime and failed-request tests remain. Existing immediate-pose expectations explicitly execute a camera phase before inspecting the new pose.

The linear CANM fixture now declares two frames because its two valid samples are used to verify motion. Its old one-frame declaration was inconsistent with expecting playback of both samples.

The updated macOS arm64 build passed. All seven actor event-camera tests
and all fourteen stage-start camera tests passed with the real RMGK01 disc,
including this new deferred lifecycle coverage. The real Mario walk,
Gateway spin checkpoint, and Gateway demo-scene fixtures also passed.
Logs are retained locally under `notes/original-camera-target-player-20260903`.
`git diff --check` passed.

## Remaining boundaries found in the review

`AuthoredGameCameraState` and player event targets retain a raw `PlayerSystemService*`. Production RuntimeContext owns the service for the camera's active lifetime, and Showcase/StageHostScene remove camera bindings or detach targets before destroying actors. Those production paths did not reveal a new dangling pointer. The public service API still permits a shorter-lived external player service to be bound to a longer-lived camera; no lifetime token currently rejects a destroyed service before dereference. A shared service-lifetime handle would close that general API risk without altering Game behavior. Actor detachment and target replacement are already handled by the retained service and do not leave a raw CameraTargetPlayer pointer inside the camera manager.

The preexisting native CANM provider continues to resample/clamp after animation completion with a live target, whereas original `CameraAnim::calc` retains its completed positional pose and only refreshes the final roll/FOV branch. This checkpoint changes request timing and sample-before-increment ordering, not that preexisting terminal animation behavior. Full original CameraAnim execution remains the source-accurate closure for that behavior.
