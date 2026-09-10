# Original process sources and ownership frontier

Imported complete GameSystem, GameSystemObjHolder, GameSystemSceneController, GameSystemFrameControl, GameSystemDimmingWatcher, GameSystemFontHolder and HomeButtonStateNotifier sources plus missing declaration dependencies. The original Wii `main` entry point is guarded by `__MWERKS__`; the native application's entry point continues to own host startup. No original GameSystem has been allocated with a partial object graph or installed merely to answer queries. These imports are preparation for activating the complete original process, not a claim that the bounded Gateway caller now uses it.

Added the exact original MR::getGameSystemObjHolder method outside Game, used by the original async facade. Restored the full StageResultSequenceChecker source and removed its three throwing host definitions. Replaced the StorySequenceExecutor source-inclusion macro wrapper with direct Game compilation. Its two invalid bool-versus-nullptr comparisons were corrected to boolean conditions in decomp first, then mirrored. The native GameDataConst header now directly forward-declares its opaque JMapData type instead of depending on a forced source wrapper.

The actual process initializer still needs implementations for its full original child graph: rendering/frame/XFB management, system error/reset/Home-button owners, audio wrapper construction, complete stationed layout resources, and NWC24 SDK storage. Actual process initialization must happen before original process queries execute. SceneTransitionRequestService and StageSessionState still own the earlier bounded demo's state; they have not been silently installed into an incomplete GameSystem. Their consolidation belongs with actual process activation.

## Thread and heap integration constraint

The new Aurora GuestThreadExecutionScope serializes native Game calls with OS-created workers. The actual process owner must enter it around construction/update/retirement, then release it between frames. A process init scope must not hold the Game current-heap mutex across a wait for an async worker that needs the same mutex. The current scene heap-restoration scope is appropriate for synchronous actor calls, not for wrapping the entire original process boot. Original scene construction will run through the original scene-controller async task.

No additional standalone native compilation or repeated gameplay replay was run for this root cohort. Combined build evidence belongs to the parent checkpoint note.

Imported the original NameObjRegister methods required by GameSystemObjHolder. Actual process activation must also consolidate registration: the current native NameObj constructor registers with the active SceneNameObjRegistry, whereas original process/scene registration uses the current holder in NameObjRegister. This checkpoint does not install an unowned global registry or double-register actors.

A further concrete async ownership gap remains: GameSceneBinding, SceneLifetimeBinding and SceneNameObjRegistry publish their active bindings in native thread_local storage. Original GameSystemSceneController creates and destroys a scene on its worker while the main thread updates/draws it. Those existing native bindings must be consolidated with the real process/scene owner (under the shared guest CPU gate) before enabling that controller. Merely creating the new OS worker does not transfer these bindings. ResourceHolderService is already process-wide. No blanket TLS-to-global replacement or unvalidated scene transfer is included in this checkpoint.

Added GuestThreadExecutionScope at the current RuntimeContext frame/draw/retirement and GameSystemService frame/update/draw entry points. These synchronize actual Game calls with the new SDK workers, without holding guest CPU ownership while the application waits between frames. Whole-process construction still needs its own explicit guarded owner once the child graph is complete.

## Compile closure

The combined build exposed missing native SDK declarations used by these originals: Home Button `HBMDataInfo`, Wii VI boolean/trap-filter declarations, and JMath's original random type/constructor. Canonical headers/source were imported; the HBM vectors use Aurora's existing equivalent KPADVec2. Native SDK declaration completeness is not implementation of Home Menu or VI analog trap-filter hardware. The existing JKRDvdRipper enum is corrected to original 0/1/2 values, with fixed-width native signatures for Wii longs. Native `size_t.h` forwards to stddef outside Game.

Original polling loops also assume Wii single-CPU preemption. The cooperative guest execution gate only releases at SDK waits/yields and between host frames, so activation must resolve polling loops that make no scheduling call; it must not hold the gate forever while a worker needs it. This checkpoint does not claim full process scheduling or scene activation.
