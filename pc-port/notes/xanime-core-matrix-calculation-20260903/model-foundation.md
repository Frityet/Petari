# Real ModelData construction foundation

This tranche imports the actual `J3DModelData` object and its embedded table
types. It does not turn the existing renderer summaries into fully decoded
`J3DModelData` resources. No BMD/BDL is registered with missing material or shape
data, and no smaller structure is cast to an original J3D type.

## Source correspondence

- `pc-port/src/JSystem/J3DGraphAnimator/J3DModelData.hpp` and
  `J3DMaterialAttach.hpp` are byte-identical to their root JSystem headers.
- `J3DShapeTable.hpp` retains the original class, private fields, constructor,
  virtual destructor, and all declarations. Its unused `J3DShape.hpp` include
  becomes forward declarations of `J3DShape`, `J3DDrawMtxData`, and
  `J3DVertexData`. The header uses only pointers to those types.
- `pc-port/src/compat/J3DModelDataCompat.cpp` copies the original ModelData and
  MaterialTable constructor, `clear`, and destructor bodies, plus the recovered
  VertexData constructor. Existing actual JointTree and DrawMtxData providers
  construct and destroy the embedded joint table. ShapeTable construction and
  destruction remain the original inline bodies.
- ModelData display-list operations and MaterialTable animation operations
  retain their declarations but are not supplied by this constructor tranche.
  There are no replacement bodies for missing operations.

The original destructors do not free the referenced joint/material/shape/vertex
arrays. A later native resource owner must retain and retire those arrays
explicitly, without adding implicit ownership to the original classes. A test
may intentionally construct a model containing only joints using the actual
type and actual joints; that is distinct from dropping material/shape data from
an authored asset.

## VertexData recovery

`src/JSystem/J3DGraphBase/J3DVertex.cpp::J3DVertexData()` is restored from verified
RMGK01 retail `0x804237F4`, size `0x80`. The instructions establish:

1. Zero the five counts, attribute-format pointer, position pointer, normal
   pointer, and NBT pointer at original offsets `0x00..0x20`.
2. Zero both color-array pointers in a two-iteration loop.
3. Zero all eight texture-coordinate-array pointers in an eight-iteration loop.
4. Set position and normal fractional bits to zero, and both component types
   to `GX_F32` (the literal value 4), in that order.

The root source uses a constructor initializer list for the scalar prefix,
typed pointer loops, and the named GX enum. The native extraction is unchanged.
No padding is written or interpreted as a field.

## Verification

Run from the repository root:

```sh
python3 pc-port/notes/xanime-core-matrix-calculation-20260903/verify-model-foundation.py
```

The verifier checks all seven root/native bodies, both exact full headers, and
the sole ShapeTable include change. It compiles the three original source units
with the configured GC 3.0a3 JSystem compiler flags through `wibo`/`sjiswrap` and
compares them against the existing DTK split of the supplied retail DOL. A
different split can be selected with `--target-jsystem`.

The DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`.
`model-foundation-evidence.json` records the compact result. The recovered
VertexData constructor is exactly all 128 retail bytes, with 100% objdiff.
ModelData `clear`/destructor and all three MaterialTable methods also compare
100%. The pre-existing ModelData constructor compares 78.62%: the compiler emits
a call to the original inline ShapeTable constructor where retail inlines its
vtable/count/pointer initialization. Retail calls JointTree, MaterialTable,
VertexData, and ModelData `clear` in the same order; the rebuilt ShapeTable
callee performs the same four stores. The existing source is preserved.

Compiler commands, logs, objects, disassembly, and full objdiff output remain
under `build/xanime-core-model-data-20260903/`. This verifier performs no native
build or runtime test. Parent-owned native tests should exercise construction,
clear boundaries, actual embedded joint access, and destruction with separately
owned arrays before activating a production resource owner.
