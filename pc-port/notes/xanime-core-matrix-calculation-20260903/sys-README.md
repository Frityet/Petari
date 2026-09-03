# Genuine J3DSys initialization

`pc-port/src/compat/J3DSysCompat.cpp` provides the real `J3DSys j3dSys` object,
the complete unchanged root constructor, and the four unchanged table builders
it calls. It also supplies their original array definitions,
`J3DSys::sTexCoordScaleTable[8]`, and `j3dDefaultViewNo`. Existing current matrix,
current scale, and parent scale definitions remain in `J3DJointTreeCompat.cpp`.
No other production file was changed by this task, and no placeholder methods,
virtual methods, or replacement class layouts were introduced.

The constructor and global definitions come from
`src/JSystem/J3DGraphBase/J3DSys.cpp`; table definitions and builders come from
`src/JSystem/J3DGraphBase/J3DTevs.cpp`. The native source preserves their bodies
exactly, including the original `NULL` spelling. Its only includes are the
existing actual `J3DSys` header, which already supplies the SDK types and GX
enum constants used by the tables.

The constructor calls texture-coordinate, TEV-swap, alpha-comparison, and
Z-mode table construction in that order, initializes the view matrix to
identity, sets flags/material mode to zero and draw mode to one, and clears
the original listed model/render pointers. All eight texture-coordinate
scale entries become `(1,1,0,0)`.

It intentionally does **not** initialize `mCurrentMtxCalc`, texture-cache
region count/storage, `mNBTScale`, or padding. The real static global receives
normal C++ zero-initialization before its constructor runs. An independently
constructed automatic object has only the fields written by the original
constructor; the provider does not add a blanket clear. The builders and
`PSMTXIdentity` require no runtime services, graphics context, or allocation.

The four arrays retain their exact dimensions: 7,623 texture-coordinate bytes
(11 × 21 × 11 triples), 1,024 TEV-swap bytes, 768 alpha-comparison bytes, and
96 Z-mode bytes. Their original loops fill those exact extents. The texture
matrix enum sequence is `30,33,36,39,42,45,48,51,54,57,60`.

## Original compiler evidence

Both complete root SDK translation units were compiled with GC 3.0a3 and
`configure.py`'s `cflags_jsys`, then compared against the current RMGK01 DOL
(SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`). No replacement include
overlay or native build was used.

| Function | Retail address | Bytes | objdiff |
| --- | --- | --- | --- |
| `J3DSys::J3DSys` | `0x804225D4` | 220 | 99.36364% |
| `makeTexCoordTable` | `0x80430A64` | 248 | 99.67742% |
| `makeAlphaCmpTable` | `0x80430B5C` | 104 | 99.61539% |
| `makeZModeTable` | `0x80430BC4` | 104 | 99.61539% |
| `makeTevSwapTable` | `0x80430C2C` | 64 | 97.81250% |

Every compiled function has the original size. All instructions, registers,
and memory offsets agree after mapping the retail unnamed data labels to the
source arrays and normalizing branch locations. The texture-matrix constant
bytes were checked directly against the DOL. The apparent objdiff mismatches
are data-symbol names and propagated relocation annotations, not changed loop
or constructor behavior.

`sys-verify-source.py` checks exact source copies, preserved body hashes,
declarations, and unchanged original global ownership. `sys-compiler-evidence.json`
records the reviewed object comparison and command paths. Generated commands,
objects, full objdiff data, and compiler logs are in
`build/xanime-core-matrix-calculation-20260903/sys/`. The parent owns native
linking and runtime validation.
