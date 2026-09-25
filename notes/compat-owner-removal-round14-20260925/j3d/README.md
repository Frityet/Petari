# J3DSys owns its native shared contexts

Removed compat/J3dCommandScope.cpp/.hpp and compat/SceneJ3dScope.hpp. CommandScope and ContextScope are actual J3DSys APIs. The SDK owns the shared command mutex; original MR::MutexHolder<0> aliases that mutex, as its current-heap counterpart already aliases JKRHeap. No independent compat state or forwarding path remains.

Preserved the existing heap-then-command lock order, guest execution scope, GD current-list pointer, interrupt state and exception-only recursive lock recovery. ContextScope preserves all previous process J3D, matrix calculator/joint, scale, matrix and texture-coordinate state around nested phases. Call sites and existing fixtures use the SDK API directly; no new tests.

Snapshots for LiveActor were taken after the concurrent effect owner edit froze; the effects lane retains that file's original batch baseline. RuntimeContext and several fixtures had preexisting dirty work. Only mechanical API/include replacements are staged onto HEAD there, including stale references in already locally removed methods.

Validation is the batch app build and 120-frame opening run only; individual scheduling fixtures are not rerun.
