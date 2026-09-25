# Canonical J3DPacket restoration — 2026-09-25

Completed the bounded batch requested after the full render audit:

- Added `src/JSystem/J3DGraphBase/J3DPacket.cpp` from the current `decomp/src/JSystem/J3DGraphBase/J3DPacket.cpp` in full. Its sole source change from donor is `<mem.h>` to standard `<cstring>` for native compilation.
- Deleted `src/compat/J3DPacketCompat.cpp`.
- Changed `J3DShapePacket::newDifferedDisplayList` declaration in `src/JSystem/J3DGraphBase/J3DPacket.hpp` from `int` to original `J3DError`.

This restores the original allocation-error return instead of changing an error to zero, and restores the original seven-entry register/size tables. The deleted fragment used eight entries but looped over seven, adding ambient-color budget and skipping the indirect-stage budget. Current donor allocates a 45-byte indirect block (rounded to 64) even without the count-mask fields; the old table produced zero for that flag alone.

The original allocation/GD/FIFO/interrupt/cache calls remain identical. There are no native address casts or disk-endian accesses in this translation unit requiring another adaptation. Existing surrounding `J3dCommandScope` serialization remains at native resource/model entry points. Header pointer-sized `setUserArea(uintptr_t)` is unchanged. No additional lifetime/allocator behavior was invented.

`validate-j3d-packet-restoration.py` passed all 10 source checks, with the result in `j3d-packet-source-validation.json`: entire-donor equality after the include substitution; deleted fragment; correct public return; original error branch and seven-entry tables; preserved SDK boundaries; exactly one canonical provider for all 32 methods and both static context globals. This is source validation, not compilation or runtime proof.

A real heap-exhaustion test is not claimed: current JKR allocation routing throws `std::bad_alloc` on exhaustion (`tests/JkrAllocationDomainTests.cpp:193-201` documents that path), so injecting a fake status-return allocator would not exercise the actual native failure behavior. Parent owns the full build and any runtime checks.

**Required parent build edit:** add `../JSystem/J3DGraphBase/J3DPacket.cpp` to the Game target's explicit canonical sources in `src/Game/xmake.lua`. The old compat wildcard no longer finds the deleted provider, so no additional exclusion is needed. No xmake files/builds/staging/commits were performed in this lane.
