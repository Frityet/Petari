# Standalone fixture ownership correction

Owned `tests/RuntimeContextConstructionTests.cpp` (now renamed to `tests/RuntimeContextFailureTests.cpp`) and `tests/RestartStageSessionTests.cpp`. Both were initially clean. Before snapshots, hashes/status and exact patch are adjacent. No production source, build wiring, index or commits changed. Root owns rebuilding and the test target rename to `smg-pc-runtime-context-failure-tests`.

## Runtime constructor failure fixture

The final fixture intentionally tests only two real failure/unwind paths: exhaustion of actual mapped capture allocation, and an injected logger failure after real runtime/video/scheduler/capture-director publication. It verifies the actual capture director NameObj registration, then complete binding/video/archive/config/catalog/particle/NameObj retirement and exact mapped heap capacity restoration. There is no exact callback-count assumption, synthetic GameSystem/FileLoader or successful standalone initialization attempt.

The old full-construction test was obsolete: successful construction attempts original message initialization through a missing FileLoader, before later standalone catalog/particle initialization even starts. LLDB evidence is preserved under `crash-diagnosis/`. Existing actual-process fixtures cover original startup/resources. **Successful full RuntimeContext initialization is not tested.** Imported aspect/camera assertions from the old successful-construction section were retired with that unsupported section, not represented as passed coverage.

HEAD/current compatibility: `RuntimeContext` HEAD and dirty constructors both accept the fixture's three arguments, publish the same runtime/scheduler/capture director before the logger injection, and enter the same original MessageHolder bootstrap after that injection. HEAD additionally constructs native scene service wrappers, whose constructors only retain a RuntimeContext reference. The failure fixture does not require the preexisting dirty removal of those services, their methods, or any particular scheduler entry count. No preexisting RuntimeContext changes are included here.

## Native stage-session fixture

`RestartStageSessionTests` keeps eight standalone checks: sound-ID packing; actual-disc HeavensDoor metadata; actual-disc FileSelect absent-Comet semantics and native binding; native session identity/restart versus immutable start; unresolved/None/Purple distinctions; unresolved versus known-absent native audio; explicit player bridge nerve capability and native audio invalidation; native audio reset/reconstruction. Removed obsolete expectations that original `MR` getters read standalone session/player/audio facades, and calls to previously deleted `begin_stage_audio`/`end_stage_audio` helpers. It does not claim actual Mario death, RestartCube BGM routing, original comet predicates, or GameSystem restart coverage.

`git diff --check` passed before root rebuild. No build/run result is claimed for the final renamed fixture by this lane. Earlier LLDB runs prove the two retained failure sections completed, then the retired successful-construction section hit the null FileLoader.
