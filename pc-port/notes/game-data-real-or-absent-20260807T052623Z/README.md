# GameData exact/real-or-absent boundary

## Outcome

- `src/Game/System/GameDataFunction.{cpp,hpp}` and `GameDataHolder.{cpp,hpp}` are byte-identical to the root decomp and their original translation units are excluded from the PC target.
- Host ownership, lookup, and absence handling now lives in `src/compat/`.
- A missing current `UserFile`, missing holder, or missing `SysConfigFile` throws. It no longer returns a blank name, slot 1, chapter 0, time 0, `false`, or silently drops a write.
- Game-event names must exist in the retail `GameEventFlagTable`. Stored state also rejects derived flags and invented names.
- Story events use the retail numeric `StoryEvent` progress rows. They no longer alias arbitrary story names into boolean game-event flags.
- Event values must exist in the retail `GameEventValueChecker` table. The picture-book value is the retail `絵本既読章`, not the previous invented `絵本話済` key.
- The fabricated host `GAM1`/proposed `GAM2` serialization is absent. `GameDataHolder::makeFileBinary` and `loadFromFileBinary` explicitly reject until the retail `BinaryDataChunkHolder` closure is implemented. Consequently `SaveDataHandler` is not implicitly constructed around a fake game-data binary.

## Retail-data provenance

The host registry is a UTF-8 representation of real RMGK02 data, not guessed progression:

- Root `src/Game/System/GameDataHolder.cpp` attaches `StoryEventBCSV` and compares `mStoryProgress` against its `progress` field.
- RMGK02 `StoryEventBCSV` is the 0x1e0-byte object at `0x8053dc20` in `build/RMGK02/asm/auto_06_8053DC20_rodata.s` (whole assembly-file SHA-256 `476ec2fbc3645072d2626fbc2614f1e1f073f903d2caf1c37e65af5c81c19bd7`). Its 14 CP932 name/progress rows decode to the entries recorded in `story-event-table.tsv`.
- Event-value names/defaults are copied from the decompiled retail constant table in root `src/Game/System/GameEventValueChecker.cpp`.
- The RMGK02 decomp build and `build/RMGK02/main.dol` checksum were green during this batch.

Exact source SHA-256 values:

| File | SHA-256 |
|---|---|
| `GameDataFunction.cpp` | `88dd99eb01b6fe5324b4a75fa85215f502f18796041f7dcf897f249f423de7af` |
| `GameDataFunction.hpp` | `454408e4404ec5231ea345a59b46d1bbaa5aa957d0fa74fbff63ff1d57954390` |
| `GameDataHolder.cpp` | `eb3e3f7fe89aaa9abd6d90690c2594b431b85837ebf63579836b373857efe1fc` |
| `GameDataHolder.hpp` | `6c72c30c04b85e2089534107419e1bf345e2ffaa5314b97c37361e54c99b778f` |

## Aurora boundary

The exact header exposed the SDK include `<revolution/os.h>`. Aurora now provides a generalized forwarding header to `<dolphin/os.h>`; commit `aa79222b88406b77c6c7b7ea637749586f233abc` is pushed to Aurora `main`.

## Verification

- `cmp` returned zero for all four exact GameData source/header pairs.
- `git diff --check`: pass.
- `smg-pc-game-data-real-or-absent-tests`: 20/20 pass.
- `smg-pc-save-data-core-real-or-absent-tests`: 3/3 pass.
- `smg-pc-story-sequence-real-or-absent-tests`: 4/4 pass.
- `smg-pc-aurora-native-tests`: 26/26 pass.
- `xmake build smg-pc`: aggregate link passed before a concurrent layout-header migration. The final retry was blocked outside this work by `Manual2P.cpp` using an incomplete `J3DFrameCtrl` from the concurrently exacted `LayoutUtil.hpp`.

## Deliberately absent

- Per-galaxy Power Star storage and GalaxyID-backed dependency predicates remain unavailable when aggregate data cannot prove the answer.
- Retail `BinaryDataChunkHolder` save serialization/deserialization remains unavailable.
- Save writes requiring that binary closure remain unavailable; no compatibility binary substitutes for it.
