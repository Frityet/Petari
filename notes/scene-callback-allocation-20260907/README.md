# Scene callback Game allocation — 2026-09-07

SceneScheduler now accepts an explicit scene-owned Game allocation domain through
SceneSchedulerAllocationBinding. The binding restores any predecessor on
teardown, and each invocation retains the selected domain until the original
callback returns or throws. SceneObjHolderBinding supplies a separate generic
scene arena, independently of the effect-system and process-resource arenas.
Its owner wiring and typed teardown are coordinated in the scene binding files.

Original movement, animation, clipping transitions, sensor updates/attacks,
messages, model view entry/draw, and draw-category/pre-draw calls enter the scene
allocation scope. Functor registration also captures that domain. Native layout
bridges, scheduler vectors, sorting, copied registration state, trace strings,
and native bookkeeping remain host allocated. Standalone CPU fixtures without
a scene binding preserve the caller's explicitly selected Game domain or its
ordinary host execution context; actual scene owners supply the binding.

Dispatch snapshots now contain registration values instead of pointers into the
mutable host vector. Registration order is never reused, including across
clear(), and callbacks re-resolve that identity before further object access.
This permits registration growth, current/future object removal, and scene clear
without dereferencing invalidated vector entries. Sensor pairs validate their
current host registration and sensor registry identity after callbacks, before
touching either pointer again. Message trace strings are captured before the
receiver can destroy its own or another object's backing.

The existing `smg-pc-scene-scheduler-heap-tests` target now also checks explicit
scene allocation, a different caller heap, nested bindings and dispatch,
exception restoration, draw/pre-draw, 512 original callback-created objects,
movement/animation removal, clear/reconnect, sensor/message recipient destruction,
clipping transitions, removal of a currently executing scene binding, and final
root-heap free-space balance. It retains the earlier host metadata and inherited
allocation-scope regressions. Source and test pass their actual target compiler
flags in syntax mode. The coordinated native build and CPU run passed with all
the scheduler-specific assertions; see `build.log` and `run.log`.

The same target subsequently gained nested Aurora allocation-routing checks:
Game -> Host -> Host -> Client, an explicit second JKR domain inside Host,
exception restoration back to Host, and restoration of the original Game/host
state. That addition passes syntax checking and awaits the coordinated backend
routing integration run; it is not covered by the earlier runtime log.

No original Game source is changed by this scheduler implementation. Native
backend calls from original draws also need host allocation boundaries: Aurora
GPU/FIFO/cache allocations must not retain scene-arena backing. That boundary is
being implemented independently and must be validated before claiming the GPU
scene teardown path is complete. The allocation scope does not replace typed
destructors/finalizers, and JKRSolidHeap still retires original allocations as a
unit; scene budget exhaustion remains real.

`design.md` records the preceding source audit and the lifetime choices that
motivated the implementation. Backend routing and GPU checks will be appended
after their coordinated runs.

Final coordinated allocation validation supersedes the pending notes above: the extended scheduler tests, including nested Aurora Host/Client routing, built and passed. The real-disc effect test also passed both original effect scenes, with host snapshot allocation and access after Game heap retirement. Exact backend commands/results are recorded in `notes/aurora-host-allocation-20260907/integration.json`. The independent intermittent MapAsync shutdown race remains under repair.
