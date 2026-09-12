# Native Wii HOME menu omission — 2026-09-12

The user explicitly directed removal of the Wii HOME menu. This is an authorized native platform omission, with the original decompilation preserved. There is no substitute menu, success-returning HBM stub, Wii RSO execution, or fake console menu owner.

## Final source changes

- GameSystem no longer constructs, initializes, draws, moves, or checks a HomeButtonLayout. Its member/declaration and the now-unused native layout header were removed.
- Error watcher movement no longer waits for a HOME menu. System warnings, reset processing, save activity and their existing scene/dimming behavior remain.
- The actual HomeButtonStateNotifier is retained because its original role also includes resuming a movie after a system warning. It now receives only `isOccurredSystemWarning()`, and the real-startup fixture still requires this owner.
- The reset process no longer asks the removed menu to deactivate. The corresponding GameSystemFunction declaration, implementation and include were deleted. Original reset/power/error behavior otherwise stays outside this lane.
- Removed the dedicated 0x80000 HOME heap allocation/field and unused MemoryUtil allocator declaration.
- Removed six HOME-only stationed resources: the three HomeButton2 files, the HomeButtonMenuWrapperRSO module, its product.sel symbol table, and HomeButton.arc. The original null table terminator remains.
- Removed every new uncommitted HomeButtonLayout/Wrapper import and Aurora menu implementation from the abandoned replacement direction. Aurora's HBM header was restored exactly. Native controller HBM data types and existing warning/input facilities remain where used independently.

## Validation and scope

All six affected native translation units compiled independently: GameSystem, GameSystemFunction, GameSystemResetAndPowerProcess, HeapMemoryWatcher, StationedFileInfo and OriginalGameSystemStartupTests. The final header-dependent recompiles are recorded in `omission-final-native-compiles.json`; the reset TU's receipt is in `omission-native-compiles.json`. A repository source scan finds no remaining HomeButtonLayout, forceToDeactivateHomeButtonLayout or RSO/HBMCreate integration. Whitespace checks pass.

Root owns the next shared startup link/run and any subsequent platform reset implementation. Compilation does not establish successful GameSystem initialization or playable Gateway intro/chase. `omission-source-manifest.json` contains the exact final source hashes and deletion. The root-owned untracked startup test was edited only to remove the HOME-layout invariant; every other line was preserved.

The earlier HomeButtonLayout native/reference compiler receipts in this directory document the initial owner audit, before the user's removal instruction. They are historical evidence, not the final implementation. `GameSystemResetAndPowerProcess.before-home-removal.cpp` preserves the root's previously frozen native recovery for its independent reference/publication receipt.
