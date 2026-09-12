# Original process entry point

`smg-pc --original` now constructs the actual original `GameSystem` and its startup graph, then runs `GameSystem::frameLoop()` within the Aurora host frame. The existing development application route remains available. The bootstrap uses the original main initialization ordering plus native DVD/archive, persistent NAND/SC, heap lifetime, and KB+M input services. It does not construct `RuntimeContext`, a synthetic process singleton, or stage markers.

NAND defaults to SDL's user preferences directory under `NAND`; `SMGPC_SAVE_DIR` overrides it and `SMGPC_NAND_DIR` optionally imports existing console data without replacing existing files. The caller's disc guard outlives every original process owner. Native exception reporting does not instantiate the Wii exception viewer, so the process directly starts the real JUTDirectPrint manager required by the original display.

Retirement stops actual original worker/callback producers before their borrowed resources and heaps: async executor and NAND worker; pointer/draw callbacks; actual scenes and native object sidecars; WPad children; resource/file owners; display; remaining original SDK workers; Game heap children. This is a production lifetime implementation awaiting the first linked runtime run, not a claim of successful original startup or full scene teardown. Native phase messages expose the next runtime frontier.

Owned production files: `src/app/OriginalGameApplication.cpp`, `.hpp`, and the dispatch/include hunk in `src/app/main.cpp`. Main's existing process request catch and DVD guard are preserved. Build integration belongs to root: compile the new TU in the `smg-pc` application target. It requires the process agent's actual `destroy_star_pointer_director` helper already present on disk.

The application TU and main compile with the existing native flags (logs alongside this note); no new fixture or shared build was run by this lane.

The route accepts `--max-frames N` and `--max-frames=N`, following the showcase CLI convention; zero or omission is unlimited. The count increments only after an actual GameSystem frame and Aurora end-frame both finish. Quit and the frame limit use the same ordinary lifetime teardown. Invalid numbers fail before creating the window or process.

## Original scene support

`scene/OriginalSceneSupport` attaches only when the original controller's `mScene` is the actual `Scene` calling `initSceneObjHolder`. It borrows that Scene's original holder/executor and the controller's original NameObjHolder, retaining the real JKR heap rather than constructing a second scene or stage/session record. Its scheduler and native lifetime bindings survive loading on the original worker and execution on the main thread; the existing single-scene routing is serialized by the guest CPU gate. Original `NameObjRegister` now receives ordinary NameObj construction whenever it has an actual holder, and native individual retirement removes the original identity from the real process/scene lists. The registry's old development constructor remains available; actual-process construction uses a borrowed holder.

The original GameScene destructor contains a PC-only call to snapshot child retirement before the derived lifetime ends. The established GameScene child binding then runs at the Scene base boundary, before support and original holders disappear. Other scene retirement releases native sidecars while original raw arrays remain valid through their owning heap teardown. Process pointer/display services remain alive until scene destructors finish.

Scene effect ownership now borrows the original GameSystem particle catalog and actual scene heap. The existing development effect lifetime remains independent. There is no replacement particle catalog or fabricated process owner.

Bridge production files: new `src/scene/OriginalSceneSupport.cpp/.hpp`; existing `src/compat/SceneLifetimeCompat.cpp`, `src/scene/SceneNameObjRegistry.cpp/.hpp`, `src/scene/SceneExecutionBinding.cpp/.hpp`, `src/scene/GameSceneBinding.cpp/.hpp`, `src/compat/SceneNameObjUtilCompat.cpp`, `src/compat/SceneInitializationCompat.cpp`, `src/compat/EffectSystemOwnership.cpp`, `src/compat/OriginalParticleResourceLookup.cpp`, `src/Game/Scene/GameScene.cpp` (native destructor hook only), and the new application TU. All changed translation units compiled using existing native flags. Shared production link/runtime remains root's lane; this is not yet evidence that Logo or GameScene initialization completes.

## Original MessageHolder publication

Message lookups and pointer-to-message-ID lookup now borrow `GameSystem::mObjHolder->mMessageHolder` whenever the real process exists. A not-yet-constructed process holder remains unavailable instead of falling through to a standalone holder. The standalone owner still works without GameSystem and rejects coexistence with a process owner. `SceneObjHolderBinding` creates its message alias only for standalone hosts; actual GameScene retains original `initSceneMessage` and `destroySceneMessage` timing, so Logo initialization does not prematurely require game dialogue data.

A shared destruction helper retires scene aliases, both MessageData records and their native BMG backing before the archive/heap owners. It serves both standalone storage and actual process shutdown. Exact paths: `src/runtime/MessageHolderOwnership.cpp/.hpp`, the message-alias hunk of `src/compat/SceneObjHolderCompat.cpp`, and include/retirement hunks in `src/app/OriginalGameApplication.cpp`. All three changed TUs compile. No new fixture or shared build was run.

## Authored stage selection

The production route accepts `--stage NAME --scenario N` (also `--stage=NAME` / `--scenario=N`; scenario defaults to 1). Example: `smg-pc --original --stage HeavensDoorGalaxy --scenario 1 --max-frames 1800`. Omitting stage selection preserves normal original startup. The frontend validates its name capacity and scenario integer before creating the process, then verifies the chosen row against the actual original ScenarioDataParser after startup resources are ready.

The request waits for original `GameSystem::isDoneLoadSystemArchive()`, initialized original game-data/scenario owners, a normally executing current scene, and no pending transition, reset, warning, or save/load operation. It then calls `MR::requestChangeStageInGameMoving`, which routes a normal type-0 GalaxyMoveArgument through GameSequenceProgress and StorySequenceExecutor into the real controller. Original default entry `(0,0)`, story decisions, camera/wipe/loading/start behavior remain authoritative. The app does not change collected items, flags, nerves, timers, player state, or story progress. It uses the original current in-memory user data; selecting or creating a persistent save slot is not implied by these options.

This route does not pretend to have traversed any preceding stage: predecessor-dependent original story behavior stays dependent on the actual preceding scene. Stage selection is one frontend request, not a claim that the requested scene initialized or ran. The only production change for this addition is `src/app/OriginalGameApplication.cpp`; its narrow native compile passes without a forced compatibility include. Root owns the next production run.

## Original scene planet catalog ownership

`OriginalSceneSupport` now creates the existing `PlanetMapCatalog` for an actual
`GameScene`, using the original process's active archive service and DVD. The
catalog parses the real `ObjectData/PlanetMapDataTable.arc`; it exists before
`StageDataHolder` partitions placements and remains alive until the scene's
actors and native holders retire. Destruction clears the shared lookup before
the process's archive service can retire.

The native `NameObjFactory` already shares this catalog for authored planet lookup,
archive collection, and supported original `PlanetMap` construction. No native
original `PlanetMapCreator` owner is currently active; activating its entire
reference constructor table would also require the unavailable unique planet
actors. This change closes the missing actual-process data lifetime without
inventing that owner or treating unique, force-low, or optional-submodel planets
as ordinary planets. Those existing creator boundaries remain explicit.

Production path: `src/scene/OriginalSceneSupport.cpp`. A narrow native compile
without the old forced compatibility include passed; output is in
`OriginalSceneSupport-planet-catalog-compile.log`. Actual stage loading is still
being exercised by the parent application's build/run lane.
