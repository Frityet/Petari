# Original auto-effect records and groups — 2026-09-03

Frozen root-first checkpoint. No production native source or build selection changed.

## Root files

- `src/Game/Effect/AutoEffectGroup.cpp`: actual allocating constructor, add, three registration overloads, and case-insensitive group construction.
- `src/Game/Effect/AutoEffectInfo.cpp`: actual metadata constructor/parser/name selection, original flags, color parsing, and nine-entry draw-order table.
- `include/Game/Effect/AutoEffectInfo.hpp`: the retail field at 0x4C is an `s32` draw-order ID, not a string pointer.
- `libs/MSL_C/include/cstdlib`: declares the existing standard `strtoul` provider.

`root.patch` and `root/` contain the exact four-file checkpoint. `AutoEffectGroup.hpp` is already committed in the previous EffectSystem checkpoint.

Original compiler proof in `root-evidence.json`: group constructor/add/three registration overloads 100%; `createAutoEffectGroup` 99.805824%; metadata constructor/name/str2Color/Color8::set 100%; metadata init 99.17083%; string/flag helpers 99.47–99.69%. Same instruction sizes and arithmetic; string/table relocation identity and iterator inline scheduling account for differences. No algorithm was replaced to achieve a match. `dol-evidence.json` verifies all 16 corresponding retail text symbols against the SHA1-verified DOL after relocation normalization. The nine draw-order integer/string pairs are preserved in source; objdiff data scores include different compiler-generated string symbol identity.

## Native boundary and isolated proof

`stage.py` copies complete original Game bodies to `build/original-auto-effect-20260903/staged`. Two complete TUs compile with native/Aurora/root fallback include ordering. The generic Color8 integer constructor and integer conversion represent 0xRRGGBBAA, independent of host byte order. `color-native.patch` changes only these two existing inline operations in `pc-port/src/Game/Util/Color.hpp`. GXColor/channel APIs retain RGBA byte ordering. Direct union writes in original AutoEffectInfo initialize zero and are endian-invariant; the other observed original direct Color8 union writes initialize 0xFFFFFFFF and are also invariant. This is a general color representation fix, not a parser-specific swap.

`verify-native.py` links the real allocating group/add/info graph against existing native resource/JMap/string/allocator libraries and runs the actual extracted Effect.arc AutoEffectList.bcsv. ASan/UBSan reported no diagnostics. 2,591 records in 614 groups pass comparisons for flags, nullable retained strings, six float fields, two frame fields, all 170 authored colors, draw IDs, and getName selection. 1,024 independent Color8 integer/channel/GXColor round trips pass. Draw ID distribution: 2167/1/38/161/4/185/34/1/0.

The fixture manually selects actual table records and invokes actual group constructors/add methods. It does not invoke scene-global createAutoEffectGroup or any registration overload, and does not claim complete EffectSystem runtime ownership or drawing. Those functions remain literal original providers with their actual external dependencies. Newly staged TUs are sanitizer-instrumented; previously built libraries are not. The fixture explicitly deletes owned AutoEffectInfo records before the group's original array destructor for bounded test cleanup; original Game scene-heap lifetime remains unchanged.

Reproduce from repository root:

```
python3 pc-port/notes/original-auto-effect-20260903/verify-root.py
python3 pc-port/notes/original-auto-effect-20260903/verify-dol.py
python3 pc-port/notes/original-auto-effect-20260903/stage.py
python3 pc-port/notes/original-auto-effect-20260903/verify-native.py
```

Native application, when the parent activates the complete owner graph: copy staged `Game/Effect/AutoEffectGroup.cpp`, `AutoEffectInfo.cpp`, `AutoEffectInfo.hpp`, and the already committed root `AutoEffectGroup.hpp` into corresponding `pc-port/src` paths; apply `color-native.patch`. No new MSL host shim is needed: host libc supplies strtoul. Parent owns xmake and atomic activation.
