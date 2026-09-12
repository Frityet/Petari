# Original stage data and placement ordering recovery

This checkpoint restores the complete StageDataHolder and PlacementInfoOrdered source owners. The reference compiles with the original GC3.0a3 compiler; both native translation units compile in isolated ARM64 objects. It does not establish that the real scene factory can construct or run these owners yet. The parent independently recompiled both native translation units against an export of the published parent tree plus only these seven native paths, and rebuilt all three reference translation units with the original compiler. No native link/run or scene factory activation is claimed.

The exact frozen files are in `reference-publish-list.txt` (five reference files) and `native-publish-list.txt` (seven native files). `source-proof.json` records all source/object hashes, commands, successful compiler exits, and every compared method. The full objdiff files remain beside it. All changes follow `decomp/AGENT_DECOMP_GUIDE.md`; no assembly or host fallback was added.

## Reference recovery and correctness

Local retail inputs are `notes/gateway-audit-20260907/restoration/retail/{asm,obj}/Game/Scene/StageDataHolder.{s,o}` and `PlacementInfoOrdered.{s,o}`. The recovered code uses the actual original table/heap/scene APIs.

| StageDataHolder recovery | Retail address | Match |
| --- | --- | --- |
| Constructor | 80346FC0 | 100% |
| getChildObjNum | 80347850 | 95.882355% |
| getChildObjInfoFromDataIndex | 8034791C | 96.55738% |
| initLayerJmpInfo | 80348100 | 99.75% |
| initPlacementInfoOrderedScenario | 803485AC | 100% |
| createLocalStageDataHolder | 8034893C | 99.875% |

The constructor receives the actual archive, retains the stage-name pointer, initializes the original arrays and counters, and gives its placement matrix the original identity. Layer loading retains the exact 17-bit order and interleaves Placement then MapParts for each enabled layer. Child-object lookup uses the exact `ParentID` key and preserves the original parent-link comparison and result order. Each StageObjInfo row creates its own original child holder; common/scenario ownership remains separate.

The existing common placement builder contained a semantic mistake: its second child loop incremented its index both in the loop body and the for header. Retail 80348570 increments the logical index once, while 80348574 advances the pointer by four. Removing the extra increment restores every child and matches that entire method exactly.

PlacementInfoOrdered is now complete. Its full retail text matches **99.36246%**, with all methods between 95.608696% and 100%. The sorting algorithm, constructor, identifier grouping, insertion, SameIdSet construction, and attachment methods match exactly. Requests and placement retain the original creator-availability checks, shape-model handling, Japanese-name lookup, and pre/post `initLiveActorSystemInfo` calls. Group sorting preserves player-loader priority, DVD priority, descending same-identifier group size, and the original gap sequence/equal-key behavior.

The earlier partial PlacementInfoOrdered constructor also cleared only `count` bytes of its pointer array. Retail 80342720–80342734 clears `count * 4`; it now uses `sizeof(*mIdentiferArray) * count`, which is exact on PPC and correct at native pointer width. The header's former oversized Identifier was not the actual owner: Identifier has name/model-number fields; SameIdSet derives from it and owns priority plus the index list; Index owns the original JMapInfoIter; the ordered array points to SameIdSet objects. Those types now describe the retail accesses directly.

The trivial BothDirPtrList(bool) constructor body moved unchanged from the reference cpp into its header, allowing original SameIdSet construction to inline its `initiate()` call. Reference StageDataHolder, PlacementInfoOrdered, and BothDirList all compile after that move and have no undefined BothDirPtrList constructor symbol. Native BothDirList already had that exact inline form and was left unchanged. This is an inlining/declaration correction, not new list behavior; the original out-of-line constructor is no longer emitted in the reference BothDirList object.

Full StageDataHolder retail text matches **95.275795%**. Two pre-existing methods remain below the recovery threshold: getJapaneseObjectName at 0.7631579% due to the compiler inlining its string findElement loop, and the two-directory initAllLayerJmpInfo overload at 82.548386% with existing expression/evaluation shaping. Their source was not changed in this recovery. The high aggregate score must not be presented as a claim that those individual functions have high instruction matches.

## Native architecture boundary

PlacementInfoOrdered cpp/header are byte-identical to the recovered reference. The new StageResourceLoader, ModelChangableObjFactory, and PlanetMapCreator headers are complete copies of existing reference declarations; importing declarations does not provide their runtime owners.

The only native StageDataHolder differences are recorded in the `.native-diff.patch` files:

- `_E4` and `_E8` hold full native addresses as `std::uintptr_t`.
- Row identity comes from the retained original byte address `JMapInfo::getEntryData(index)`.
- Bounds use `JMapInfo::getData()` plus `getDataSize()`, preserving the actual source archive interval rather than the host DataCompat allocation address.
- Two local count-descriptor declarations use the native shared descriptor type, retaining the original count/null behavior.

The address comparison is meaningful for original JMapInfo attachments into each actual contiguous archive. Replacing it with copied zone IDs, pointing it at separately decoded DataCompat objects, or inventing a StageDataHolder around a catalog would lose original ownership semantics. Synthetic per-table vectors must not stand in for that archive layout.

## Remaining activation dependencies

`native-provider-audit.json` compares the two newly compiled objects against the existing shared Game archive plus each other. It is an object-provider inventory, not an executable link receipt. Six direct Game calls remain absent:

1. StageResourceLoader: `tryRequestLoadStageResource()` and `isLoadStageScenarioResource()`.
2. PlanetMapCreatorFunction: `isLoadArchiveAfterScenarioSelected(const char*)`.
3. ModelChangableObjFactory: `getModelChangableObjCreator`, `requestMountModelChangableObjArchives`, and `isReadResourceFromDVDAtModelChangableObj`.

The actual SceneUtil/StageFileLoader and JMapUtil dependencies remain in the wider call graph as well. Whole-source StageResourceLoader and StageFileLoader imports are the next small candidates; their original source should supply their methods together.

The archive registration boundary is also unfinished. Real ArchiveHolderArchiveEntry mounts a fixed JKRMemArchive directly; the old ArchiveMountService's eager JMap registrations do not cover that route. Original StageDataHolder attaches archive entry bytes through JMapInfo::attach, which correctly requires a bounded retained source. The process/resource lane owns moving typed preparation into the actual SDK archive fetch/entry lifetime; see `notes/original-file-loader-20260912/RESOURCE_MIGRATION.md`. Do not add StageDataHolder-specific registrations or a fallback to the old mount service. Fixed archive borrowers must retire in the actual Game order before FileHolder releases the underlying heap bytes; typed-owner metadata must not create an allocation-domain cycle.

Factory activation must also use the actual GameSystem scene controller/scenario owner. The original SceneObj_StageDataHolder case creates `StageDataHolder(MR::getCurrentStageName(), 0, true)` and calls normal initialization. No fabricated process, alternate readiness state, or manual catalog-populated holder has been installed. Positive table/placement runtime validation remains a later step once those real owners and lower source cohorts are available.

## Publication validation

Reference commit `df1d7db9539834311722be1bb9b88a54c5f62195` was pushed to `pcp-decomp` and verified against the remote branch. `head-only-compile-audit.json` and `fresh-reference-compile.json` record the independent compiler checks. `validation-evidence.tar.gz` contains the commands, compiler logs, provider inventory, native differences, and exact publication lists; it contains no game assets or object files.
