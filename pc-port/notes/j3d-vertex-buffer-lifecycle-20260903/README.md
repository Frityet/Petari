# Original J3D vertex-buffer lifecycle and transform accessor

Recovered against the current RMGK01 DOL on 2026-09-03 after parent checkpoint
`373060717` (Aurora merge). This closes a prerequisite for actual J3DModel
construction and data attachment. It does not activate MarioAnimator, create an
animation-name substitute, change simulation rate, or supply a jump handler.
The wider ownership frontier remains in
`../actual-mario-animation-owner-20260903/audit.md`.

## Root-first recovery and native ownership

Root `src/JSystem/J3DGraphBase/J3DVertex.cpp` now contains the original
`J3DVertexBuffer::setVertexData`, `init`, and destructor bodies. Their literal
native copies live in `pc-port/src/compat/J3DVertexBufferCompat.cpp`. Existing
J3DVertexData and J3DDrawMtxData providers remain in their original native
locations, so the complete root SDK unit was not imported a second time.
The root and native J3DVertex headers are unchanged and byte-identical.

The small `XanimeCore::getJointTransform` definition was missing in the current
root source despite its declaration and an existing PC-only body in Mario.cpp.
Retail verification confirmed that exact null-check/index body. It is now in
root `src/Game/Animation/XanimeCore.cpp` and its unchanged native mirror. The
separate jump worker removed only the duplicate from native Mario.cpp after
comparing the body. Retail's symbol resides in its Mario object at
`0x802AEC3C`; placing this recovered class method in XanimeCore's authoritative
source does not change its semantics. No original or native header changed.

## Retail contracts retained

- `setVertexData` stores the data pointer, borrows position, normal and **color
  channel zero** into local slot zero, clears all alternate local slots, seeds
  transformed position/normal slot zero with those original arrays, clears the
  alternate transformed slots, then calls `frameInit`. It does not copy array
  data or use the resource's color channel one as the alternate buffer.
- `init` clears all fourteen pointer fields and calls `frameInit`. Each pair
  uses the original assignment order. Calling it on an attached buffer forgets
  those borrowed pointers; it does not free them.
- `frameInit` selects the three local slot-zero pointers as current arrays.
  Transformed slots are independent. Swapping local slots does not itself
  change current selection; a later `frameInit` does.
- The destructor has an empty source body. The original compiler emits its
  normal delete-this check, but there is no cleanup of attached resource data,
  local arrays, or transformed arrays. Their owner must keep them alive and
  release them separately. The native provider adds no hidden allocation owner.
- `getJointTransform` returns null when the optional transform list is absent;
  otherwise it returns the indexed original transform. Retail has no bounds
  check. The recovered typed indexing naturally follows the host element size,
  including pointer-width changes, rather than hardcoding the Wii `0x70` stride.

No extra null-input guard was added to `setVertexData`: retail requires a real
J3DVertexData object. A valid object containing null arrays is supported and
resets the current pointers to null through the original code.

## Original compiler proof

`verify-object.py` compiles the actual complete root translation units using
GC 3.0a3 through the configured Shift-JIS wrapper. J3DVertex uses exactly
`configure.py`'s `cflags_jsys`; XanimeCore uses `cflags_game`; both use RMGK01
`VERSION=0`. A separate layout probe includes the actual root headers. No
replacement header overlay, dummy object, or native compiler is used by this
verification.

The DOL is `build/compat-math-oracle/main.dol`, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`. The script uses the current symbols
and splits through dtk's `--no-update` mode without changing their production
configuration. All results below were obtained in this tranche.

| Function | RMGK01 address | Retail / compiled bytes | objdiff |
| --- | --- | --- | --- |
| `J3DVertexBuffer::setVertexData` | `0x80423874` | 72 / 72 | 100% |
| `J3DVertexBuffer::init` | `0x804238BC` | 64 / 64 | 100% |
| `J3DVertexBuffer` destructor | `0x804238FC` | 64 / 64 | 100% |
| Existing inline `frameInit` | `0x804239B4` | 28 / 28 | 100% |
| `XanimeCore::getJointTransform` | `0x802AEC3C` | 32 / 32 | 100% |

In addition to objdiff, every instruction word is compared after masking only
the relocated REL24 displacement bits. Opcodes, AA/LK flags, register operands,
field offsets and all other instruction bits must match. Relocation position,
kind, symbol and addend must match exactly. The only relevant dependencies are
`frameInit` for attachment/init and ordinary `operator delete` in the compiler's
destructor ABI. The getter has no helper calls.

The PowerPC layout probe verifies the `0x38` buffer size and every buffer field
used by these methods; resource position/normal/color offsets; `0x70` transform
size; and Core transform-list offset `0x14`. The compiler emits only the existing
warning about nontrivial J3DTransformInfo in a union in the Core header.

Evidence is recorded in `compiler-evidence.json`, `function-comparison.txt`,
`commands.json`, and `original-compiler.log`. Complete generated objects, split
DOL objects, and objdiff JSON stay under ignored
`build/j3d-vertex-buffer-lifecycle-20260903/`. The evidence contains original
function hashes, source/header/tool hashes, exact relocations and checked layout.
`verify-source.py` checks the exact root/native method copies, unchanged headers,
compile-time source hashes and unique native providers. It writes
`source-correspondence.json` with exact source locations and body hashes.

## Native fixture and integration

`pc-port/tests/OriginalJ3DVertexBufferTests.cpp` uses constructed J3DVertexData
and J3DVertexBuffer objects with retained typed arrays. Four test groups cover:

1. All initial pointers, attachment identity, resource aliasing, valid empty
   data, and reset without resource mutation.
2. Independent transformed/local swaps, explicit current selections, next-frame
   restoration, and reattachment clearing alternate state.
3. Heap-allocated buffer destruction while borrowed resource/alternate arrays
   remain owned and usable by their fixture.
4. An actually constructed Core and ModelData/joint fixture, optional transform
   allocation, native typed stride, mutation of the selected transform and
   shared-core identity. Its fixture explicitly releases original Core storage
   once because the original Core destructor is empty.

The parent wired `smg-pc-original-j3d-vertex-buffer-tests` with the original
Core target's dependencies. The production provider is covered by
`src/Game/xmake.lua`'s `../compat/**.cpp` glob. The native build passed; all four
fixture groups passed against the actual linked provider. The original Core,
Binder/KCL, 29-case Aurora native, and seven camera regression programs also
passed. The complete application and showcase build passed. These results
verify the bounded lifecycle; actual Mario model ownership and jumping remain
open.
