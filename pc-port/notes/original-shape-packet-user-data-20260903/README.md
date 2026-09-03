# Original shape packet texture-matrix data

This tranche recovers the original `ShapePacketUserData` class and its two MR
entry points in root source, then imports them unchanged into PC Game. It also
recovers the original texture-matrix info assignment/effect-matrix helper. No
Game model or animator owner is activated by this work.

The verified RMGK01 DOL is the locally extracted
`build/compat-math-oracle/main.dol`, SHA1
`25c5959534b3c21246c6c7e42021b916b41fb578`. Binary slices and objects stay in
ignored build storage; the note contains reproducible scripts, source hashes,
commands, percentages, and a bounded byte-transfer check.

## Recovered Game methods

| Method | Retail address | Size | Current original-compiler match |
| --- | --- | --- | --- |
| Constructor | `0x803A9DD0` | `0x38` | 100% |
| `init` | `0x803A9E08` | `0x234` | 100% |
| `callDL` | `0x803AA03C` | `0x10` | 100% |
| `loadTexMtx` | `0x803AA04C` | `0x144` | 100% |
| `MR::getJ3DShapePacketUserData` | `0x803AA190` | `0x14` | 100% |
| `MR::initJ3DShapePacketUserData` | `0x803AA1A4` | `0xD4` | 100% |

The compiler is the configured GC3.0a3 compiler with actual `cflags_game`, real
root includes, and no generated header overlay. All six match their full retail
sizes. The constructor reveals eight entries of signed mode and post-matrix
index, followed by texgen count, display-list size, and the retained pointer.
The class occupies `0x4C` on Wii and `0x50` on the native 64-bit host. The pointer
starts at `0x48` in both layouts. Signed mode is required by retail `cmpwi`.

`init` runs under the original `MR::ProhibitSchedulerAndInterrupts`, allocates a
32-byte-aligned display list, invalidates/stores the actual range, and emits GD
state for each supported texgen entry. Position source maps to mode1, normal
source to mode2, and an environment map with nonidentity matrix maps to mode3.
Active entries use `GX_PTTEXMTX0 + 3*index`; inactive entries use `GX_PTIDENTITY`.
The real active texgen count and material blocks determine the emitted commands.

`loadTexMtx` reads the model's actual draw matrix for mode2, clears translation,
loads it at `GX_TEXMTX0 + 3*slot`, and calculates/loads each active original post
texture matrix using the actual inverse camera view. The packet getter tests
the real user pointer for null and returns it directly. There is no tag bit,
mask, or synthetic user-data table; native user storage retains the full pointer.

The original initializer deliberately leaves `__GDCurrentDL` referring to its
stack descriptor when it returns. That existing retail contract is preserved;
subsequent builders must establish their own current descriptor. This recovery
does not add cleanup behavior absent from the original instructions.

## SDK value objects and compiler limits

Root `J3DStruct.cpp` now defines `J3DTexMtxInfo::operator=` at `0x804313A8`
(`0x7C`) and `setEffectMtx` at `0x80431424` (`0x48`). The assignment copies bytes0/1,
center, SRT including the complete rotation/padding word, and all16 effect
matrix elements. It intentionally leaves bytes2/3 unchanged. `setEffectMtx`
copies the12 supplied affine elements and appends the row `(0,0,0,1)`.

The canonical configured SDK compile outlines the existing vector/matrix copy
helpers, producing measured matches of **24.129032%** (108/124 bytes) and
**0.16666667%** (72/72 bytes). Those low values are recorded, not described as a
high-fuzzy recovery. A separately labelled compile with existing Game IPA flags
exposes the straight-line transfer instructions: **87.09677%** (124/124 bytes)
and **78.5%** (76/72 bytes). Assignment differs only in the independent order of
center XY/Z loads and stores. The effect helper uses the same six paired loads
and stores, then four scalar stores instead of retail's paired zero store plus
two scalar stores, with different register and constant-symbol allocation.

`verify-original.py` checks the actual retail split instructions against the
verified DOL, masking only SDA constant relocations. Its bounded symbolic
load/store checker then compares every destination byte for both functions,
including preserved bytes, constants, separate objects, and identical input/
output pointers. It rejects any unrecognized opcode. This proves their field
transfer correspondence; it is not a general PPC/floating-point emulator or a
claim of full binary equality under canonical SDK flags.

The existing `J3DTextureSRTInfo::operator=` was entirely inside `__MWERKS__` and
did nothing on Clang. The native branch now copies the actual scalar members
and uses two `__memcpy` operations through a word temporary to preserve both
rotation and its two padding bytes, including self-assignment. The original
compiler branch and original field layout remain unchanged.

## Source and platform boundaries

- Root and PC `Game/System/ShapePacketUserData.{cpp,hpp}` are byte-identical.
- Original `Game/Util/SchedulerUtil.{cpp,hpp}` is imported byte-identically.
  The separate OS compatibility owner supplies the actual scheduler/interrupt
  contract. There are no replacement no-op lock methods in this tranche.
- `compat/MaterialTextureModeCompat.cpp` contains the four literal existing root
  `ModelUtil.cpp` predicates: `isEnvelope`, `isUseTexMtx`, `isUseTexMtxEnvMap`,
  and `isUseTexMtxProjMap`. The original early false return is retained.
- `compat/J3DStructCompat.cpp` is byte-identical to new root `J3DStruct.cpp`;
  the root/native Struct headers also agree.
- Root GD headers gain the missing actual `GDSetTexCoordGen` prototype and C
  linkage around `GDSetCurrentMtx`. The latter changes an incorrect C++-mangled
  relocation into the exact retail C call. The native `revolution/gd/GDTransform.h`
  forwards to the existing Dolphin SDK compatibility header.
- Parent owns the original material, texture-matrix calculation/load, GD/FIFO,
  shape hook, and OS dependencies. Their separate notes cover those providers.

## Validation and reproducibility

Run `python3 pc-port/notes/original-shape-packet-user-data-20260903/verify-original.py`
to recompile the real root TUs and recreate the comparison under the ignored
build directory. It reuses the independently extracted, verified retail objects
from `build/j3d-vertex-buffer-lifecycle-20260903/retail/obj`. The checked-in
`compiler-evidence.json` and `original-commands.json` are the current result.
Run `verify-source.py` to check the literal mirrors and those source hashes.

All newly imported source providers and
`tests/OriginalJ3DTextureMtxTests.cpp` passed isolated native compiler probes using
the actual compile database. No shared native build was run by this worker.
Parent owns target wiring and integration/runtime results.

The new test uses real `J3DTextureSRTInfo`, `J3DTexMtxInfo`, and `J3DTexMtx`
objects. It checks rotation/padding and self assignment, bytes2/3 preservation,
effect homogeneous row, real constructor/forwarding behavior, and an independent
translated-coordinate example for original ordinary/post mode1 calculations.
It has no fabricated virtual table or material/renderer stand-in.

The source models original allocation lifetime: `ShapePacketUserData` has no
new destructor and its aligned display-list storage remains with the original
arena/owner lifetime. A future resource owner must retain it for every packet
that references it and release native allocations through that owner.
