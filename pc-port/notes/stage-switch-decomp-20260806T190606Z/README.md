# StageSwitch decomp verification

Updated: 2026-08-06T19:06:06Z

## Scope

- Audited `src/Game/Map/StageSwitch.cpp` against `build/RMGK02/asm/Game/Map/StageSwitch.s` before using it in the PC port.
- Fixed the missing `ZoneSwitch` loop increment. Without it, construction never returned.
- Reconstructed `StageSwitchCtrl::isOnAnyOneSwitchAfterB(int)` from the target assembly.
- Preserved assembly-backed pointer snapshots in both consecutive-switch queries; these affect register allocation only, not behavior.
- No inline assembly or port-specific behavior was added to the root decomp.

## Behavior recovered

The unit provides the original two-level stage-switch model:

- IDs below 1000 are local to a placement zone.
- IDs from 1000 upward address the shared global 128-bit switch bank after subtracting 1000.
- `StageSwitchCtrl` binds `SW_A`, `SW_B`, `SW_APPEAR`, and `SW_DEAD` from a placement row.
- `isOnAllSwitchAfterB` and `isOnAnyOneSwitchAfterB` inspect consecutive bits starting at `SW_B` in the same local/global bank.

That zone distinction is required by HeavensDoor: the stage contains repeated low switch IDs in different zones as well as global IDs such as 1111–1118. A raw integer-keyed compatibility map cannot reproduce the original semantics.

## Objdiff evidence

Comparison used the repository's RMGK02 target object and `build/tools/objdiff-cli`.

- Overall fuzzy match: **95.32516%**.
- Exact functions: **27/31**.
- Exact code: **1396/1956 bytes**.
- Data and vtable match: **100%**.
- No semantic mismatch was found.

The four non-exact functions are behaviorally equivalent code-generation differences:

| Function | Fuzzy match | Difference |
| --- | ---: | --- |
| `ZoneSwitch::ZoneSwitch()` | 76.42857% | Equivalent base-array zeroing sequence |
| `isOnAllSwitchAfterB()` | 98.97059% | Register assignment only |
| `isOnAnyOneSwitchAfterB()` | 98.97059% | Register assignment only |
| `createSwitchIdInfo()` | 64.63636% | Current headers inline JMap lookup helpers while the target calls `getValue<s32>` |

All bit operations, switch wrappers, zone lookup/storage, on/off/read operations, constructors other than the noted codegen case, factory helper, and destructor are exact.

## Port boundary

The PC integration should copy this game implementation rather than maintain an alternate switch state machine. The only host-side requirement is general placement metadata: each `JMapInfo` row must retain its placed-zone ID so the original `JMapIdInfo` constructor can recover it through `MR::getPlacedZoneId`.

