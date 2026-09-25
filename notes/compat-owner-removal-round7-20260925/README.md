# Original archive and resource ownership

Validated checkpoint: eight compat files removed (168 -> 160), plus the duplicate ArchiveMountService. App build and 20 focused targets pass. A fresh real-disc Metal run completed 600 opening frames and normal teardown, with an unchanged recorded binary hash. See `final-validation.json` and `focused-final.json`. Full compatibility removal and the Gateway gameplay goal remain incomplete.

This batch removes the duplicate archive/resource registry path. `FileLoader::mArchiveHolder` owns real mounts; the complete donor `ResourceHolderManager` owns its original 512-entry registry and original main-thread creation dispatch. `ResourceHolder` and `LayoutHolder` own the native decoding and cleanup required by little-endian, 64-bit execution. Generic resource bounds, borrowing and cache replacement belong to JKRArchive. The standalone RuntimeContext message publisher is removed; the actual GameSystem message owner remains authoritative.

The original manager's stationed/raw entry points are not deduplicated: their donor behavior can create distinct holders for one archive. Native loader records must therefore remain independent and archive cache replacements must support arbitrary retirement order. The normal basename lookup retains the original hash and `.arc` spelling.

Archive/file/holder retirement is checked as a whole before mutation, including independent raw archive/JMap borrowers. Source metadata ownership alone does not keep fixed heap-backed bytes alive. Typed JMap/JPC registration stays at the Game ArchiveHolder boundary, outside the generic JKR API.

Initial integration compiled but stalled at scene frame 26. LLDB identified a native `JkrAllocationScope` around `ModelManager::init` retaining the global current-heap mutex while waiting for resource construction on the main thread; that thread needed the same mutex. Native model/factory entry scopes now use guest allocation routing without extending mutex ownership across original asynchronous waits. The actual original heap selection and per-allocation serialization remain in effect. `gateway-initial-hang.txt` is the captured evidence.

The working tree contained unrelated source/test changes and staged route notes before this batch. Baselines and scoped staging records preserve them; validation uses this working tree and is not a clean-checkout claim. Validation/publication records identify exact outcomes and remaining limitations. Two pre-existing test changes are intentionally included as necessary dependency closure: the actual-process NameObj factory migration and retirement of the obsolete AuthoredPlacement simulator fixture. Other unrelated edits and the pre-existing staged route notes remain excluded.


The raw-reader preflight exposed owners which relied on bulk heap reclamation without releasing native C++ parser fields. Actor animation/camera helpers, lighting records and Demo sheet keepers now own their JMap parsers. Canonical ScenarioData/Parser, StageDataHolder and ParticleResourceHolder cleanup similarly releases their real child objects before archive retirement. These are Game lifetime adaptations; no archive-name or stage-specific exemption was added.

A subsequent real-process run completed 120 frames but stopped during retirement with 14 live DemoSheet leases. The old DemoDirectorOwnership adapter had no production callers for either executor or cast-group capture. Restoring proper ownership requires canonical Demo object destructors and removal of that dead snapshot/reclaim adapter. That run is failure evidence, not a successful shutdown.

Independent review also caught a typed-registry regression in the temporary texture-factory integration. The original manager intentionally does not deduplicate raw/stationed creation, so routing every texture through a new LayoutHolder both consumes entries and can collide with the original ResourceHolder basename lookup. The factory must follow its original loadTexFromArc/JUTTexture path; generic native texture backing and heap finalization belong at the SDK boundary.


The host placement inventory now calls the same original `MR::makeMtxTR` used by StageDataHolder, removing its separate sine/cosine formula. The unchanged placement-transform assertion exposed the table-lookup precision difference; no tolerance or authored transform was altered.

The actual-process fixtures preserve the original scheduler and heap constraints. Uncached resource creation runs on the original async worker while the main thread services its queue. Synchronous raw/stationed entry points remain on main with original client allocation enabled. Synthetic duplicate model data lives in bounded test arenas rather than consuming a retail archive's full solid heap. Repeated complete game boots use separate executable processes because OSInitAlloc reservations last for the OS process. Each process still checks actual teardown and weak-owner expiration.

The broad provider provenance gate remains failing. Its final artifact reports zero duplicate strong providers, zero stale source providers and no unavailable build/owner inputs, but existing unresolved and differing reviewed-source records remain. This batch does not claim a full provenance-gate pass.

Compatibility removal remains active. The next recommended closure removes the test-only StageSessionState, StageScenarioMetadataResolver and StageZoneMatrixRegistry services, after migrating their remaining fixtures. The original Gateway opening run is a bounded regression check; it does not establish progress through Rosalina or completion of Gateway.

The migrated wall provenance assertion now compares the retained authored archive path and exact RARC entry path. The old DVD cache normalized its diagnostic path to lowercase; the canonical FileLoader mount preserves original spelling. The test continues to require the complete exact source identity.
