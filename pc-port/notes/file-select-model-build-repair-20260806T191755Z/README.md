# FileSelectModel declaration build repair

Timestamp: 2026-08-06T19:17:55Z

## Scope

The restored RMGK02 root build reached `FileSelectModel.cpp` and failed because `FileSelectModel.hpp` declared the same four nerve handlers twice. This repair removes only the second, byte-for-byte duplicate declaration block.

Changed root file:

- `include/Game/Map/FileSelectModel.hpp`

No implementation, class field, virtual declaration, PC source, or unrelated unit was changed.

## Evidence

The initial focused failure was:

```text
ninja build/RMGK02/src/Game/Map/FileSelectModel.o

include\Game\Map\FileSelectModel.hpp:27:
struct/union/class member 'FileSelectModel::exeOpen()' redefined
```

The header contained two consecutive copies of:

```cpp
void exeOpen();
void exeBlinkOnce();
void exeClose();
void exeBlink();
```

The retained block is the complete API required by the source nerves:

- `src/Game/Map/FileSelectModel.cpp` defines each of the four handlers exactly once.
- RMGK02 contains the standalone `exeBlinkOnce` and `exeBlink` functions.
- Metrowerks inlines `exeOpen` and `exeClose` into their respective nerve execute thunks, which are present in the target assembly.
- Removing a repeated declaration does not alter the class layout, mangled names, or generated behavior.

## Verification

The exact failed target now compiles:

```text
ninja build/RMGK02/src/Game/Map/FileSelectModel.o
[1/1] MWCC build/RMGK02/src/Game/Map/FileSelectModel.o
```

A one-unit objdiff report against `build/RMGK02/obj/Game/Map/FileSelectModel.o` is fully exact:

- Overall fuzzy match: **100%**.
- Exact functions: **25/25**.
- Exact code: **1004/1004 bytes**.
- Exact data: **268/268 bytes**.
- All code and data sections: **100%**.

## Next full-build boundary

One subsequent root `ninja` pass advanced through 96 additional camera, map, map-object, name-object, NPC, player, ride, scene, SDK, and runtime compilation steps before stopping at an unrelated header duplication:

```text
FAILED: build/RMGK02/src/Game/Scene/SceneDataInitializer.o

include\Game\Scene\StageDataHolder.hpp:43:
struct/union/class member
'StageDataHolder::makeMarioJMapInfoIter(const JMapIdInfo &) const'
redefined
```

`StageDataHolder.hpp` declares `makeMarioJMapInfoIter(const JMapIdInfo&) const` at both lines 32 and 43, while `StageDataHolder.cpp` and the RMGK02 assembly each expose one implementation. The same header also visibly repeats `attachJmpInfoToArray`, but no StageDataHolder edit was made in this bounded repair.
