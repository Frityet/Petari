# RMGK02 AreaForm / AreaObj / SwitchArea audit

Updated: 2026-08-06T19:48:16Z

## Outcome

The root Area system audit found one genuine source defect: merge `a56d6f448` introduced an explicit `AreaObjMgr` destructor declaration even though no out-of-line definition exists. Both merge parents lacked the declaration. Removing it restores the compiler-generated virtual destructor, resolves the manager vtable, and makes `AreaObj` fully match RMGK02.

No behavioral source changes were needed in `AreaForm` or `SwitchArea`.

## Match evidence

| Unit | Exact functions | Code bytes | Data bytes | Fuzzy |
| --- | ---: | ---: | ---: | ---: |
| `AreaForm` | 27/33 | 2996/3764 | 128/128 | 99.51966% |
| `AreaObj` | 24/24 | 1984/1984 | 136/136 | 100% |
| `SwitchArea` | 5/5 | 456/456 | 56/56 | 100% |

The six non-exact `AreaForm` functions differ only in compiler code generation: constructor store order, register allocation, and one extra move. Their control flow and behavior agree with the target.

Focused objects and the complete RMGK02 `ninja` build pass. `git diff --check` is clean.

## SwitchArea semantics and disc data

RMGK02 placement data confirms all four relevant `SwitchCube`s are one-shot latch-on volumes with `Obj_arg0..2 = -1`:

| Zone | Rows / local IDs | SW_A | SW_B | Scale |
| --- | --- | --- | --- | --- |
| `HeavensDoorMysteriousZone` | 1/8, 2/11 | 1113, 1114 | 1111 | 0.3, 0.9 |
| `HeavensBlackHoleZone` | 0/0, 4/4 | 21, 1016 | -1 | 3.84, 5.75 |

The mysterious-zone pair is B-gated; the black-hole pair is not. The original factory mapping is `SwitchCube -> Type_Cube2`, `SwitchSphere -> Type_Sphere`, and `SwitchCylinder -> Type_Cylinder`.

## Source-only compatibility boundary

`libs/JSystem/include/JSystem/JGeometry/TMatrix.hpp` currently declares `multTranspose` with a const output reference, but the RMGK02 symbol requires a writable `TVec3f&`. The source tree also lacks host definitions for `mult`, `multTranspose`, and both `concat` overloads; the target obtains them from the incomplete `AnmPlayer` object.

This did not justify changing root Game source during the audit. A source-only PC import needs a narrow host JGeometry compatibility layer with the corrected writable output signature and host-safe definitions. That work belongs outside `pc-port/src/Game` so the eventual Area sources can remain source-close.
