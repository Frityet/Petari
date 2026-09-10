# Gateway spin fixture owner refresh — 2026-09-10

The original PlayerUtil/CameraUtil activation makes this fixture exercise the real Mario reset/control APIs. The previous setup had neither completed process resources nor a scene heap owner for Mario and his constructor/init children.

Changes are confined to `tests/GatewaySpinCheckpointTests.cpp`:

- Include the actual PlayerUtil declaration for `MR::isOffPlayerControl`.
- Complete scenario and particle resource publication before constructing GatewayDemoScene, matching Showcase.
- Use the active scene allocation domain and NameObjChildOwner for actual Mario construction/init/retirement; construct and initialize during the original player-placement phase. The read-only player bridge includes actual center and nerve eligibility, as in Showcase.
- Keep the exact StartInfo camera-resolution assertion while using the scene's original CameraDirector for execution. No test-owned cached camera pose is installed.
- Open only the renderer setup frame before placement initialization. No RuntimeContext frame runs until finalization has completed the actual executor lists.
- Move the test stimulus through `MR::setPlayerPos`, whose original `MarioAccess::setTrans` updates both MarioActor and Mario positions, camera position and hit sensors. This replaces a write to only the actor position.
- Retain the exact authored rabbit identities/rows, fade and demo timing, prompt/entitlement and LOD lifetime assertions. Explicitly verify owner retirement detaches Mario and clears the actual MarioHolder.

Existing changes already present at task entry replaced removed PlayerSystemService shadow flags with actual original control/entitlement checks; this task preserves them.

Validation: isolated native syntax check exit 0; command and frozen source SHA in `syntax.json`, warnings in `syntax.log`. Root owns linked build and real-disc runtime validation; no runtime pass is claimed here. No production files or build configuration were changed and no commits were made by this task.

## Locked spin precondition

The first linked run built successfully and reached only the initial locked-spin assertion before failing. `MarioActor` construction sets `_EEB = 1` (native source line 147). Retail `GameSequenceProgress::startScene` disables it through `MR::setPlayerSwingPermission(false)` when the `スピン権利` event is absent (reference lines 118–120). That full sequence owner is not yet activated by this bounded demo scene.

The fixture now asserts that the story entitlement is absent and establishes its locked precondition through that existing original player API after actual Mario initialization and placement finalization. It neither restores a host shadow flag nor claims full GameSequenceProgress startup coverage. The production full sequence dependency closure remains outstanding. Updated isolated syntax passes; root will rerun the linked fixture.

## Original observer timing and secondary teardown failure

The next linked run failed at the post-guard acceptance assertion, then aborted during cleanup. First-throw LLDB (`lldb-first-throw.log`) identifies that assertion before any provenance failure. The live observer state at frame 1792 is `mDisplayFrame == 0` while `MR::testCorePadTriggerA(0) == true` (`lldb-display-state.log`); both expressions succeeded before the final unrelated expression typo.

Category values are identifiers, not ordering: original SceneExecutor actually runs NPC/category 40 before Layout/category 14. `InformationObserver::entry` calls `makeActorAppeared`, whose `connectToSceneTemporarily` queues the original NameObjExecuteInfo connection. This occurs after that frame's post-ClippingDirector connection phase. The observer therefore first executes after the next connection phase: delegation at frame 1762, first `exeDisp` at frame 1763. The aggregate scheduler follows the same phase ordering. Original InformationObserver sets 30, decrements, then requires a value below zero. Display execution 30 at frame 1792 is still guarded. The fixture now explicitly tests that fresh A is rejected, releases A during execution 31/frame 1793, and sends a new edge on execution 32/frame 1794 to accept. All 1670 pre-prompt ticks and authored dispatch assertions remain exact.

The parent's separate `../spin-backtrace.log` identifies the secondary panic as native RuntimeContext::SemanticTraceEvent string storage allocated under the scene Game heap and released after that heap. Parent owns the general host allocation boundary correction; this task changes no production code. Updated syntax passes. Linked full checkpoint success remains pending root rerun.

## Second teardown sink after semantic trace fix

Two sequential LLDB runs of the newly built binary `55a998c4f5b046324173b2f83f39df95b1f37f3677ff762e888008ffdebc1489` now reach the complete `[proof]` line, including story progress 5→10→15 and the exact spin handoff. The `__cxa_throw` breakpoint never fires: all fixture assertions finish, followed by a cleanup SIGABRT. This is not yet a successful process exit.

Both `second-first-throw.log` and `second-provenance.log` identify the next sink: `RuntimeContext::~RuntimeContext` → `WipeService::~WipeService` → the `std::vector<WipeEvent>` backing allocation → the original-allocation provenance panic. The remaining vector survives its scene heap. Exact debugger commands and binary hash are recorded in `second-debugger.json`.

`WipeService::push_event` in RuntimeServices.cpp appends the native vector and copies the event name without a host allocation scope; `force_open`, `force_close`, and `start_transition` also assign persistent `_current_name`. All are native bookkeeping, with no Game callback inside. Parent owns the general boundary fix. No production changes were made by this investigation, and both debugger processes have exited.
