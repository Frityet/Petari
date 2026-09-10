# Post-renderer shutdown abort: native collision strike storage

## Actual reproduction

The requested preview binary SHA `05028c3a289d505f68c885ff711b35de5b67f3e6e4a69ea76be40325ca3dd753` renders screenshot frame 1, exits its Gateway loop, destroys the scene and renderer, then aborts. `command.json` records the exact LLDB run with `--max-frames 2 --screenshot-frame 1 --exit-after-screenshot`, the real Korea RVZ, abort/assert breakpoints only, and explicit debugger cleanup. `first-abort.log` contains the first failure stack.

The preceding `Device was destroyed` notification does not identify the fault. The first abort is malloc's invalid free of a 640-byte `std::vector<HitInfo>` backing allocation at `dyld::ThreadLocalVariables::finalizeList`, after normal renderer teardown. `GameMapCollisionCompat.cpp::strike_infos()` owns the matching thread-local native vector. The allocator routes its delete through the retired original arena's now-absent provenance, then incorrectly reaches `std::free` on that old arena address.

## Minimal shared compatibility fix

The thread-local strike buffer has exactly three allocating mutation paths. `store_line_hits` already enters Aurora's host allocation scope. The two missing guards were `store_sphere_contacts` (shared by ordinary sphere, moving-reaction and thickness APIs) and `Collision::checkStrikePointToMap`. Both now enter the same existing native allocation scope. No collision math, count/order rules, Game code, cleanup policy or heap lifetime changes. The first linked regression correctly caught an original-callback routing gap: the parts-filter wrapper and line traversal already re-entered the client domain, but the triangle-only wrapper in `HitInfoCompat.cpp::make_collision_triangle_filter` did not. A third one-line change now enters `ClientAllocationScope` immediately before that actual original `TriangleFilterBase::isInvalidTriangle` call. This keeps query scratch storage native while restoring original callback allocation behavior.

## Regression and validation

The existing `LineCollisionQueryTests.cpp` fixture now runs a new native strike-lifetime group first, before its thread-local buffer has capacity. Under a real 64 KiB Game child heap it performs an actual point hit (first allocation), then an actual sphere query against three real KCL prisms (capacity growth), checks host pointer provenance and zero Game heap consumption, and exercises a triangle filter that allocates in the original Game heap. After return it checks that caller routing is restored. The entire original arena is then retired before the latest contact values are read and a normal miss clears the native results. Process exit subsequently destroys the thread-local backing storage normally.

The initial two changed translation units compiled with exact native compile database arguments, exit 0 (`compile.json`). The final three-source cohort subsequently built as the official `smg-pc-line-collision-query-tests` target, exit 0. The exact new group runs with `--strike-heap-lifetime` and exits 0 through normal process/TLS destruction; `strike-heap-runtime.json` records commands, durations and binary SHA, and `strike-heap-runtime.log` records the successful assertions. Frozen final source bytes are in `source-manifest.json`.

The first full linked fixture attempt found and led to fixing the callback routing gap above (`fixture-runtime.log`). The second full attempt passed the new group and then failed the unrelated older water-surface fixture with `Register objects only inside their active scene execution owner` (`fixture-final.json`). That legacy setup was not migrated; the narrowly selectable new group supplies the bounded ownership proof. Do not claim the whole legacy fixture passed.

An independent reproduction by the audio agent also found the preview's primary `std::bad_alloc` in `StageCollisionService::sphere_contacts_impl`: a 32-entry `IndexedContact` scratch-vector reserve (1664 bytes) under the same unguarded `store_sphere_contacts` caller. The wrapper's host scope covers both that observed primary allocation failure and the retained TLS vector lifetime. Exact primary stack: `../preview-bad-alloc-20260910/lldb-session.log`. This source-path proof is separate from live confirmation; the parent owns the rebuilt actual showcase run.

## Later integrated clean-exit proof

The parent subsequently rebuilt the combined fixes with LLVM23 O2 and retained debug assertions. The actual real-disc showcase completed 1200 ticks / 1068 presents and exited 0; binary `d1ad339684fdf16bff8125591f658c66b046c3db9a80d1f7de3c7ea5d6139240`. Exact run and shutdown evidence: `../preview-fps-crash-20260910/optimized-debug.json` and `.log`. This is a combined integration result after the additional attribute-group registry ownership repair described in `../native-tls-shutdown-next-20260910/README.md`.

The exact strike heap lifetime group was also rebuilt and rerun under the final LLVM23 O2 configuration, build0/runtime0; binary SHA `4db36fd89407006cefc394858eacd6a79a9477dbf58358176e8a800f2f0dcf64`. See `../preview-fps-crash-20260910/optimized-tests.json`.
