# Original stage-table addresses and memory-archive indexes

## Regular native integration checkpoint

The raw-source reader, original archive index/fetch methods, BothDirList and
shape-ID query prerequisites are now selected in the production native build.
The two frozen patches were applied with `--exclude='pc-port/src/Game/Scene/*'`;
complete StageDataHolder and PlacementInfoOrdered remain staged until the
original factory graph is ready. This prevents an incomplete scene constructor
from being selected by the build's Game source glob.

After rebuilding all dependent translation units for the new JMapInfo and
JKRArchive layouts, these regular LLVM 23 macOS ARM64 tests pass:

- `smg-pc-original-jmap-resource-tests`: all 13 groups, including exact raw
  identity, original heap retirement and source lifetimes.
- `smg-pc-original-resource-holder-tests` with `SMGPC_REAL_DISC`: all five
  groups, including the actual model/material holder and retained parser
  cleanup. The resource-only fixture discards unused code at link time;
  otherwise unused gameplay facades pull in incomplete Mario methods. No
  reachable resource provider was replaced.
- `smg-pc-original-scenario-catalog-tests` with `SMGPC_REAL_DISC`: two complete
  48-archive / 234-zone catalogs, authored Gateway queries and original
  allocation-failure cleanup. The allocation diagnostic is the intentional
  failure fixture; it passes and restores ownership.

`regular-test.log`, `holder-test.log` and `catalog-test.log` retain output.
The ordinary command is `xmake build -j8 <target>` followed by its executable
under `build/macosx/arm64/debug`; disc-backed tests use the actual RMGK01 RVZ.
This checkpoint supplies resource identity/lifetime, not a complete stage or
camera constructor runtime.

StageDataHolder compares the actual raw BCSV row address against the address
range of each placed zone's tables. The native JMap reader previously exposed
only a shared decoded count/cache object, which cannot supply that identity.
The staged reader now retains the exact pointer passed to attach separately
from the shared decoded fields and strings. Two raw aliases of one decoded
resource compare as different original tables; copies retain the same raw
identity. Direct from_bcsv construction owns its complete copied byte image.

Three small inline root JMapInfo accessors expose the existing raw pointer,
row address, and original header-plus-row extent. StageDataHolder uses those
accessors and uintptr_t address bounds. The root PowerPC build is unchanged:
all four affected methods are 100% instruction matches, and all 652 bytes
are identical to the DOL after independently verified relocations. The count
accessor substitution also preserves both original count loads. The native
row-address calculation preserves the original 32-bit row-product arithmetic
while retaining full-width host pointers. String bytes are intentionally not
part of the original row-range extent.

## Archive lookup and retention

Original StageDataHolder::initJmpInfo requests a file-table index. It must not
be interpreted as a file ID. The staged SDK addition imports complete original
JKRArchive::getIdxResource and JKRMemArchive::fetchResource methods. Both
compile to exactly the original 140 instruction bytes. Native JKRMemArchive
retains its actual file-data base from the validated RARC header and uses the
existing decoded original file records, including the original cached pointer
and optional size output. No new archive or resource authority is introduced.

A JMap attachment now retains its source registration's actual byte owner,
as well as the shared decoded reader/cache. Removing a mount or registration
unpublishes lookup immediately; existing attached readers keep their raw
addresses valid until the final reader is disposed. Archive registrations hold
RarcArchive bytes, not JKR allocation domains. The reader's original
JKRDisposer therefore releases its native owners before freeAll reuses the
arena, without a resource-to-heap retention cycle. Borrowed alias registration
still requires its caller to keep the borrowed bytes alive through reader use.

## Evidence and staged application

verify-original.py records the six complete original methods, exact DOL
relocations and source hashes in compiler-evidence.json (792 bytes total).
verify-native.py runs a bounded CPU fixture with the real JKR heaps and SDK
archive metadata; --sanitize instruments all changed parser/archive providers
and the fixture with address and undefined-behavior sanitizers. Both runs pass
13 groups. They cover raw aliases with shared caches, exact row pointers and
extent, copy/move identity, owned input bytes, source retention after
unpublication, Game-heap disposer release and arena reuse, index versus file
ID, memory-fetch caching and size, concurrent readers, and existing alias and
malformed-input checks. The sanitizer run reports no diagnostics.

native.patch contains 13 files. Apply the frozen
original-placement-info-ordered-20260903/native.patch first, then this patch.
The merged JMapUtil header preserves its shape-ID declaration and adds the
complete original checkJMapDataEntries inline body. Ordered patch application
was tested on isolated copies and reproduces every native-manifest.json hash.
The JMapInfo and JKRArchive layouts change; all dependent translation units
must be rebuilt. The patch updates the regular JMap-resource test and the
ResourceHolder test's source-lifetime expectations. The latter full fixture
has not been rerun in this isolated package; its existing parser must now
release the final raw archive lease explicitly.

Full StageDataHolder and PlacementInfoOrdered compile against the staged
headers (game-compile.json), with existing root declarations as fallback for
not-yet-imported Game dependencies. Their sources remain complete root
bodies. The original general source set and factory graph are still required
before selection in the production native target. This checkpoint performs
no native-production edits, shared build, GPU work, or scene activation.

Root checkpoint files are include/Game/Util/JMapInfo.hpp,
include/Game/Scene/StageDataHolder.hpp, and src/Game/Scene/StageDataHolder.cpp.
root.patch and root-manifest.json freeze only those changes. Binary artifacts
and synthetic archive fixture build products remain under ignored build/.

## Remaining scene/camera closure

Raw address identity and PlacementInfoOrdered are ready for the real stage
constructor fixture. The fixture still needs actual mounted stage/JMP archives,
original NameObjFactory and stage-selection queries, and the complete retained
StageDataHolder graph before CameraRailHolder constructs. The scene transaction
must own and retire actual registered StageDataHolder descendants while JKR
heaps and archive leases remain valid. Legacy camera and collision facades
remain gated for atomic replacement; these tests do not claim a running
CameraDirector, placed collision scene, Mario movement, or jumping.
