# Actual wrapper ownership with explicitly disabled audio

Implementation ready for shared validation. Root approved the explicit disabled-output subsystem boundary; original GameSystem initialization remains unchanged.

`GameSystemObjHolder::initAudio` constructs the actual `AudSystemWrapper` in its real process heap and invokes `requestResourceForInitialize`. `GameSystem::exeInitializeAudio` still launches the original async worker and requires both its completion and the wrapper's system-init load query before moving to LogoScene.

The reference wrapper constructor creates the speaker child heap. Initialization requests six real audio resources, receives those archives, constructs a real AudSystem, creates three converter singletons, and configures microphone output. Every original wave-load completion query returns false without AudSystem/AudSceneMgr. Returning a null AudSystem cannot therefore support original startup without explicitly adapting the audio subsystem boundary.

Proposed next checkpoint: an explicit DisabledAudioBackend owns initialization/request/reset state for the actual wrapper. Its native wrapper adapter reports completed disabled initialization only after the backend has initialized and the actual retail sound-name owner has been published. No AudSystem, rhythm owner or speaker owner is manufactured. Disabled static/stage/scenario requests require no voice/bank I/O but have explicit request/completion state, and pre-initialization queries remain false. The original GameSystem worker and nerve sequencing remain unchanged.

The native resource lifetime should be associated with the actual wrapper's original heap using the existing JKR heap finalizer. It must NOT strongly retain its own containing allocation domain through that finalizer: that would create a disposal cycle. Owned native byte buffers and ordinary original name arrays can borrow this original heap until its finalizer runs before storage reuse. Any separate retained child heap must have a clear external owner and must be retired before its parent heap's destruction.

The existing frozen original-name owner should be reused/extracted into a generic typed archive/table lifetime accepting bounded bytes and a real heap, instead of copying it into a second adapter. That refactor would change files in the 31-file frozen audio cohort, so implementation is held for root's publication/coordination decision.

Reference gaps to restore separately before native import: `createSoundNameConverter`, `movement`, `stopAllSound`. Retail asm confirms three actual AudSingletonHolder init calls, guarded virtual AudSystem frame-work call, and JAUSoundMgr::stop forwarding respectively. Reset `_29` and initialization `_2A` semantics must be retained; do not repurpose `_28` based only on its current apparent lack of consumers.

Suggested meaningful fixture: actual wrapper allocation inside a real JKRSolidHeap, ordinary request/create sequence through a real OS worker; false-before-init and completed-disabled-after-init; no output device opened and no voice graph constructed; authentic named SE IDs; all static/stage/scenario request queries; reset during initialization suspension/resume; repeated real-heap retirement clearing original table/singleton borrowers. Full GameSystem startup remains root's integration fixture and is not yet proved by this design.


## Implemented boundary

`DisabledAudioBackend` owns explicit Created/Requested/Received/Initialized phases, three bank-request completion bits, reset state, and the reused actual disabled object/name service. `DisabledAudioSystemWrapper.cpp` supplies the complete native wrapper boundary. The native wrapper owns one backend pointer with a real destructor, attached to its actual JKR heap finalizer. No object registry, root-heap lease cycle, partial AudSystem, or GameSystem global was added.

The wrapper queues the original `/AudioRes/SMR.szs` request through original FileLoader, receives it through the original completion wait, and bounds the already-decompressed BAA using its actual JKR block extent (never the compressed DVD size). Disabled bank playback needs no sequence/chord/ME/remix/speaker archive requests. Only after the real table and original name converter have been published does the disabled backend report initialization complete. Original `_29` pre-initialization suspend and `_2A` reset-permission state are retained. Explicit disabled bank requests complete with no voices or physical bank I/O; requests before initialization remain incomplete.

The name service now supports a borrowed actual process heap. Its name arrays remain in that same heap until the wrapper's finalizer; no child heap can be destroyed ahead of the finalizer, and no strong reference back to its own containing heap is retained. Native bytes remain separately owned. Retained RuntimeContext ownership still uses its existing private child domain. The original code's exceptional allocation failure inside a partially constructed converter is reclaimed at original heap retirement for the borrowed mode; no partial singleton is published.

Service publication is now process-scoped, matching the original global table/converter. A real audio-initialization OS worker can therefore publish its actual disabled SE object for subsequent main-thread use. The existing borrowed `disabled_system_sound_object()` query exposes that real owner across guest threads. The temporary HOME-specific policy query was removed after the user requested removal of that menu; no HOME audio integration remains.

The prior 31-file audio cohort was validated by root's shared `--names-only` run, then copied with zero hash drift under `notes/original-audio-name-owner-20260912/frozen-source/`. Its manifest and exact RuntimeContext/SingletonHolder patches remain intact for publication. This checkpoint's eight-file manifest lists only the subsequent wrapper/extraction changes.

## Validation and limits

- Four recovered/fixed original methods compile with the original compiler and each match retail 100%: createSoundNameConverter, movement, stopAllSound, requestReset. The latter restores the verified missing JAUSoundMgr::stop(10) after resetAudio. `reference-proof.json`.
- All three changed/new native production TUs compile in isolation. `native-syntax.json`.
- Existing JAI ownership target now has `--backend-only`. Test syntax passes (`backend-test-syntax.json`). It allocates the real wrapper in an actual heap, initializes the reused names on an actual OS worker, queries from main, tests pre-init and bank/reset transitions, and retires three actual heaps. It supplies the real already-received BAA directly to the lower backend, so it does **not** claim the original wrapper FileLoader request path or full GameSystem startup has executed.
- Root owns shared build/runtime validation; backend test success is not claimed before that receipt arrives. The ordinary retained-name `--names-only` test must also rerun after extraction.
- Audio playback/rhythm/speakers remain explicitly disabled. MainLoop/display and other GameSystem child frontiers are separate owners. No gameplay or whole-process startup success follows from this subsystem's readiness.

## Shared build result

Root shared build passed. Both `--names-only` and `--backend-only` runs passed, including worker publication and three heap retirements (`root-build.json`, `root-names-run.json`, `root-backend-run.json`). The original FileLoader request path remains for process startup validation.
