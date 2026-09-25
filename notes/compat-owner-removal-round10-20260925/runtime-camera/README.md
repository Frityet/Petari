# Runtime camera closure

Deleted CameraSystemService completely: its authored/start/event/programmable controllers, duplicate shake math, target publication, host counters and retained wrapper state. All Game-facing camera APIs already use CameraDirector/CameraContext. The only production consumers outside the retired wrapper lane were RuntimeContext presentation and stale ParityTrace fields.

RuntimeContext now reads the existing CameraDirectorRuntime presentation adapter directly at frame start and refresh. SceneScheduler still refreshes after the actual Camera movement category. Manual set_scene_camera_pose remains a direct render-pose input; a refresh without an actual camera owner does not erase that input. Debug freecam remains authoritative while enabled. No host controller runs a substitute movement/update phase.

Removed PlayerSystemService's camera target injection/advance/readback state, whose only production consumer was retired EventCameraRuntime. Actual CameraTargetHolder/CameraDirector keep original target movement and selection. Other player actor and matrix bridge behavior remains unchanged.

ParityTrace now reports effective_camera_pose from the actual camera adapter and presented_camera_pose from RuntimeContext (including legitimate manual/freecam presentation). Obsolete service counters, fake programmable status and host shake events are deleted rather than filled with placeholder values.

Five source files changed: RuntimeServices.cpp/.hpp, RuntimeContext.cpp/.hpp, ParityTrace.cpp. Both RuntimeContext files were initially dirty; before/ and runtime-camera.patch preserve those edits and isolate this lane. Added a direct JGeometry vector include after removing transitive wrapper includes. No build/wiring, tests, staging or commits performed. Gateway lane owns 19 camera source deletions/restoration; merge_audit owns obsolete fixture cleanup. No new files or services replace the removed camera state. Source frozen for root integration.

## Isolated-index adaptation for retained HEAD-only draw entry

The initially dirty working tree had already deleted RuntimeContext::draw_3d_normal(), so the lane's before-to-after patch cannot update its old _camera_system access. Preserve HEAD's draw entry when staging and apply staged-only-legacy-draw.patch after the normal lane patch: change only its conditional to current_camera_director_runtime() and its assignment to camera->pose(). The CameraDirectorRuntime include is already added by the main patch. Do not absorb the unrelated dirty draw-entry deletion. No working source or index was modified for this staged-only recommendation.
