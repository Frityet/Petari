# Native JMap ownership in original heaps

The native `JMapInfo` retains C++ owners for decoded BCSV rows, string caches,
names, overrides and child/rail maps. Original Game allocation cohorts can
retire an entire `JKRSolidHeap` without invoking ordinary `delete` on each map.
The map therefore now participates in the real `JKRDisposer` list, so the
original heap's virtual destructor dispatch releases those native owners.

`compat/NativeJkrDisposer.hpp` is a reusable native identity adapter over the
actual `JKRDisposer`. Its copy and move constructors invoke the original default
constructor at the new object's address. Assignment leaves the destination's
intrusive registration untouched. `JMapInfo` privately inherits this adapter
and defines its destructor; its metadata copy/move behavior preserves shared
table identity. Host and stack objects do not register because the original
constructor uses actual heap address containment, not the current allocation
scope.

All native metadata construction and mutation occurs under
`JkrHostAllocationScope`, including copied names/maps and nested `make_shared`
child/rail maps. A nested shared map must not be independently disposed by the
Game heap while a host copy still owns it. The BCSV constructor accepts a const
reference and copies the table inside the host scope, avoiding allocations in
a by-value argument before the constructor body. `from_bcsv` retains efficient
move construction directly from the parsed table inside that same scope.

This changes the already-native parser's architecture and ownership boundary;
it does not recover or claim byte-for-byte execution of the original JMap
parser algorithm. BCSV values, queries, shared string identity and placement
metadata are preserved. Original Game algorithms and JKernel disposal bodies
are unchanged.

## Source correspondence

`src/JSystem/JKernel/JKRDisposer.cpp` and
`pc-port/src/compat/JKRDisposerCompat.cpp` are byte-identical. The original
constructor appends its own `mLink` when `JKRHeap::findFromRoot(this)` returns a
heap; its destructor removes that same link. Original `JKRHeap::dispose` and
range disposal explicitly call the virtual `~JKRDisposer`; `JKRSolidHeap`
calls the disposer list before resetting head/tail allocation positions.
The new adapter uses those existing providers directly, without a second
registry or an emulated heap callback. Run `verify-source.py` to recheck and
record the exact source hashes.

## Focused runtime coverage

`smg-pc-original-jmap-heap-lifetime-tests` uses actual `JkrHeapRuntime`,
`JkrAllocationDomain`, `JKRSolidHeap` and disposer lists. Five groups cover:

1. Reusable adapter copy/move construction, assignment and explicit deletion.
2. JMap range disposal, negative-alignment tail allocation/freeTail, explicit
   deletion and later freeAll without repeat disposal.
3. JMap copy/move/self-assignment registration and retained metadata values.
4. Host-only metadata allocations and child/rail ownership surviving complete
   source heap destruction; all shared metadata expires with its final owner.
5. Actual allocation-domain retirement dispatching JMap cleanup.

The metadata test checks original heap free bytes before and after copies and
mutations, exact disposer counts, actual allocation provenance, and weak/shared
owner counts. It does not infer cleanup solely from successful process exit.

Build and execution results are recorded in `validation.md` once completed.
