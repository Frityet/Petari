# Original scenario catalog and archive ownership

The staged implementation executes the original full-disc scenario preloader,
ScenarioDataParser constructor, GalaxyID ordering, and ScenarioData queries.
The isolated native fixture passes against the supplied RMGK01 disc. Native
production activation and actual GameSystem/scene publication remain the
parent integration task. No stage list, synthetic ScenarioData, or fake
GameSystem layout is supplied.

## Actual resource results

The original preloader discovers **48 Scenario archives**. The actual parser
retains **234 ZoneList rows** across those archives. HeavensDoorGalaxy contains
**2 ScenarioData rows, 2 visible scenarios, 2 nonzero PowerStarIds, and 7 zones**.
The fixture derives expected scenario/star counts independently from the
archive BCSV and checks original queries against them. Counts are observations,
not native restrictions or replacement metadata.

Two complete create/delete/heap-retire cycles pass. Additional coverage proves
mount/receive distinction, repeated mount identity and original heap provenance,
case-insensitive galaxy lookup, original table ordering, every authored zone
name, attached table survival after mount publication removal, explicit
NameObj unregister, metadata disposal at actual JKR heap retirement, and a
constrained-heap partial parser constructor failure. That failure deliberately
prints two original `allocFromHead` diagnostics before the fixture passes.

The first unguarded run exposed an actual native NameObj lifetime bug: the
process-static registry's bucket array was allocated inside the parser's Game
heap scope, then freed again during process-static teardown. The independent
`name-registry-host-allocation.patch` puts all registry insertion/name update
and allocated snapshot operations inside `JkrHostAllocationScope`. No registry
prewarming is used. Long-name update and snapshot storage survive retirement
of the caller Game heap in a focused regression. Nonallocating pointer-field
updates and erasure retain their existing behavior.

## Original construction and ownership

`GameSystemStationedArchiveLoader::startToLoadStationedArchiveOthers` calls
`StationedArchiveLoader::loadScenarioData` after its stationed table. That
unchanged method enumerates `/StageData`, tests each ordinary Scenario archive,
and calls `MR::mountArchive`. The actual parser independently enumerates that
directory, calls `receiveArchive`, constructs ScenarioData, attaches real
ScenarioData.bcsv/ZoneList.bcsv, and sorts using GalaxyNameSortTable.

The staged `ArchiveMountService` replaces the existing process-static mounted
archive map. RuntimeContext owns one service immediately after its DVD service,
so dependent actor/resource owners retire first, then archive identities, then
DVD backing. Headless fixtures install the same actual service with a real
DvdFileSystemService. The service retains actual JKRMemArchive wrappers, shared
RARC bytes, deferred bounded raw JMap registrations, and the requested heap
identity. `receive` never loads. Repeated mount retains the first wrapper/heap.
Explicit shared mount leases retain wrappers/registrations past unpublication.

The existing native FileUtil provider moves from Game/Util/FileUtil.cpp to
compat/FileUtilCompat.cpp as one coherent replacement. Archive-resource copies
now belong to the explicit service and use `(archive identity, resource path)`
keys. The former unavailable GameEventFlagTable macro provider is removed when
the actual Game table TU and embedded resource are installed.

ResourceHolder `.banmt` registration uses the same deferred raw-source identity
as mounted archives. Previously it constructed a second eager JMapResource
owner for the same RARC byte address, conflicting with the mount registration.
Original ResourceHolder still constructs and attaches its real JMapInfo; there
is no substitute holder/table or change to Game resource classification.

## Exact embedded resource and source proofs

RMGK01 DOL SHA1: `25c5959534b3c21246c6c7e42021b916b41fb578`.
`GalaxyIDBCSV`: address `0x8053DE00`, 0xD20 bytes, SHA256
`44b3ace292ed8f9f7c60f0ddd49a89cdd0648c67613b5974e85658bfd18a2d43`.
Its original BCSV header is 55 rows, 7 fields, data offset 0x64, row stride24.
Empty spacer-name rows are preserved: the first is index5, HeavensDoor is6,
EggStar is8. Fifty-five is not the scenario archive count.

`verify-embedded-data.py` stages the exact opaque `const u8[]` object and corrects
three extern declarations (GalaxyNameSortTable, GameEventFlagTable,
GameDataConst.hpp). GC3.0a3 emits the exact retail bytes. All four consumers
have identical before/after instructions and relocations: 1,072 bytes total.
The native process GameResourceRuntime registers the actual static symbol's
full bounded bytes through retained EmbeddedGameTables/JMapResource ownership.

Three complete original StringUtil methods close the actual link frontier.
`verify-archive-strings.py` verifies `removeExtensionString` (124 bytes) and
`copyString` (4 bytes) at **100%**. The former uses `uintptr_t` address comparisons
so its original unsigned pointer/null-plus-one arithmetic is defined natively;
Wii instructions are identical. Its `%s` SDA relocation is checked against the
actual bytes at `0x806B26B0`, rather than ignoring data references.

Root `getBasename` incorrectly returned nullptr for a slashless filename.
Retail retains and returns the original input. The minimal source correction
compiles to the original 68-byte size, with **87.23529% objdiff** because the
compiler chooses another result register and epilogue ordering. The script
executes both actual retail and rebuilt PPC instruction bodies for 387
input/last-slash cases, checking return value, external strrchr arguments,
stack restoration, and LR restoration. Both return input when no slash exists,
or last-slash+1 otherwise. No artificial branch or volatile annotation is added
to increase the score. The native extraction has the corrected complete body.

The original extension-removal routine assumes an extension in slashless input;
its retail missing-extension dereference is not replaced with a new fallback.
Authored Scenario archive paths satisfy that original contract.

## Package and validation

Root patches: `embedded-data-root.patch`, `archive-string-root.patch`.
Native activation: `native.patch`, with exact file list/hashes in
`native-manifest.json`. The independent NameObj guard patch is excluded from
that native package so the parent can activate it first. No xmake edits are
included; a regular game-linked test target named
`smg-pc-original-scenario-catalog-tests` should use
`tests/OriginalScenarioCatalogTests.cpp`.

Ignored staged files, native compiler command records, exact isolated link
command, and binary are in `build/scenario-catalog-owner-20260903`. The recorded
native source build covers all staged TUs, including RuntimeContext. The
fixture is linked against the frozen native Game/Aurora archives with these
explicit complete provider objects taking precedence; unused full-Game system
entrypoints are dead-stripped. This is a linked headless actual-catalog result,
not a full GameSystem or rendered scene result.

Commands:

```
python3 pc-port/notes/scenario-catalog-owner-20260903/verify-embedded-data.py
python3 pc-port/notes/scenario-catalog-owner-20260903/verify-archive-strings.py
SMGPC_REAL_DISC='<actual RMGK01 RVZ path>' build/scenario-catalog-owner-20260903/scenario-catalog-tests
```

`catalog-test.log` contains the complete compact passing output. Build command
and source correspondence evidence is recorded in `native-evidence.json`.
No shared xmake build, GPU session, production source activation, commit, or
push was performed by this agent for this package.

## Precise remaining integration boundary

* ScenarioDataFunction::getScenarioDataParser still contains its unchanged
  actual GameSystem/SceneController lookup. Native service classes are different
  types. The parent must supply actual parser publication through its agreed
  native owner boundary, or the actual complete original owners; never cast a
  service into Game layout. The fixture directly owns the actual parser and
  does not invoke that unavailable global owner path.
* The parser's original allocation storage has 64 slots; the fixture validates
  actual archive count before construction. The future scene owner must validate
  resource counts before original fixed-capacity storage is entered. Collision
  keeper initialization must use the complete current authored ZoneList and
  respect its original 32-zone capacity, not a placement-derived count.
* Mount heap pointers preserve provenance; they do not independently retain a
  JKR allocation domain or automatically observe arbitrary heap destruction.
  Existing original explicit remove-before-heap-retire calls/context ownership
  must be respected. Any future owner bypassing those calls needs a proper
  general heap-lifecycle boundary, not a fabricated wrapper destructor.
* `removeResourceAndFileHolderIfIsEqualHeap` now removes the service's matching
  mount publication. Full original FileLoader/resource-holder/raw-file removal
  is not claimed: the prior native raw-file cache remains outside this bounded
  archive migration, and explicit archive resource copies live with the service.
* Whole StationedArchiveLoader import remains blocked by the existing native
  JKRDvdRipper enum/class mismatch (native TOP0/BOTTOM1 versus original
  UNKNOWN0/FORWARD1/BACKWARD2). Only the complete, unrelated-closure-free original
  loadScenarioData method is extracted. FORWARD is not aliased to a wrong value.
