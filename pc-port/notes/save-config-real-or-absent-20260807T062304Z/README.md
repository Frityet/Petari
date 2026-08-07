# Save/config real-or-absent boundary

Date: 2026-08-07 UTC

## Outcome

The PC port no longer fabricates save slots, partial `GameDataHolder` payloads,
event-name metadata, config defaults, or a usable `SaveDataHandleSequence`.
Save-dependent operations either consume an actual validated retail
`GameData.bin` or throw/report the capability as absent.

The following `pc-port/src/Game/System` sources and headers are byte-identical
to the decomp counterparts:

- `BinaryDataChunkHolder`
- `BinaryDataContentAccessor`
- `ConfigDataHolder`
- `ConfigDataMii`
- `ConfigDataMisc`
- `SaveDataHandleSequence`
- `SysConfigFile`
- `UserFile`

Host byte-order and unavailable-capability behavior is implemented under
`src/compat`. Exact translation units that require a host adapter are excluded
in `src/Game/xmake.lua`; the Game copies themselves are not edited.

## Removed fallback formats and state

- `SaveDataService::SlotState` and `slot_state_or_default`
- generated/default `GameData.bin`
- `SourceChunks` / `GameBinaries` partial payload encoding
- legacy Mii/source-chunk readers
- `CFG1`, `SYS1`, and `EVNM` host formats
- `SaveEventNameDictionary`
- invented IPL/BT `SysConfigService` defaults and caller-provided read fallbacks
- no-op/default-answer `SaveDataHandleSequence` methods and synthetic singleton
- unconditional RuntimeContext save-sequence construction/update

`try_initialize_save_data_ui` remains only as a non-mutating capability probe
and returns `false`. All sequence operations, queries, getters, and the global
sequence accessor throw while retail backing is absent.

## Format evidence

The config test compares the 60-byte default `config1` serialization against
the Dolphin oracle at:

`notes/dolphin-oracle-20260806T201750Z/seed-dolphin-user/Wii/title/00010000/524d474b/data/GameData.bin`

The exact bytes are also retained in `dolphin-config1.hex`. The test verifies
the retail `CONF`, `MII `, and `MISC` headers, hashes, sizes, payloads, and zero
padding.

`SysConfigFile` now serializes only the decompiled retail binary-chunk shape:
file version 1, one `SYSC` chunk, hash 1, and the three hashed attributes
`mTimeAnnounced`, `mTimeSent`, and `mSentBytes` in big-endian PPC byte order.
Malformed or truncated input is rejected explicitly.

## Aurora boundary

Generic NAND path normalization, storage, quota behavior, and tracing already
live in `aurora::NandFileSystem`; `NandFileSystemService` is an alias to that
implementation. The remaining byte-order bridge knows the SMG-specific v2,
19-member `GameData.bin` structure and therefore remains title compatibility
code. Moving it into Aurora would couple the generic platform NAND layer to one
game's save ABI.

## Verification

Commands run from `pc-port/`:

```text
xmake build smg-pc-game
  build ok

xmake run smg-pc-save-data-core-real-or-absent-tests
  Save-data core real-or-absent tests passed: 14/14

xmake run smg-pc-game-data-real-or-absent-tests
  Game-data real-or-absent tests passed: 17/17

xmake run smg-pc-story-sequence-real-or-absent-tests
  3 StorySequence real-or-absent test(s) passed

xmake run smg-pc-aurora-native-tests
  24 Aurora-native test(s) passed

xmake run smg-pc-save-config-real-or-absent-tests
  Save/config real-or-absent tests passed: 4/4

xmake build
  build ok
```

The focused save/config target also compares all 16 listed Game files against
the root decomp, loads the actual Dolphin `GameData.bin`, validates the retail
container, translates only its outer PPC structure for host access, and proves
that an unconfigured save service does not generate a file or accept a write.

## Honest current limitation

The real retail `GameDataHolder` binary closure and save state machine are not
yet linked. Initial title/FileSelect routing that does not require save state
remains available, but after-load routing and global picture-book/story state
stop explicitly at the missing save boundary. No route progress is preserved
through empty files, null callbacks, default answers, or invented persistence.
