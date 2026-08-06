# StageDataHolder declaration build repair

Timestamp: 2026-08-06T19:20:09Z

## Scope

The RMGK02 root build reached `SceneDataInitializer.cpp` and failed because `StageDataHolder.hpp` contained duplicate declarations for two existing methods. This repair removes only the repeated declarations and preserves one declaration for each target/source API.

Changed root file:

- `include/Game/Scene/StageDataHolder.hpp`

No implementation, overload, class field, PC source, or unrelated unit was changed.

## Proven duplicates

The header declared each of these identical signatures twice:

```cpp
JMapInfoIter makeMarioJMapInfoIter(const JMapIdInfo&) const;
JMapInfo* attachJmpInfoToArray(JMapInfo*, const char*);
```

Both are proven declaration-only merge duplicates:

- `src/Game/Scene/StageDataHolder.cpp` defines each signature once.
- RMGK02 exports one `makeMarioJMapInfoIter__15StageDataHolderCFRC10JMapIdInfo` symbol.
- RMGK02 exports one `attachJmpInfoToArray__15StageDataHolderFP8JMapInfoPCc` symbol.
- The duplicates came from combining two valid header orderings. The retained declarations are the newer upstream positions; signatures and accessibility are unchanged.

Neighboring `getStageDataHolderFromZoneId` declarations were not touched because they are distinct const/non-const member overloads.

## Focused verification

Both the implementation unit and the originally blocked include consumer compile:

```text
ninja build/RMGK02/src/Game/Scene/StageDataHolder.o \
      build/RMGK02/src/Game/Scene/SceneDataInitializer.o

[1/2] MWCC build/RMGK02/src/Game/Scene/SceneDataInitializer.o
[2/2] MWCC build/RMGK02/src/Game/Scene/StageDataHolder.o
```

Objdiff results relevant to the repair:

- `makeMarioJMapInfoIter`: **100%**.
- `attachJmpInfoToArray`: **100%**.
- `SceneDataInitializer`: **100%**, all **9/9** functions, **512/512** code bytes, and **96/96** data bytes.
- `StageDataHolder` remains an independently incomplete decompilation unit overall (64.99518% fuzzy); no unrelated function was changed in this build repair.

## Full-build result

The requested single root `ninja` pass completed all remaining work:

```text
[276/277] LINK build/RMGK02/main.elf
[277/277] DOL build/RMGK02/main.dol
```

Outputs were generated successfully:

```text
build/RMGK02/main.elf  10,408,776 bytes
build/RMGK02/main.dol   6,367,712 bytes
```

There is no next compiler or linker blocker after this repair.
