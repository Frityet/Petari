# J3DModelX RMGK02 reconstruction evidence

Captured 2026-08-09 from the retail RMGK02 split object and the reconstructed
source object.

## Scope

- `src/Game/Player/J3DModelX.cpp`
- `include/Game/Player/J3DModelX.hpp`

No PC copy/activation, fallback implementation, protected-file edit, or
configuration-source edit is part of this reconstruction. `configure.py -v
RMGK02` only regenerated the existing ignored build graph. The unit remains a
`NonMatching` object and is not linked into the retail-verification DOL.

## Assembly-proven ABI corrections

The retail object proves that the extension after the `0xDC`-byte `J3DModel`
base has two byte fields at `0xDC/0xDD`, sixteen `Mtx*` slots at
`0xE0..0x11C`, a material callback at `0x120`, a shape callback at `0x124`,
callback/reference words at `0x128/0x12C`, sixteen display-list pointers at
`0x130..0x16C`, and sixteen display-list byte counts at `0x170..0x1AC`.
The active material index at `0x1C0` is a `u16` (retail uses `sth`/`lhz`),
followed by two bytes of alignment.

The sixteen matrix fields intentionally remain `Mtx*`, not `MtxPtr`: retail
callers allocate `new Mtx[count]`, while `setDrawView`/`viewCalc*` dereference
the selected pointer before passing the resulting `MtxPtr` to
`J3DMtxBuffer2::rotationMtx`. This preserves both the retail layout and the
dependent `MarioActorDraw.cpp` allocation syntax.

The retail signatures additionally prove `viewCalcRef`, `viewCalcRefPos`,
`drawIn`, and the `const` qualifier on `shapePacketDrawFast`.

## Recovered behavior

All 24 retail code symbols are present. The implementation reconstructs:

- multi-view draw/normal-matrix rotation, reference-model matrix generation,
  board/bump matrices, cache stores, and packet preparation;
- plane-projected reference positioning and the retail `0.1` axis scale;
- material/shape filtering, material and packet display lists, fog mixing,
  packet user data, global/dynamic/per-material display-list injection,
  callbacks, and replacement-shape drawing;
- fast shape drawing, matrix-group loads, differed texture matrices, and the
  no-matrix path;
- all sixteen constructor-recorded GD/GX display lists, including the captured
  screen texture recipe, aligned storage, initialization arrays, and lifecycle
  behavior;
- the translation-unit-emitted matrix helpers and both retail destructors.

## Focused object comparison

Generated with:

```sh
python3 configure.py -v RMGK02
ninja build/RMGK02/src/Game/Player/J3DModelX.o
build/tools/objdiff-cli diff \
  -1 build/RMGK02/obj/Game/Player/J3DModelX.o \
  -2 build/RMGK02/src/Game/Player/J3DModelX.o \
  -o pc-port/notes/j3d-model-x-rmgk02-20260809T035422Z/objdiff-final.json \
  --format json-pretty
```

Section results:

- `.text`: 95.75795% over 7,420 target bytes
- `.data`: 100%
- `.sdata2`: 71.42857%
- `.sdata`: 0% (the four-byte color constants were pooled differently; the
  constructor loads and uses the same values)

Function results:

- `J3DModelX::viewCalc2`: 83.36957%
- `J3DModelX::setDrawView`: 98.75%
- `J3DModelX::setDrawViewBuffer`: 100%
- `J3DModelX::copyAnmMtxBuffer`: 100%
- `J3DModelX::viewCalc3`: 99.82143%
- `J3DModelX::viewCalcRef`: 99.72222%
- `J3DModelX::viewCalcRefPos`: 99.75%
- `J3DMtxBuffer2::calcNrmMtx2`: 100%
- `J3DMtxBuffer2::calcDrawMtx2`: 95.81481%
- `J3DMtxBuffer2::calcDrawMtx3`: 86.11905%
- `J3DModelX::directDraw`: 100%
- `J3DModelX::drawIn`: 91.74%
- `J3DModelX::simpleDrawSetup`: 90.86957%
- `J3DModelX::simpleDrawShape`: 100%
- `J3DModelX::storeDisplayList`: 85.125%
- `J3DModelX::J3DModelX`: 97.21788%
- `J3DModelX::shapePacketDrawFast`: 100%
- `J3DModelX::shapeDrawFast`: 96.86364%
- `J3DModelX::~J3DModelX`: 100%
- `J3DMtxBuffer::swapNrmMtx`: 100%
- `J3DMtxBuffer::getDrawMtx`: 100%
- `J3DModel::getDrawMtxPtr`: 100%
- `J3DModel::~J3DModel`: 100%
- `J3DMtxBuffer2::rotationMtx`: 100%

The raw comparison and both object disassemblies were retained in the ignored
local evidence directory. They are reproducible with the commands above and
are intentionally omitted from the committed note to avoid adding several
megabytes of generated output.

## Full-build verification and decomp report

`ninja -j12` completed successfully, including the repository SHA check:

```text
build/RMGK02/main.dol: OK
SHA-1:   54b71431af0d509097bfdef4ec28617afc487e89
SHA-256: 8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf
```

The SHA-256 is identical to `orig/RMGK02/sys/main.dol`. The regenerated report
at this checkpoint is:

- overall: 65.40% matched, 14.25% linked (734 / 2,219 files)
- Game: 62.71% matched, 13.30% linked (532 / 1,605 files)
- JSystem: 64.01% matched, 7.11% linked (32 / 169 files)
- SDK: 76.12% matched, 18.05% linked (76 / 279 files)

`git diff --check` passed for both owned source paths.

## General PC/Aurora providers required for exact Mario integration

This source should only be copied into `pc-port/src/Game` after the following
general providers exist; none should be implemented as a Mario-only fallback:

1. A source-shaped J3D model provider that constructs authentic
   `J3DModelData`, joint trees, vertex buffers, material/shape relationships,
   packet arrays, texture tables, and animation matrices from disc resources.
   The current PC path parses models into a separate `J3dModelRenderer` graph,
   so it cannot directly satisfy the pointers and ownership used here.
2. A complete `J3DMtxBuffer` provider with dual draw/normal matrix arrays,
   per-view slots, weighted-envelope matrices, billboard/bump updates, and the
   exact pointer rotation semantics used by Mario's multiple draw views.
3. One authoritative rendering path. If the source-shaped path is selected,
   Aurora must execute the original GD/GX display-list stream, including J3D
   CP array-base state, partial XF matrix/register writes, vertex arrays,
   matrix loads, TEV, fog, alpha compare, destination alpha, blend, cull, Z,
   indirect/projection-map, and differed texture-matrix state.
4. Packet/runtime ownership for `J3DDisplayListObj`, `J3DMatPacket`,
   `J3DShapePacket`, `ShapePacketUserData`, shared/double-buffered material
   lists, aligned dynamic/per-material list injection, and the callback hooks
   used by Mario's special draw modes.
5. A screen-capture texture provider that exposes the active EFB through a
   stable `ResTIMG` image identity and a real `JUTVideo` render mode. Constructor
   display list 6 records that image pointer and EFB dimensions.
6. Wii-compatible heap/aligned allocation, cache maintenance, and
   scheduler/interrupt guard behavior. Cache operations may be coherent-host
   no-ops, but list lifetime and 32-byte alignment must remain valid.
7. Disc/archive lifetime and diagnostics sufficient to load Mario's model,
   texture, material, joint, and animation assets before model construction.

The evidence directory is intentionally ignored by `pc-port/.gitignore`
(`notes/`) and requires an explicit force-add if it is to be committed.
