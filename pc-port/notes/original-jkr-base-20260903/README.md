# Original JKR heap, disposer and list foundation

The native port now imports the actual JKRHeap, JKRSolidHeap, JKRDisposer and
JSUPtrList algorithms. This replaces the empty native Heap/Disposer declarations.
The retained runtime, expanding heap, current-heap scopes and global allocation
routing are a coordinated companion change; no gameplay jump is activated here.

Root changes are deliberately small:

- Recover SolidHeap's individual `do_free(void*)` as the retail bare return at
  0x803A43C8. Solid allocations are reclaimed by tail or bulk release.
- Initialize the existing heap default at 0x806B26D8 to true. Its actual retail
  byte is 01; the previous root declaration incorrectly initialized it to false.
- Name the existing base bytes 0x6A/0x6B `mAllocMode`/`mCurrentGroupId`, matching
  the actual expanding-heap member accesses. This does not grow the PPC base.
- Supply the inline member `destroy()` with the same virtual `do_destroy()`
  contract as the existing, exactly matched static destroy wrapper.

`verify-original.py` uses the configured original compiler and verified RMGK01
DOL, compares 81 existing methods, and verifies the recovered free method plus
the maximum-allocation helper against every relocated retail byte. All ten
intrusive-list methods and the disposer lifecycle match exactly. Solid creation,
destruction, allocation dispatch and bulk/tail release also match exactly.
Percentages and explicit byte-equality results are in `original-evidence.json`.
Existing lower scores are retained honestly: Base construction differs through
JSUTree constructor inlining, Base range disposal through iterator/register
allocation, and Solid state registration through reordered independent loads.
The unusual low-four-bit expression in `getMaxAllocatableSize` was checked
against retail and preserved.

Native adaptations are confined to SDK/platform boundaries:

- Preserve full-width pointers in range disposal, alignment masks and diagnostic
  formatting; native object sizes/alignment naturally differ from PPC.
- Skip a missing parent list while destroying the parentless native root. Retail
  keeps that root for process lifetime. Native runtime teardown can release its
  caller-owned arena after all child heaps and resources have gone. Other tree,
  disposer and current/system-heap restoration behavior remains original.
- Check a failed SolidHeap allocation before performing pointer arithmetic.
- Record allocation provenance at the Base allocation/free/bulk/tail/destruction
  boundary. The native global delete provider can then use the original owning
  heap, including external-buffer children, without guessing from a pointer or
  requiring the caller's current heap to match.
- Send heap diagnostics through the native `JkrDiagnostics` boundary. On-screen
  JUT console/exception objects are not fabricated. Aurora now supplies its
  previously disabled serial-report and fatal-output functions using host stderr;
  it copies caller va_lists and does not allocate a Game-owned formatting buffer.
- Wii boot-arena setup and global allocation operators are not duplicated in
  this native Base provider. The explicit retained runtime owns the host arena;
  the companion routing provider implements ordinary/aligned/JKR new and delete.

`OriginalJkrHeapTests.cpp` exercises five groups against actual providers:
head/tail alignment and reset, native-width partial disposal with virtual
destructors, nested heap teardown/current-heap restoration, external-buffer child
lookup, and intrusive-list ownership transfer. `verify-native.py` passes these
under normal execution and ASan/UBSan. It records source and frozen-archive
hashes; complete runtime/provenance/concurrency tests are recorded separately.

Aurora's `os_report_tests` passes five checks: formatting/newline ownership,
repeated use of a caller va_list, long messages, panic termination/location, and
literal fatal messages. This Aurora change is committed as c595d62 and pushed.
No full-game success is implied by these CPU checks.
