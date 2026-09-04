# Scheduler metadata survives scene heap retirement

## Failure and correction

The staged complete original EffectSystem successfully entered with the real
RuntimeContext capture texture, created resource-backed emitters, and advanced
eight frames. Teardown then aborted in `deallocate_jkr_or_host`: scheduler
execution-history strings had been allocated in the active scene arena. The
arena retired before RuntimeContext cleared its persistent scheduler history.
The retained strings consequently no longer had live allocation provenance.

`SceneScheduler::push_trace` now creates both records and their strings inside
a host-allocation scope. Message history, returned snapshots/layout diagnostics,
returned registration records, and temporary sorted pointer batches follow the
same metadata ownership rule. Scopes end before actual movement, animation,
draw, message dispatch, and registration-removal callbacks. No whole execution
phase is placed in host allocation mode. There are no Game source changes. Temporary actor/sensor/message pointer lists
also grow under narrow host scopes: freeing their native vectors cannot reclaim
space from a JKRSolidHeap, so per-frame arena growth must be prevented.

## Validation

The expanded existing `smg-pc-scene-scheduler-heap-tests` target passes. It runs
128 actual NameObjs through movement and animation inside a real JKRSolidHeap
domain, retains 256 trace records plus a snapshot, and checks that neither the
containers nor long name strings use Game storage. Registration, sorting,
tracing and snapshots consume no additional arena space. The test retires the
domain, allocates/overwrites another arena, then reads and destroys the retained
metadata. A separate actual NameObj allocates from both virtual movement and
animation methods and verifies that those allocations still use the caller's
Game heap. This protects against fixing lifetime by accidentally rerouting
original Game callbacks. A further fixture runs two actual native LiveActors with
real registered HitSensors through 32 movement frames and 64 received messages,
using the actual MessageSensorHolder. Those scratch lists and persistent message
traces consume no Game arena storage; actor and message callbacks all run, and
trace names survive arena retirement. This verifies native metadata ownership,
not full original actor physics or collision behavior.

After rebuilding normal libraries, the unchanged staged original EffectSystem
runtime fixture was relinked and passed two complete actual RuntimeContext/
Metal initialization and teardown cycles. Each uses real particle resources,
swaps IndDummy to the actual screen ResTIMG, creates emitter groups 0/1/7/8,
advances eight movement/animation frames, rejects capacity overflow, reuses a
freed emitter, and restores root-heap and NameObj baselines. Its reported scene
allocation fell from 19,208 to 8,048 bytes after removing native scheduler
metadata from the Game heap. The fixture's original effect TUs are instrumented
with ASan/UBSan; rebuilt existing shared archives are normal debug builds.

No particle draw was invoked and the full original actor EffectKeeper/player
pipeline is not activated by this scheduler fix. The renderer logs a pending
buffer-map cancellation/device destruction at final shutdown; the process exits
successfully with both ownership cycles passed and no sanitizer report.

## Reproduction

```
cd pc-port
PATH=/opt/homebrew/opt/llvm/bin:$PATH xmake build -j8 smg-pc-scene-scheduler-heap-tests
./build/macosx/arm64/debug/smg-pc-scene-scheduler-heap-tests
```

The renderer fixture lives in `../original-effect-system-native-20260903`.
After staging/compiling that separate proposal, relink from the repository root:

```
PYTHONDONTWRITEBYTECODE=1 python3 pc-port/notes/original-effect-system-native-20260903/link-runtime.py
SMGPC_REAL_DISC='/path/to/disc.rvz' ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./build/original-effect-system-native-20260903/runtime-probe
```

`failure-backtrace.txt` identifies the original ownership failure. `run.log`
and `runtime-effect-proof.log` preserve the successful checks. Full compiler
warnings and generated objects remain in ignored storage.
