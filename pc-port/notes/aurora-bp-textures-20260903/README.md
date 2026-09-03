# Original GD texture commands through Aurora BP decoding

The original GD/J3D texture words can now select retained MEM1 texture data and
load/select palettes in hardware TMEM. This is a decoder and memory-semantics
change in Aurora. It does not activate a Game model/resource owner and does not
change a Game camera or movement algorithm.

## Why this boundary was needed

`lib/dolphin/gd/GDTexture.cpp` still emits the original five-byte BP words.
`GDSetTexImgPtr` and `GDPatchTexImgPtr` use `OSCachedToPhysical(pointer) >> 5`;
the patch writes exactly three address bytes. Native J3D's `loadTexNo` records
four BP words (20 bytes), plus seven words (35 bytes) for a palette. Its material
patch loops and display-list swapping rely on those lengths. `GXCallDisplayList`
copies command bytes into the FIFO, so adding metadata keyed by the original
display-list address would lose information when the original code copies or
swaps a list.

Before this change, raw Image3 writes did not set a texture data pointer, TLUT
selection writes were ignored, and the TLUT-load decoder treated the hardware
TMEM offset as a native logical palette slot. Native GX calls worked through
their separate 64-bit pointer metadata command, which original GD does not emit.

## Implemented semantics

| Original fields | Aurora behavior |
| --- | --- |
| Image3 bits 0–23 | Physical byte address is the field shifted left 5. Resolve it only in the actual MEM1 mapping. |
| Image0 width/height fields | Each stores dimension minus one. The maximum field value means 1024, not zero. |
| Mode0 mip filter / Mode1 max LOD | Raw BP samplers derive mip usage from hardware filter bits, round fractional maximum LOD upward, and cap the chain at the last one-texel level. |
| LoadTLUT0 bits 0–23 | Physical source byte address, shifted left 5. |
| LoadTLUT1 bits 0–9 | TMEM destination byte offset, shifted left 9. |
| LoadTLUT1 bits 10–20 | Eleven-bit count of 32-byte lines. Zero copies nothing; 1024 copies the full 16K-entry palette. |
| Texture TLUT bits 0–9 / 10–11 | Per-texture TMEM offset and palette format. The load itself has no palette-format field. |

The command processor retains a 1 MiB TMEM byte store, allocated on the first
load. Each load copies source bytes when the FIFO executes the command. Later
source writes do not affect TMEM until another load. Overlapping loads update
only their destination range; existing selections see those bytes with their
own format. C4, C8, and C14X2 read 16, 256, and 16384 entries respectively.

Raw texture and TLUT descriptors use content hashes rather than borrowing a
native GX object's identity. Register writes also clear stale native object
identities, including when the BP register bits happen to equal their previous
shadow value. Hardware palette changes therefore cannot reuse a stale native
texture cache entry. Dynamic palette conversion with no object identity creates
a distinct output per recorded conversion, preserving earlier draws.

Texture source extents are checked before sampling/upload, including the mip
chain. Palette source extents are checked before copying. These checks remain
active in release builds. Invalid ranges fail visibly rather than truncating a
pointer or reading beyond mapped memory. Temporary raw Image3 values emitted
before a native GX metadata command are allowed; they must be replaced before
sampling if they do not name mapped memory.

Native GX still uses its existing full-pointer metadata path. Its former fake
BP TLUT trigger encoded `logicalSlot` and `entries - 1` with source address zero.
That word now encodes a zero-line transfer, retaining its byte count; the
following native metadata command supplies the actual pointer, format, count,
identity, and version. Native GX logical slots and raw hardware TMEM selections
are explicitly distinct. This change does not provide the still-missing native
GX TLUT region/callback APIs or emulate a shared hardware region for those
logical slots.

## Mapped resource ownership

Aurora's `OSInit` allocates real MEM1 through `lib/dolphin/os/OSMemory.cpp` and
initializes the arena in `OSArena.cpp`. Merely setting `config.mem1Size` does not
call `OSInit`; no current PC production caller invokes it yet. The typed owner
must first enter this original OS initialization lifecycle.
`OSAlloc.cpp` supplies working `OSInitAlloc`,
`OSCreateHeap`, `OSAllocFromHeap`, and `OSFreeToHeap`, with 32-byte-aligned payloads.
The new fixture allocates its sources through those actual APIs and verifies the
original physical-address round trip. No virtual-address registry, pointer
interning, or display-list sidecar is added.

The previous portable MEM1 allocator used `calloc`, which does not guarantee a
32-byte base alignment. That would make an absolutely aligned OS heap pointer's
physical offset 16 modulo 32 on a 16-byte-aligned base, losing address bits in
the BP shift. It now uses `SDL_aligned_alloc(32, size)` and explicitly zeroes the
requested extent. A process-lifetime owner calls `SDL_aligned_free` on that
allocation. The Windows debug VirtualAlloc path retains its full reservation
and releases it with `VirtualFree(..., MEM_RELEASE)`. OS initialization remains
process-idempotent, and renderer shutdown does not invalidate live OS resources.

The current PC `JKRHeap` declaration is empty and the aligned-new provider uses
host `posix_memalign`. Existing decoded resource vectors likewise reside outside
MEM1. They cannot be passed to original physical-address GD writers. The next
typed texture/resource owner can reserve part of the real arena for a retained
OS heap and place texture/palette payloads there. It must initialize allocation
ownership once, retain ranges while referenced by any reusable display list,
and finish FIFO reads before freeing/reusing them. Calling `OSInitAlloc` per
resource would replace the global heap table and is not an acceptable owner.

This decoder checks mapped extents, not OS heap allocation liveness. It does not
claim to detect a dangling/reused resource allocation. The owner contract is
the same explicit command-lifetime requirement as ordinary native GX inputs.
The fixture drains the FIFO before freeing allocations. Aurora's configured
MEM2 size does not currently create a second mapping; raw MEM2 addressing and
preloaded texture sampling from TMEM remain outside this change.

## Source evidence

Aurora baseline: `7fecb5d1931f495d2820d2df8762715c0f71428a`.
Local Dolphin baseline: `ed8e44d4be114fc70258fbfaeb239f3e83b041fe`.

- `aurora/lib/dolphin/gd/GDTexture.cpp:27–36,57–58,83–104`: unchanged original
  physical-address writers, palette units, and command counts.
- `aurora/include/dolphin/gd/GDTexture.h`: BP masks/shifts and original APIs.
- `dolphin/Source/Core/VideoCommon/BPMemory.h:1072–1104,2254–2280`: Image3,
  TLUT-selection, and eleven-bit TLUT load-count fields.
- `dolphin/Source/Core/VideoCommon/BPStructs.cpp:2234–2260`: palette source/dest
  shifts, count times 32, and copy-at-command-execution semantics.
- `dolphin/Source/Core/VideoCommon/TextureInfo.cpp:28–40,98–108`: physical texture
  address, palette offset, mip filter, ceiling of max LOD, and dimension cap.
- `pc-port/src/compat/J3DTevsCompat.cpp` / `J3DMatBlockCompat.cpp`: original
  fixed-size `loadTexNo` and material patch strides. These are owned/verified by
  the parent task; this tranche does not edit them.

`source-correspondence.txt` records hashes and confirms the GD writer source and
header are byte-identical to the Aurora baseline. This is source-backed hardware
compatibility work, not a new Game decompilation or an original-compiler match.

## Validation

```
cmake --build build/aurora-upstream-merge-tests --target gx_fifo_tests gx_texture_cache_tests -j 6
build/aurora-upstream-merge-tests/tests/gx_fifo_tests
build/aurora-upstream-merge-tests/tests/gx_texture_cache_tests
cmake --build build/aurora-upstream-merge-tests --target aurora_gx -j 6
cmake --build build/aurora-upstream-merge-tests --target os_alloc_tests -j 6
build/aurora-upstream-merge-tests/tests/os_alloc_tests
```

All 255 GX FIFO/GD tests, 21 texture-cache tests, and six allocator tests pass. The complete
`aurora_gx` library, including renderer texture replacement integration, builds.
Logs are alongside this note.

The new allocator fixture invokes the actual MEM1 allocator with a requested
extent of 65543 bytes, checks zeroed contents and the exact advertised end, then
allocates and frees five sizes through an actual OS heap. Every physical offset
survives the original shift-left/shift-right texture address encoding. The test
process exits through the actual allocation owner's matching deleter.

Eleven new GD tests cover all eight texture units, actual OS heap ownership,
20-byte texture layout, 30-byte palette loads, three-byte patching after a byte
copy, physical zero and offsets, full-range boundaries, palette snapshot/reload,
overlap, independent selection formats, the 1024-line palette count, native/raw
switching, and mip bounds. One test runs the actual palette converter and checks
RGBA red/green pixels from big-endian original RGB565 palette bytes. Three new
cache tests cover changed TMEM static results, distinct dynamic conversion
results, and rejection of a sampled texture exceeding MEM1.

Two existing assertions were corrected to the hardware contract: maximum raw
dimensions are 1024, and a zero-line palette trigger does not load a native slot.
There are no new renderer stubs for address conversion, BP decode, palette
copying, or texture conversion. Existing CPU-test GPU allocation/queue stubs
remain, so these checks do not constitute a new end-to-end GPU raw-GD demo.
The parent task owns full PC integration and runtime gates.
