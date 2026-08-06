# Story-event and spin-entitlement persistence

Date: 2026-08-06

## Outcome

The PC runtime now preserves the original story boundary used by the Gateway
opening:

1. `MR::onGameEventFlagEndTicoGuideDemo()` records
   `チコガイドデモ終了`.
2. `MR::onGameEventFlagEnableToSpinAndStarPointer()` records `スピン権利`
   and enables swing input immediately.
3. A new stage player restores swing permission from the saved entitlement.
4. Both names, and arbitrary other event flags/values, survive a full
   `GameData.bin` encode/decode cycle.

The event functions expose the same original `GameDataFunction` and `MR`
surfaces used by decompiled actors. Stage/runtime adaptation remains in
`compat/`; no stage name, route index, frame number, or actor name is used.

## Generic host save metadata

The existing source-chunk container stores event state in retail-shaped
records:

- `FLG1`: 15-bit name hash plus the flag value;
- `VLE1`: 16-bit name hash plus the `u16` value.

Those records are not reversible after a process restart. The old decoder
could therefore recover only its hard-coded ending flags and `MissNum`, even
though `SaveDataService::SlotState` supports arbitrary named state.

`SaveEventNameDictionary` adds an optional `EVNM` chunk to the PC host's
source-chunk representation. It contains only:

- format version and flag/value counts;
- length-prefixed UTF-8 flag names;
- length-prefixed UTF-8 value names.

The values remain authoritative in `FLG1`/`VLE1`. On load, each dictionary
name is matched to the corresponding original hash. Thus this is a general
reversibility layer, not a parallel save format and not a spin-specific
special case.

The codec rejects empty/NUL-containing names, truncation, invalid versions,
count/length overflow, and dictionaries that exceed the caller's remaining
`0xF80` game-file budget. If optional metadata is absent or invalid, the
retail-shaped known fields still decode as before.

When saving to the Wii-facing NAND path, `SaveDataService` continues to emit
the original game-binary payload shape; `EVNM` is host source metadata only.

## Original behavior cross-check

The root RMGK02 decomp correction in commit `6e4776b99` establishes that
`GameDataHolder::isPassedStoryEvent` means
`current_story_progress >= required_progress`. The PC holder currently exposes
that boundary through named story events because its host save model is map
based; the compatibility functions preserve the original call sites and can
move to a data-driven monotonic progress service without actor-source changes.

## Verification

- `xmake build smg-pc-aurora-native-tests`: pass
- `SMGPC_REAL_DISC=/workspaces/pcport/RMGK01.iso xmake run
  smg-pc-aurora-native-tests`: 27/27 pass
  - fresh save starts without spin;
  - the original acknowledgement setter persists and enables spin together;
  - Tico-guide completion remains an independent event;
  - UTF-8/arbitrary dictionary codec round trip;
  - malformed and over-budget metadata rejection;
  - complete `GameData.bin` writer-to-reader round trip for true and false
    flags plus event values;
  - existing HeavensDoor title/file-select/five-page-picturebook assertion.
- `SMGPC_REAL_DISC=/workspaces/pcport/RMGK01.iso xmake run
  smg-pc-stage-player-runtime-tests`: 8/8 pass after the serializer wiring.
- `git diff --check`: pass.
