# Direct Game GX FIFO write audit

Trigger: the supervised route crashed after the last actor sample at frame 24300 and the last input log at frame 24303 in `SpinDriverPathDrawer::sendPoint`, whose donor code wrote five floats through Wii MMIO address `0xCC008000`.

Read-only bounded audit of current `src/Game/` source/header files for `GXWGFifo`/`PPCWGPipe`/WGPIPE, FIFO address literals (including decimal form), graphics MMIO ranges, raw GX write macros, volatile-pointer access, and relevant store assembly. No independent build/test/controller operation was performed.

| Owner | Finding and disposition |
| --- | --- |
| `src/Game/MapObj/SpinDriverPathDrawer.cpp::sendPoint` | The only direct Game FIFO stores found. Root owns and has now replaced the three position float stores with `GXPosition3f32` and the two texture-coordinate stores with `GXTexCoord2f32`. No changes to geometry, values, command order or stage predicates are required. |
| `src/Game/System/MainLoopFramework.cpp::handleGXAbortAlarm` | Emits byte `0x61` and BP word `0x5800000f` with `GX_WRITE_U8/U32`. Already includes `revolution/gx/GXRegs.h`; those macros call actual native `GXWriteFifoU8/U32`. No raw MMIO remains here and no edit is needed. |
| Other `src/Game/` files | No additional direct graphics MMIO/FIFO stores found by this source scan. Remaining volatile fields/constants and guarded math assembly are not graphics writes. |

Aurora already supplies the required general implementation: `include/dolphin/gx/GXVert.h` declares native GX vertex writers under `TARGET_PC`; `lib/dolphin/gx/GXVert.cpp` emits position and texture coordinates in the same order via `fifo::write_f32`. `lib/gx/fifo.hpp` converts values into big-endian command bytes before recording, so replacing stores with these APIs preserves the original stream on a little-endian host. `lib/dolphin/gx/GXRegs.cpp` provides the raw command writers used by MainLoopFramework.

At audit start the SDK header exposed `GXWGFifo` to PC callers. Root subsequently restricted the hardware lvalue to non-PC builds, so future donor imports with untranslated stores fail compilation. This audit lane makes no SDK change and introduces no alternate provider, stage condition or rendering stub. After root's sendPoint edit, the repeated Game scan found zero raw-pipe identifiers or graphics MMIO address literals, with only the two already-native MainLoopFramework GX_WRITE calls remaining.

Files written by this lane: this note only. Production correction belongs to root's SpinDriverPathDrawer change. This establishes source coverage of direct stores, not runtime rendering correctness of the resumed launch.
