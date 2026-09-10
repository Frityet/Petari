# Original save-sequence prerequisite audit — 2026-09-10

This is a read-only production audit and isolated native object probe. No source under `src/`, `decomp/`, or `aurora/` was changed; no Xmake, linking, runtime test, or Git mutation was performed.

## Concrete result

The whole original `SaveDataHandleSequence.cpp` is already byte-for-byte present in the port, including its original header. Its normal archive inclusion is disabled in `src/Game/xmake.lua:71`, and `src/compat/SaveDataHandleSequenceCompat.cpp` supplies a null-child constructor and unavailable operations instead.

The unchanged original translation unit compiles successfully to a native object with the exact missing `NANDErrorSequence.hpp` supplied in the scratch include directory. The unchanged original `NANDErrorSequence.cpp` also compiles successfully once four SDK constants are supplied from `decomp/libs/RVL_SDK/include/revolution/nand.h:33–36`:

| Constant | Original value |
| --- | --- |
| `NAND_CHECK_HOME_INSSPACE` | `0x1` |
| `NAND_CHECK_HOME_INSINODE` | `0x2` |
| `NAND_CHECK_SYS_INSSPACE` | `0x4` |
| `NAND_CHECK_SYS_INSINODE` | `0x8` |

The initial NAND compile reports only these four missing identifiers. The second compile uses `NANDCheckConstants.hpp`, containing their exact SDK definitions. This requires an SDK declaration expansion, not a Game algorithm change.

`compile.json` records both successful commands and the initial failed NAND probe. Objects were compiled using the existing native Game options, LLVM 23, ARM64 macOS, and optimized debug settings. The source and missing header are unchanged reference code. Warnings are the existing missing-override warnings in LiveActor.

## Actual symbol closure

Both object files were inspected using LLVM nm. `root-archive-providers.json` maps each undefined symbol to definitions in all four current debug `libsmg-pc*.a` archives; `cohort-closure.json` removes definitions supplied within the two candidate objects themselves. The **only game symbols absent after adding both original translation units** are:

- `GameSystemFunction::setResetOperationReturnToMenu()`
- `GameSystemFunction::requestGoWiiMenu(bool)`

The remaining unresolved names in that report are platform C++ exception/stack-check runtime symbols. They are excluded from the game-dependency claim, not supplied by invented providers. This is a symbol inventory, not a successful link.

The two missing original functions both delegate to the real `SingletonHolder<GameSystemResetAndPowerProcess>` in `decomp/src/Game/System/GameSystemFunction.cpp`. The reset owner is absent natively. Its `requestGoWiiMenu` performs permission checking and a nerve transition, so replacing it with an immediate quit or a successful no-op would not preserve behavior. Whole reset-owner closure includes GameSystem services, reset/audio/save permission, system wipe, NWC24 reset readiness and original frame-control dependencies. Original `GameSystemResetAndPowerProcess.cpp` also contains an explicitly undecompiled draw method; this audit does not claim that owner is ready wholesale.

Activation must remove the competing SaveDataHandleSequence class-method definitions. The remaining host global accessor/probes must use the actual original director's child when that process owner exists, rather than introducing another save-sequence singleton.

## Guaranteed resource-initialization frontier

Constructor/functor-registration availability is substantially closer than initialized save availability:

1. Original `SaveDataHandleSequence` constructor allocates an aligned temporary buffer and creates its NoOperation nerve. Functor registration clones the caller's actual callbacks. It does not allocate its save/UI children until `initAfterResourceLoaded()`.
2. `initAfterResourceLoaded()` constructs actual `SysConfigFile`, two `UserFile` objects, then `SaveDataHandler`, two child-executed `SysInfoWindow` objects, `NANDErrorSequence`, and `SaveIcon`.
3. Current native `SaveDataHandler` constructor calls `initializeAllFileInSaveData` (`src/Game/System/SaveDataHandler.cpp:170`), which serializes a UserFile. `UserFile::makeGameDataBinary` calls `GameDataHolder::makeFileBinary`.
4. The current `GameDataHolderCompat.cpp:383` implementation **always throws** for retail serialization; deserialization immediately below also always throws. Thus the current initialized save-core closure is unavailable even though its method symbols are linkable. This is a source-proven reachable blocker, not a runtime reproduction or claim that no earlier resource prerequisite could fail.

Restoring the actual whole `GameDataHolder` and its real data is a coherent prerequisite. Its original constructor owns six chunks: GameDataPlayerStatus, the GameEventFlagChecker chunk, StarPieceAlmsStorage, SpinDriverPathStorage, GameEventValueChecker, and GameDataAllGalaxyStorage; it additionally creates ScenarioProgressTestRun and its StoryEvent JMapInfo. Current compat keeps a separate map-backed HolderState and has many fabricated-null original child pointers. Creating the original save owner without consolidating the current GameDataSession with the director's actual current/backup UserFile holders would leave two conflicting gameplay databases.

This audit has not certified the six-chunk cohort as ready. The existing BinaryDataChunkHolder compat is **not** an absent serializer: it already serializes/deserializes its registered chunks using explicit big-endian metadata and bounds checks. The immediate missing behavior is GameDataHolder's real chunk ownership/registration and underlying data. Do not replace the endian-aware buffer implementation blindly with the raw original struct accesses on little-endian PCs.

Other original-source cleanup dependencies are also real:

- Current native `SaveDataHandler.cpp` is substantially rewritten, including eager NAND loading in its constructor, a new `mSaveDataDirty` policy, and native struct/checksum serialization. Original constructor only creates/initializes its objects and enters Wait; its original preload nerve issues the read. Original whole `SaveDataHandler.cpp` plus `SaveDataFileAccessor.cpp/.hpp` is the proper future replacement, with explicit serialized-byte compatibility handled at a general boundary.
- Current native `NANDManager.cpp/.hpp` replaces the original queued manager/thread and normalizes paths itself. Original `NANDManager.cpp` plus `NANDManagerThread.cpp` should eventually drive generalized NAND/OS services. Its original thread performs request completion and callbacks; avoid retaining synchronous host request policy in Game.
- Current native `SaveDataBannerCreator.cpp` creates a zero-filled banner object and writes a fixed host path. Original whole source loads the real SaveIconBanner textures/messages and uses NAND banner APIs. Its actual assets/SDK support belong in the prerequisite scope when save functionality is claimed.

## Proportionate validation for activation

The existing `tests/SaveDataCoreRealOrAbsentTests.cpp` deliberately checks the null constructor and unavailable operations. Those expectations become obsolete when the actual constructor is activated. Replace them with an actual process-allocation-domain owner fixture that verifies original NoOperation state, aligned temporary storage, real functor cloning, and exact constructor versus resource-loaded readiness. Exercise real initialized children only after actual chunk and layout resources are available. The full save claim additionally needs a real original preload/save/readback cycle and checked Wii-compatible bytes, not just constructor success.

`source-manifest.json` captures all probed source and archive hashes. The scratch include header is an exact copy; no recoveries or altered decomp statements are proposed by this probe.
