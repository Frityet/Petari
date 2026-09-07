# Native exception ownership, 2026-09-07

The Gateway error `Camera state is unavailable.` exposed an exception lifetime
bug during scene unwinding. A native standard exception's message was allocated
in the selected Game arena; the exception escaped that arena's lifetime.

## Allocation and destruction proof

`JkrExceptionProbe.cpp` intentionally reproduces the unfixed bug with the actual
JKR root/domain/global-new bridge. Its temporary build target was removed after
the proof; the source and debugger transcripts remain as evidence.

`probe-allocation-backtrace.log` records the 53-byte allocation at
`0xb50000210`: `std::logic_error(char const*)` -> native `operator new` ->
`allocate_jkr_or_host` -> original `JKRHeap::alloc` ->
`record_jkr_allocation`, with child heap `0xb50000120` selected. The scene domain
is destroyed before the outer catch.

`probe-delete-backtrace.log` records the message printed in that outer catch,
then the matching failure class: libc++abi's `std::logic_error` destructor calls
native free on retired scene storage, stopping at `malloc_error_break`.
The full showcase reported the provenance guard for its analogous escaped
message; this smaller binary reaches libc++abi's direct free instead. The
provenance guard is unchanged.

## Shared boundary and actual camera callers

`aurora/exception.hpp` adds `aurora::throw_host_exception<Exception>(string_view)`.
It selects host allocation only while copying the message and constructing the
exception. Unwinding restores both routing flags. The selected original heap
remains observable. The exact nine standard message-bearing exception classes
are supported; custom exception construction and global throwing semantics are
unchanged. Accepting message text avoids adopting a preexisting exception's
potentially guest-owned shared message storage.

The first integration changes all 17 explicit standard-error producers in
`CameraUtilCompat.cpp` and `CameraLocalUtilRuntime.cpp`. Argument text and types
are preserved exactly; the only edits are the throwing prefix and shared header
include. A temporary concatenated caller string may occupy the Game arena; it
is destroyed during unwinding while that arena is still alive. The retained
exception message is copied under host allocation.

## Validation

`xmake build -y smg-pc-jkr-exception-ownership-tests` and the resulting native
binary both exit 0 (`ownership-build.log`, `ownership-run.log`). Tests verify:

- All nine exact standard exception types and an unterminated message view.
- No Game allocation for exception construction; exact Guest/Host/Client and
  nested-domain routing restoration; subsequent original new uses that domain.
- A 4 KiB Game-owned message plus concatenated temporary, copied exceptions,
  and an exception_ptr rethrown after complete scene retirement and replacement
  arena allocation/overwrite. The exception does not retain the original domain.
- The actual `MR::getCamPos` error, caught and destroyed after its Game domain
  expires, with its original type and message unchanged.

`results.json` includes exact tested paths and reversible-prefix proof for the
camera migration. This CPU test does not claim Gateway gameplay completion.

## Reviewed broader migration

`migrate_native_exceptions.py` tokenizes comments, strings (including raw
strings), characters and digit separators. It balances (), [] and {} before
recording explicit constructors of the nine standard classes. Each proposed
edit records the entire original argument, its hash, exact prefix, source hash
and line. Applying a cohort requires explicit paths and a reviewed manifest;
all files are checked for drift before any is written. Custom exceptions,
bad_alloc, original Game and imported SDK sources are excluded.

`native-migration-manifest.json` captures 1,510 sites in 168 native files before
the initial camera migration. Counts by type: runtime_error 630, logic_error
526, invalid_argument 289, out_of_range 31, length_error 22, overflow_error 12.
Only five arguments start with a variable/function instead of a literal,
std::string or formatter; all are message strings, not copied exceptions.
No replacement prefix contains comments. Root common library now exports the
Aurora public-header include path, which is sufficient for this header-only API.

`aurora-migration-manifest.json` separately inventories 68 sites in six Aurora
files (audio/stream/archive/SYSCONF, BRLAN parser and Aurora's native JAISound
handle validation). Both reviewed cohorts have now been applied; exact applied
manifests are `native-migration-applied.json` and `aurora-migration-applied.json`.
`reversible-migration-proof.json` verifies all 174 files/1,578 edits: reversing
only the include and throwing prefix reproduces every original source hash.

All 174 affected files pass isolated LLVM 23 syntax checks: 170 complete .cpp
translation units and four headers. `syntax-results.json` records commands,
source hashes and individual logs. The inactive ScreenshotService source and
four headers use flags from their owning libraries, disclosed in each result;
the other 169 use their own actual compile-database command. This catches
throw-expression/statement typing changes as well as message-argument types.
The lexical edge-case tests pass 3/3 (`migration-lexer-test.log`).

`remaining-native-throw-tokens.json` records the remaining throw tokens:
message-free bad_alloc, rethrows, the custom DemoSheetParseError, and the
preexisting J3dMaterialTableData templated fail helper (already Host-scoped).
Library-internal and third-party error constructors require their own ownership
audit; this work does not intercept those constructors.

The reusable Aurora helper was separately committed and pushed as codex in
`40b3e98`; the six producer files were committed and pushed in `9e11f4a`.

## Custom parser and final integration

The exact custom `DemoSheetParseError` throw now has one narrow
HostAllocationScope immediately before construction; its type, constructor and
message expression are unchanged. Existing malformed Time/Camera parser tests
now execute in actual Game domains and assert that the outer catches see fully
reclaimed arenas and host-owned exception messages. The existing DemoSheet
target builds/runs 0: 11 cases pass, including both actual custom parser-error
paths. Its two optional extracted-disc checks explicitly skip without fixtures.

The expanded ownership target builds/runs 0 again after the broad adoption,
including actual invalid BRLAN, SYSCONF and JAISoundHandle API calls caught after
complete scene retirement (`ownership-broad-build.log`, `ownership-broad-run.log`).
`smg-pc-app` and the full `smg-pc` main binary build successfully. The showcase
compiles successfully, then exposes 46 original owner/provider link gaps from
the newly restored complete Mario constructor graph. These are recorded with
origin references in `showcase-undefined.txt`/`.json`; none originate from the
exception change. Parent is closing that separate source graph. This checkpoint
does not claim a successful new Gateway runtime or playable Mario movement.
