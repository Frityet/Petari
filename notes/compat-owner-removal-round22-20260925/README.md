# Delete the final actor and heap compatibility providers

The src/compat and src/scene directories are deleted, and their source/header build globs are gone. No replacement compatibility directory, actor state map or heap-domain registry was introduced.

NameObj owns its intrusive live identity, registration generation, ownership claim and mutation-safe borrower notifications. Actual group/director/follower owners retire their own references. LiveActor owns its original resource pointers, shared model, sound heap and clipping references; actual keepers/holders manage sensor and clipping retirement. Ordinary init/movement methods perform these operations directly.

JKRHeap owns retained native lifetimes and current-heap restoration. Original manually owned child heaps remain manually owned; shared native root/display heaps adopt their actual storage and release it after destruction. GameResourceRuntime owns root/MEM2 backing, with MEM2 attached to the actual GDDR3 heap. Resource, model, scene, effect, executor and rendering owners retain actual heap handles. Async initialization and display callback waits do not gain persistent heap-selection locks.

Existing fixtures are migrated to actual owners without expanding the test campaign. Validation remains one integrated application build plus the bounded fresh-save 120-frame Gateway opening/shutdown check. The full wakeup-to-Rosalina demo is outside what that check can establish; audio remains disabled.

See the three owner notes and resource review for implementation details. The original unrelated staged route notes and editor/history changes are preserved.

The application builds successfully. Removing the registry header exposed one missing direct J3dMatrix include, which was fixed. A source review also added an explicit missing sound-heap guard. The first opening run exposed a normal actor-teardown crash: draw-list removal still needed the model after LiveActor had cleared it. LiveActor now disconnects the actual active scheduler first. The diagnostic LLDB run reached 120 frames and stopped on expected worker cancellation; it was then terminated deliberately.

The final fresh-save Metal run completed 120 frames and exited 0 in 3.033 seconds with no remaining process. Binary SHA-256: c08e1e4cd27cfa9d059ed3b627bcbc0be301d73f95ab3912ea823fab1488ad55. Final incremental build: 5.478 seconds. No fixture suites were built or run.
