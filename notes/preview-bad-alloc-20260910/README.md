# Preview primary allocation crash

Exact root and packaged binary SHA256: `05028c3a289d505f68c885ff711b35de5b67f3e6e4a69ea76be40325ca3dd753`.

An untouched Gateway run under LLDB, with `--max-frames 1200`, stopped at its first `__cxa_throw` on simulation frame 412. The exception was `std::bad_alloc`: native collision traversal requested 1664 bytes while the selected original `JKRSolidHeap` had only 1232 bytes remaining. No scripted input was required. Full captured session is `lldb-session.log`; compact source-bound stack and heap/frame observations are `first-throw-stack.txt`. Owned process 71717 was killed and its debugger exited after capture.

The throwing allocation is `StageCollisionService::sphere_contacts_impl` reserving 32 native `IndexedContact` entries, called through `store_sphere_contacts`, the original Binder, and MarioActor movement. The fixed-size original heap cannot reclaim these per-query temporaries individually. Depth's disjoint `GameMapCollisionCompat.cpp` change places sphere/point query result storage under the native allocator and tests persistent strike metadata after arena retirement.

A concrete second path bypasses that wrapper: `MR::isExistMapCollision` and `first_line_hit` call `StageCollisionService::line_cast` directly. Its native traversal vector also used the selected Game heap. This cohort adds the missing host allocation scope at `line_cast`, matching existing `line_hits` and `area_polygons` ownership. It changes no Game source, query geometry, heap budget, or callback semantics.

`StageCollisionRegistrationTests.cpp` now runs 2048 pairs of original MR hit/miss queries under an actual 64 KiB Game heap, requires unchanged free capacity on every pair, verifies subsequent original allocation routing, and queries again after heap retirement. Both production and fixture translation units compile successfully with LLVM 23. The targeted regression builds and runs successfully: all four stage-collision registration groups pass, including 4096 repeated native queries with no Game heap consumption. The frozen original layout group fixture also builds and runs successfully with the exact local RVZ. `runtime-results.json` records binary hashes and both zero exits; target-specific logs preserve full output.

The slow frame-rate diagnosis is separate and parent-owned. This stack establishes the memory failure; it is not GPU or FPS evidence.
