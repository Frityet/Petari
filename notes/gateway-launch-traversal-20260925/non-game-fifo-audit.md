# Non-Game graphics FIFO audit

2026-09-25, following the first-launch `SpinDriverPathDrawer::sendPoint` crash at the Wii write-gather address. Read-only bounded inspection; no source changes, builds, tests, or controller input in this lane.

No additional direct hardware FIFO/MMIO stores were found in current `src/JSystem`, `src/nw4r`, `src/render`, `src/runtime`, or the shared `src/revolution`, layout, camera and resource sources. Searches covered `GXWGFifo`, write-gather/FIFO aliases, `0xcc008000` and its decimal spelling, CC/CD hardware register prefixes, CP/PE/PI register aliases, volatile accesses and pointer-from-address spellings. The `0xdc00` matches in TextEncoding are UTF-16 surrogate values, not register accesses. Audio status volatile fields are unrelated and unchanged.

Actual compiled paths inspected:

- `src/JSystem/J3DGraphBase/J3DFifo.hpp:6–38` already emits XF/CP/BP/index commands with `GXCmd1u8/u16/u32`. `J3DShape.hpp:47` emits its remaining matrix-index words through the same calls.
- `src/JSystem/J3DGraphBase/J3DGD.cpp:540–627` routes immediate matrix/texture-cache commands through those helpers. `J3DGD.hpp:49–60` sends floating-point bits via `GXCmd1u32`; its separate `J3DGDWrite_*` helpers write ordinary display-list memory through `__GDWrite`, not MMIO. These owners are explicitly included by `src/Game/xmake.lua:127–139`.
- `src/nw4r/lyt/lyt_common.cpp:126–163` and `src/nw4r/ut/ut_CharWriter.cpp:202–216` already use GXPosition/GXColor/GXTexCoord vertex entry points. Their source inclusion is explicit in `src/Game/xmake.lua:47–57`.
- `aurora/lib/dolphin/gx/GXVert.cpp:259–260` implements the GXCmd calls with `aurora::gx::fifo::write_u32/write_f32`. `aurora/include/revolution/gx/GXRegs.h` macros similarly call GXWriteFifo functions, implemented by `GXRegs.cpp` through the native FIFO stream. These are the appropriate existing architecture boundary.

Root owns replacing the path drawer's five raw float stores with `GXPosition3f32` plus `GXTexCoord2f32`, and hiding the legacy hardware lvalue under `!TARGET_PC` in Aurora's GXVert header. This inspection found no non-Game consumer requiring that lvalue to remain available on PC. The compile guard will catch future imported donor stores instead of permitting another native dereference of the Wii register address. Game-wide raw-store inspection belongs to the other lane; no inactive donor files were imported or changed here.

This is a source audit, not a rendering or launch-traversal validation result.
