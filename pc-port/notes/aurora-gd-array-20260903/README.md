# Native GD array display lists

Aurora now implements ordinary `GDSetArray` and `GDSetArrayRaw`, repairs
`GDPatchArrayPtr`, and exposes `GXSetArrayBase` for an original CP base write
that preserves the current stride. These use the existing Aurora array command
and actual FIFO decoder. No renderer/game-specific decoder path was added.

The root SDK `src/RVL_SDK/gd/GDGeometry.c` defines the original contract:
`GDSetArray` writes a base and a stride; `GDSetArrayRaw` accepts a physical
address rather than a C++ pointer. Native adaptations preserve that contract:

- `GDSetArray` records the full host pointer, native-endian flag, and existing
  unsized-array flag. Actual indexed draws/XF loads determine the required span.
- `GDSetArraySized` retains its explicit resource size and byte order.
- `GDSetArrayRaw` resolves the SDK physical address through the real
  `OSPhysicalToCached` service. Current Aurora implements a configured MEM1
  range; this change does not add MEM2 or accept truncated native addresses.
  Physical zero resolves to the start of that range, as the OS API specifies.
- `GDPatchArrayPtr` writes the full eight-byte native pointer payload. Callers
  seek to native array-command start + 3; size, flags, and stride are preserved.
- `GXSetArrayBase` uses the same GX base emitter as both existing `GXSetArray`
  forms, and emits no stride command. It replaces the binding with an unsized,
  native-endian host pointer while preserving the selected array's stride.
- Normal/binormal/tangent bindings keep the original NBT-to-normal slot mapping.
- The existing GD buffer-alignment assertion now uses `uintptr_t` to inspect a
  host pointer without narrowing it to `u32`.

An Aurora base command is 16 bytes (3-byte command, 8-byte pointer, 4-byte size,
1-byte flags). Including the 6-byte stride command makes each GD binding 22
bytes. The original VCD/VAT sequence with twelve bindings is now 303 bytes
before padding and **320 bytes** after 32-byte padding. The original version is
183/192 bytes respectively. The CPU fixture verifies this bound directly.

Validation: **244/244 FIFO/display-list cases pass**, including all eleven new
GD cases. The new tests use the real GD writers, GX/FIFO decoder, OSAddress and
OSCache providers, with fixture-owned MEM1 storage. They cover all sixteen
array slots and NBT aliasing, pointers above 32 bits on this host, sized resource
metadata, physical offsets and physical zero, pointer patching, actual indexed
draw replay, stride-preserving base replacement with padded vertex records,
and indexed matrix loads with known float values. Existing 233 cases also pass
following the shared GX emitter refactor. No GPU or full-game run is claimed.

Reproduce using the existing isolated CMake configuration described in
`../aurora-upstream-merge-20260903/tests.md`:

```sh
python3 pc-port/notes/aurora-gd-array-20260903/verify.py --run
```

The script builds only `gx_fifo_tests` with two jobs, runs its CPU cases, and
records source hashes and XML outcomes. Without `--run`, it records evidence
from the existing test output. Sources belong to the nested Aurora repository;
the parent owns the integration build and nested/parent checkpoints.
