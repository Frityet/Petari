# Current Mario start iterator recovery

Restored two missing original functions in the root decompilation:

| Function | RMGK01 address | Size | Source |
| --- | --- | ---: | --- |
| `StageDataHolder::makeCurrentMarioJMapInfoIter` | `0x80347d84` | `0xd4` | `src/Game/Scene/StageDataHolder.cpp` |
| `MR::getCurrentMarioStartIdInfo` | `0x803f756c` | `0x10` | `src/Game/Util/SceneUtil.cpp` |

Both declarations already existed and remain unchanged. No PC source mirrors,
runtime providers, build configuration, or native camera algorithms were changed.
This recovery prepares the original scene-camera startup dependency path.

## Recovered behavior

`getCurrentMarioStartIdInfo` returns a const reference to the current scene
controller's actual `JMapIdInfo`, using
`SingletonHolder<GameSystem>::get()->mSceneController->mCurrSceneControlInfo.mStartIdInfo`.
The retail getter is three pointer loads followed by return: the singleton at
`0x806b5be8`, `GameSystem` member `0x24`, and current `SceneControlInfo` member
`0x48`. It does not allocate, copy, or select an initial default ID. Its original
contract assumes these owners are initialized.

`makeCurrentMarioJMapInfoIter` copies that ID, calls the existing
`makeMarioJMapInfoIter`, validates the returned iterator with the existing
`MR::checkJMapDataEntries` inline helper, and returns it only when valid. Failed
validation returns the original default iterator `{nullptr, -1}`. Validation
checks the info pointer, nonnegative index, and index less than the table's row
count; a null table data pointer has zero entries.

There is no fallback to Mario number zero, the first start row, the root zone,
or another scenario. In particular, the validation after lookup does not add a
missing-zone recovery: the already-existing `makeMarioJMapInfoIter` assumes
the requested zone resolves to a placed holder.

## Traversal and identity

The retail `makeMarioJMapInfoIter` at `0x80347cac` (`0xd8` bytes) was inspected
without changing its existing source. It resolves `JMapIdInfo::mZoneID` through
`getStageDataHolderFromZoneId`, then scans **only that holder's** `mStartObjs`
tables in their stored order. Each table searches from row zero for the first
exact `MarioNo == JMapIdInfo::_0` match. The first matching table/row is returned;
exhausting those tables returns an invalid iterator.

The holder selector at `0x80347ac0` returns `this` for zone zero and otherwise
checks immediate child holders in stored order. This current-start lookup is
different from the recursive, index-based `getStartJMapInfoIterFromStartDataIndex`.
The returned iterator preserves the actual table/row identity, which the
existing `getCurrentStartZoneId` and `getCurrentStartCameraId` consumers use.

## Verification

The supplied RMGK01 rev0 DOL has SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`.
Both full root translation units compile without diagnostics using the
original GC/3.0a3 compiler, wibo, and the Game flags from `configure.py`.

The included verifier compares all emitted instructions after resolving two
branch relocations and one embedded SDA relocation. It checks the actual
`__init_registers` instructions at `0x8000422c`/`0x80004230`, which initialize
`r13` to `0x806b9620`; the singleton displacement is therefore `-0x3a38`.
Result: **57/57 instructions match exactly after 3 verified relocations**.
This claim covers these two recovered functions, not the rest of either
translation unit.

```sh
python3 pc-port/notes/scene-current-start-iter-20260903/verify-objects.py --compile
```

Compiler command records, objects, retail disassembly, and `verification.txt`
are under ignored `build/compat-scene-current-start-iter/`. Binary assets are
not part of the source changes. `git diff --check` passed for both root files.

## Remaining startup dependencies

The direct calls from the recovered iterator function now have root bodies.
This does not instantiate the original `GameSystem`, scene controller,
`StageDataHolder`, or their tables on PC. An eventual compatibility owner must
retain the real current scene start ID, selected holder occurrence, start-table
ordering, and row identity throughout this call chain.

`SceneControlInfo` already allocates its `mStartIdInfo` from
`MR::getInitializeStartIdInfo` and copies the requested ID via `setStartIdInfo`.
The initial-ID getter is still a placeholder in root `SceneUtil.cpp`; it was
not recovered in this bounded task. The wider `StageDataHolder` construction
and table-loading closure also remains incomplete. No fabricated defaults or
native scene-specific substitutes were added to bypass those dependencies.
