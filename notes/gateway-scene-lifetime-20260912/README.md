# Scene retirement across native workers — 2026-09-12

The original GameSystemSceneController initializes and destroys scenes through
FunctionAsyncExecutor workers. Native SceneLifetimeBinding registrations were
thread-local, so a different destruction thread silently skipped the scene's
native retirement callbacks. The registering thread exiting also stranded the
list. This is a scene ownership defect independent of any stage or actor.

The registry now retains bindings by Scene identity across native threads. A
mutex protects only intrusive-list registration, duplicate checking and removal.
The callback is unpublished and the mutex released before calling the owner;
self-removal, dependent-service cancellation and reverse registration order are
preserved. No Game source changes, replacement Scene, guessed nerve state or
actor-specific behavior are involved. Scene owners still must serialize access
to their individual Scene and service lifetimes.

Validation: `xmake build -j 8 smg-pc-scene-lifetime-binding-tests` passed, and the
binary exited 0. The existing original Scene/JKR test now registers services on
a native worker, lets that worker exit, checks duplicate rejection from the
main thread, and destroys the actual derived Scene on another worker. It checks
derived/base/service ordering, self-removal, full original arena reclamation
over 32 lifetimes, and cross-thread cancellation without disturbing another
Scene. `build.log` and `test.log` retain local output.

This closes retirement-registry thread affinity only. GameSceneBinding,
SceneExecutionBinding, StageInitializationService, SceneNameObjRegistry and the
stage resource/session bindings still need a coherent process/scene ownership
activation path. Full GameSystem/GameScene startup and Gateway's bunny chase
through Rosalina remain incomplete; this test does not claim gameplay progress.

Initial unrelated changes were preserved: staged DISCREPENCY_REPORT.md and
MACOS.md deletions; edits to original-sequence-galaxy-move notes,
src/Game/LiveActor/LiveActor.hpp and src/Game/xmake.lua; and untracked
script/package_walking_demo.py. Other agents are preparing separately reviewed
NPC item, ClipArea filter and stationed layout compatibility changes.
