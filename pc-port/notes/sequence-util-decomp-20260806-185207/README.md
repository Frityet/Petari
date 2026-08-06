# SequenceUtil decompilation

## Scope

- Reconstructed `src/Game/Util/SequenceUtil.cpp` from `build/RMGK02/asm/Game/Util/SequenceUtil.s`.
- Replaced the known PC-port WIP behavior (invented stage requests and silent demo no-ops) with the only currently supported bridge already present in the port: `requestChangeStageInGameAfterLoadingGameData()`.
- Did not add port-specific substitutes for missing game systems.

## Reconstructed behavior

The original unit contains 23 exported functions. The move-type values observed in the assembly are preserved exactly:

| Move type | Sequence request |
| --- | --- |
| `0` | Move to a named stage/scenario/start ID |
| `2` | Enter scenario select (normal or comet-specific) |
| `3` | Return to AstroDome |
| `4` | Continue after a stage clear |
| `5` | Retry after a miss |
| `6` | Continue after loading game data |
| `7` | Transition after title/boot/game over |

The miss path copies the player restart ID, substitutes the initialization start ID when a comet is active, preserves the current stage/current scenario, and writes the selected scenario to `GalaxyMoveArgument::_C`. The two Astro helpers construct `JMapIdInfo(startId, 0)` and request `AstroGalaxy` or `AstroDome`. The remaining helpers are direct delegates to `GameSequenceFunction`, `GameSceneFunction`, comet-event, and HP-meter APIs.

## CodeWarrior/objdiff evidence

Compilation used the repository's `GC/3.0a3` CodeWarrior compiler and the RMGK02 flags from `build.ninja`. Comparison used `build/tools/objdiff-cli` against `build/RMGK02/obj/Game/Util/SequenceUtil.o`.

| Symbol | Fuzzy match |
| --- | ---: |
| `requestChangeScene__2MRFPCc` | 100.00% |
| `requestChangeSceneTitle__2MRFv` | 100.00% |
| `requestChangeStageInGameAfterLoadingGameData__2MRFv` | 100.00% |
| `requestChangeStageAfterStageClear__2MRFv` | 100.00% |
| `requestChangeStageAfterMiss__2MRFv` | 100.00% |
| `requestChangeStageInGameMoving__2MRFPCclRC10JMapIdInfo` | 100.00% |
| `requestChangeStageInGameMoving__2MRFPCcl` | 100.00% |
| `requestChangeSceneAfterGameOver__2MRFv` | 100.00% |
| `requestChangeSceneAfterBoot__2MRFv` | 100.00% |
| `requestChangeStageGoBackAstroDome__2MRFv` | 100.00% |
| `requestStartScenarioSelect__2MRFPCc` | 100.00% |
| `requestStartScenarioSelectForComet__2MRFPCcl` | 100.00% |
| `hasRetryGalaxySequence__2MRFv` | 100.00% |
| `isExecScenarioStarter__2MRFv` | 100.00% |
| `requestPowerStarGetDemo__2MRFv` | 100.00% |
| `requestGrandStarGetDemo__2MRFv` | 100.00% |
| `requestStartGameOverDemo__2MRFv` | 100.00% |
| `requestEndGameOverDemo__2MRFv` | 100.00% |
| `requestEndMissDemo__2MRFv` | 100.00% |
| `requestShowGalaxyMap__2MRFv` | 100.00% |
| `executeOnWelcomeAndRetry__2MRFv` | 100.00% |
| `requestGoToAstroGalaxy__2MRFl` | 99.47% |
| `requestGoToAstroDomeFromAstroGalaxy__2MRFll` | 99.52% |

The two non-100% reports have identical instructions. Their only differences are local compiler-generated string symbols (`@2220`/`@2222`) versus the split original data labels (`lbl_805E2048`/`lbl_805E2054`).

`llvm-size` reports a 976-byte (`0x3D0`) `.text` section for both objects. `llvm-nm` also reports all 23 exports at the same offsets as the reference assembly, from `requestChangeScene` at `0x0` through `requestGoToAstroDomeFromAstroGalaxy` at `0x37C`.

## PC-port dependency boundary

The full original source cannot yet be copied into `pc-port/src/Game/Util/SequenceUtil.cpp`. It fails at the source/type boundary, before linker section garbage collection could discard unused functions. The port is missing:

- `GalaxyMoveArgument` and `GameSequenceFunction::requestGalaxyMove`;
- `JMapIdInfo`, initialization/restart-ID APIs, and current stage/scenario controller state;
- `GameSceneFunction` and all six requested scene/demo operations;
- comet state/start APIs used by miss and welcome/retry paths;
- system reset and forced HP-meter APIs.

Adding only `GalaxyMoveArgument` and `JMapIdInfo` would not be sufficient: their construction and the callers still require the broader scene/game-sequence systems. Until those source-close dependencies are ported, leaving these functions undefined is more accurate than inventing stage routing or returning/no-oping silently.

The retained PC bridge continues to notify both `StorySequenceExecutor` and the runtime sequence request channel for the supported post-load transition.

## Verification

- Root source compiled successfully with the RMGK02 CodeWarrior command/flags.
- `clang-format --dry-run --Werror src/Game/Util/SequenceUtil.cpp` passed.
- `xmake build smg-pc-game` passed.
- Full `xmake build` passed and linked `smg-pc`.
