# MarioShadow RMGK02 reconstruction evidence

## Scope

- Added the previously absent `src/Game/Player/MarioShadow.cpp` with all 23
  retail class methods and the two compiler-emitted GX FIFO helpers.
- Expanded `MarioShadow.hpp` to the assembly-proven `0x34c` retail layout and
  complete virtual/non-virtual method surface.
- Added the assembly-proven static
  `JUTTexture::captureDolTexture(void*, int, int, int, int, bool, GXTexFmt)`
  declaration. No PC implementation or source activation was added.

## Focused objdiff

```text
ninja build/RMGK02/src/Game/Player/MarioShadow.o
build/tools/objdiff-cli diff -p . -u main/Game/Player/MarioShadow \
  -o pc-port/notes/mario-shadow-rmgk02-20260809T044210Z/focused-objdiff.json \
  --format json-pretty
```

- `.text`: 86.45779% across all 8,528 retail bytes
- `.data`: 100% (strings and 36-byte vtable)
- `.sdata2`: 95.833336%
- Exact methods/helpers: destructor, `draw1`, `initCaptureTex`, `draw`, all
  three getters, `GXPosition3f32`, and `GXTexCoord2f32`
- Near-exact methods include `setViewMtx` (99.93077%), `draw2` (99.92683%),
  `setUpdateFlag` (99.70588%), `calcView` (96%), and `draw3` (95.698746%).

The remaining differences are source-expression/compiler-shape differences;
the complete retail control flow, geometry buffers, capture path, volume path,
sorting, and GX state transitions are represented without inline assembly.

## Full verification

```text
ninja -j12 build/RMGK02/main.dol
build/tools/dtk shasum -c config/RMGK02/build.sha1
git diff --check
```

The full DOL check reports `build/RMGK02/main.dol: OK`.

## PC provider implication

Activating this source in a PC build will require a provider for the newly
declared static `JUTTexture::captureDolTexture` entry point. That production
bridge is intentionally absent from this root-only reconstruction lane.
