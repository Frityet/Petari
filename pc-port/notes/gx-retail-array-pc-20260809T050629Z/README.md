# Retail GX array surface on PC

Captured 2026-08-09 against root snapshot
`7ec60a93ae02f7813380cfd571ec4d2e9ab44ca9` and Aurora snapshot
`dcb5221a572095b965899748fb509dbebc183969`, with the working changes described
below applied but not staged.

## Result

Aurora now exposes the retail three-argument C surface
`GXSetArray(GXAttr, const void*, u8)` while keeping `GXSetArraySized` and the
existing five-argument C++ overload for native consumers that know the byte
span and byte order. The retail enum tags `_GXAttr` and `_GXTlutSize` are also
available. `nm` on the built Aurora archive reports separate unmangled
`GXSetArray` and `GXSetArraySized` definitions.

The retail surface does not invent a maximum allocation size. Each indexed
draw scans its FIFO vertex indices using the active VCD/VAT layout, calculates
the exact highest referenced byte, and uploads only that proven prefix.
Indexed XF loads use their real 16-bit source index and derive their span the
same way. A later wider index invalidates a previously narrow cached upload and
separates draw ordering. `GXInvalidateVtxCache` clears all cached array uploads
at its FIFO position so a subsequent draw observes source mutations.

Malformed array-base and indexed-XF packets fail before reading outside their
FIFO buffers. Referenced null arrays, 32-bit span overflow, and explicit-size
overruns fail rather than silently drawing or guessing. Once a retail prefix is
uploaded, draws that stay within that cached prefix no longer access the source
pointer. As with the hardware API, an unsized source must remain alive through
any first access, wider access, cache invalidation, or frame boundary that
requires it to be uploaded again. Callers that can provide an allocation bound
should use `GXSetArraySized` for full known-size overrun checking.

## Endianness contract

The three-argument retail surface is for arrays generated in native host
memory, and records little-endian data on the supported PC host. The FIFO unit
proof decodes native `float` position and indexed matrix arrays, and the real
Vulkan proof renders a green indexed quad from native host floats.

Resource/J3D buffers with big-endian element encoding remain on the explicit
path: `GXSetArraySized(attr, data, size, stride, false)` (or the source-compatible
five-argument C++ overload). FIFO proofs cover both a big-endian vertex resource
upload and big-endian indexed-XF matrix decoding. Array element endianness is
kept distinct from FIFO/display-list command endianness.

## Frozen Player compile frontier

Run:

```sh
python3 notes/gx-retail-array-pc-20260809T050629Z/verify_frozen_frontier.py
```

The frozen closure contains 96 translation units represented by 93 source
compile attempts. Its baseline passed 52. With the concurrent exact actor ABI
tranche plus this GX declaration tranche, 61 pass: `61/96`, with zero baseline
regressions. The actor tranche accounts for four passes; this GX surface adds
exactly five:

- `src/Game/Player/JetTurtleShadow.cpp`
- `src/Game/Player/MarioActorHand.cpp`
- `src/Game/Player/MarioActorShadow.cpp`
- `src/Game/Player/MarioModule.cpp`
- `src/Game/Player/ModelHolder.cpp`

The machine-readable result is in `verification.json`, with all 93 per-source
statuses in `live-frontier.tsv`.

## Verification

The final focused results are:

- GX FIFO encode/decode, span, ordering, malformed-input, endianness, and
  lifetime suite: 185/185 passed.
- AddressSanitizer-focused array/lifetime subset: 17/17 passed with leak
  detection enabled.
- Real Vulkan indexed-array render proof: 1/1 passed under `xvfb-run`.
- Repository Aurora-native integration tests: 27/27 passed.
- `smg-pc-game` rebuilt successfully as part of the native integration target.
- Frozen Player syntax frontier: 61/96, nine expected combined unlocks, zero
  regressions.
- `git diff --check`: passed in the Aurora nested repository.

The standalone Aurora commands used were:

```sh
cmake -S aurora -B /tmp/aurora-gx-array-proof -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DAURORA_DAWN_PROVIDER=package \
  -DAURORA_SDL3_PROVIDER=system \
  -DCMAKE_PREFIX_PATH=/home/vscode/.xmake/packages/l/libsdl3/3.4.8/e33732f2a33145cfb287c6138f75d8a4 \
  -DAURORA_ENABLE_CARD=OFF
cmake --build /tmp/aurora-gx-array-proof --target gx_fifo_tests gx_z_scale_offset_render_test -j 12
/tmp/aurora-gx-array-proof/tests/gx_fifo_tests
ctest --test-dir /tmp/aurora-gx-array-proof -R '^gx_z_scale_offset_render$' --output-on-failure
xmake build smg-pc-aurora-native-tests
xmake run smg-pc-aurora-native-tests
```

The sanitizer subset was configured separately with
`-DCMAKE_BUILD_TYPE=Debug`,
`-DCMAKE_CXX_FLAGS='-fsanitize=address -fno-omit-frame-pointer'`, and
`-DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address'`, then run with
`ASAN_OPTIONS=detect_leaks=1:abort_on_error=1`.

No Game source, creator activation, audio, RFL source, or shared xmake/CMake
path is part of this change.
