# NameObj registry allocation boundary

The first NameObj registration could allocate global registry buckets and names
from the currently selected original JKR heap. Those host containers outlive a
scene heap, so later lookup or process shutdown could access retired storage.
Registry insertion, name replacement and snapshots now explicitly select host
allocation. Game NameObj instances still use the selected original heap.

The existing scene scheduler heap fixture now starts with its first registry
registration inside an actual JKR allocation domain, renames the object with a
long name, and retains a snapshot beyond heap retirement. It verifies registry
names and snapshot storage are outside JKR heaps, while the object is inside
the original heap. Solid-heap object allocation remains until heap reset;
registry operations consume no additional arena storage. Existing scheduler
registration and teardown checks also pass.

Validation: rebuilt `smg-pc-scene-scheduler-heap-tests` on macOS ARM64 with LLVM
23, then ran the executable to normal process exit. See
`scheduler-guard-runtime.log`. The complete scenario catalog owner is a separate
pending checkpoint. No Game source changes are part of this fix.
