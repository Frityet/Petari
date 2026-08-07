# RMGK02 dependency rebuild repair

Date: 2026-08-07 UTC

## Scope

A fresh RMGK02 dependency rebuild exposed stale source/header mismatches that an
older incremental build had not recompiled. This repair updates the root
decompilation only. It does not add host behavior and does not edit any file in
`pc-port/src/Game`.

The repairs follow `AGENT_DECOMP_GUIDE.md`: use current typed engine members and
existing helpers, preserve original behavior, and avoid changes to the matrix
or vector implementations. RMGK02 assembly, current source definitions, object
layouts, and project history were used as the authority for every widened
declaration or renamed field.

## Repairs

### Demo executor and timekeeper declarations

- Restored the const `DemoSheetKeeperBase`, wipe, and sound virtual signatures
  encoded in the RMGK02 vtables.
- Moved the base keeper no-op bodies out of the header and used the real camera,
  wipe, and sound declarations in `DemoExecutor.cpp`. This preserves the retail
  `0x18` wipe/sound allocation instead of accidentally modeling a `0x1c` local
  class.
- Restored the executor's source-used fixed arrays and counters at `0x354`,
  `0x378`, and `0x3bc`, plus semantic stage-switch, starter, demo-name, and
  start-type fields.
- Corrected the time-part member names without changing the assembly-exact
  `isDemoEnd()` comparison.
- Completed the director/executor and keeper includes and corrected stale
  standard-library helper names.

### Typed layout and declaration repairs

- Restored the complete 85-method `MarioAccess` declaration surface from
  historical commit `0378b267f9cad4926efbc408fe67497f92558d80`, then reconciled
  it with all 85 current definitions and all 85 RMGK02 global symbols.
- Restored Mario return types, `AreaObj*`, the two-entry eye texture array,
  `MarioParts` effect-name pointer, `RushEndInfo` semantic fields, and required
  forward declarations/includes at their existing offsets.
- Corrected `MapObjActor::mRotator` to the concrete type used by the retail
  seesaw matrix accesses.
- Corrected `PlacementInfoOrdered` allocation clearing to `count * sizeof(pointer)`.
- Updated `ScenarioData` accessors to the already-declared `JMapInfoIter`
  representation and its selected row.

### Source/header drift at call sites

- Replaced removed `negateInline`/`setRotateInline` calls with the existing
  unary-negation and `setRotate` APIs.
- Used `HitSensor::mPosition` for the retail sensor-plus-`0x04` access.
- Used a layout-preserving raw-word view for `ActorLightInfo`'s two five-word
  `LightInfo` copies.
- Added the narrow headers required for nerve macros, demo utilities,
  `MarioConst`, sound utilities, and typed sensor access.
- Adapted the legacy mutable player-gravity return at its API boundary with a
  narrow const cast.

## Verification

The complete RMGK02 build finished successfully and the configured SHA-1 check
reported `build/RMGK02/main.dol: OK`.

Both built and extracted-oracle executables hash to:

```text
54b71431af0d509097bfdef4ec28617afc487e89
```

The regenerated report records:

```text
matched code: 64.51891%
fuzzy match: 77.49804%
complete code: 14.115765%
complete units: 727 / 2220
```

`git diff --check -- include/Game src/Game` passed. The only modified
`pc-port/src/Game` paths in the shared worktree remain the pre-existing
`SaveIcon` and `TriggerChecker` user edits; they were neither changed nor
staged for this repair.

See `verification.log` for the exact final commands and compact results.
