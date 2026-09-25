# Round 7: actual FileLoader archive ownership

Baseline: `0f8bc440af14cbc62ebaf26f221124eae447ec03`. Owned source/test paths were clean when captured; `manifest.json`, `before/`, and `before-status.txt` preserve the exact initial state. No build, staging, or commit performed by this lane.

## Canonical owners

- Enable the complete existing donor `Game/Util/FileUtil.cpp` and delete `compat/FileUtilCompat.cpp`. The donor already implements all original language/path, load, mount, query, and decompression logic. The only new body adaptation is atomic native retirement preflight before its original resource-manager/file-loader sequence.
- Delete `runtime/ArchiveMountService.cpp/.hpp`. `FileLoader::mArchiveHolder` is the sole actual mount owner; root migrated runtime/wrapper consumers.
- `ArchiveHolderArchiveEntry` owns its original `JKRMemArchive`, name allocation, and bounded typed-source registrations. Its native state retains the actual allocation domain. Entry removal keeps that domain alive through `delete`, including array-name and archive cleanup.
- Keep original `FileHolder` byte ownership and file-before-archive teardown. Archive cleanup removes metadata/registration identities without reading the retired raw buffer. No raw buffer copy or substitute mount registry was added.

## SDK and typed attachment contract

`JKRArchive::source()` and `retainSource()` expose bounded RARC metadata. Fixed mounts still borrow the original bytes, so metadata alone is **not** a raw lifetime token. `retainNativeResources()` explicitly borrows an archive; `validateNativeRetirement(releasingBorrows)` rejects remaining borrowers. The optional count only discounts exact references that a preceding validated holder deletion will release.

`JKRArchive::overrideNativeResource(index, replacement)` returns a handle owning one archive token. Each actual file entry has a chain of live overrides; any retirement order restores the newest surviving replacement or original cache pointer. A separately changed cache is not overwritten by retiring an unrelated replacement. No Game type decoding occurs inside JKR.

JMapInfo preserves original raw identities in `getData()` and `getEntryData()`. Accordingly, `register_jmap_source` now accepts an optional attachment-lease callback. ArchiveHolder supplies a weak archive-token callback; each attached reader acquires a distinct token before decoding. Registrations themselves acquire no token, avoiding self-borrowing. Attached readers must retire before the original archive/file/heap can retire. Copying JMapInfo shares the attachment lifetime. JPC decoding already copies complete raw/typed data and its resolved resource needs no original fixed buffer.

`FileUtil` first validates all selected ResourceHolder/LayoutHolder borrowers, then validates each selected archive with exactly `ResourceHolderManager::countNativeArchiveReferences` expected releases. Both scans complete before either owner changes entries. Direct FileLoader retirement validates all selected archive borrowers too. Removing only a file while its archive is mounted rejects explicitly instead of invalidating the live fixed mount.

## Tests and wiring

- `OriginalScenarioCatalogTests.cpp` now uses the actual original process, FileLoader, and scene-controller ScenarioDataParser. It independently enumerates DVD StageData, compares authored GalaxyID ordering/zones/scenario counts, verifies repeat preload/mount identity and heap metadata, and retains meaningful native name/failed-constructor tests on actual child heaps.
- `ScenarioPublicationTests.cpp` runs two complete original processes. It verifies actual parser publication, real mount identity, explicit archive-borrow retirement rejection, and normal singleton/JMap/archive/NameObj cleanup each time.
- Both targets need `smg-pc-app` and `aurora-main` dependencies. Root owns test wiring and execution.
- This lane's only `src/Game/xmake.lua` edit removes `remove_files("Util/FileUtil.cpp")`.
- Root/merge_audit own focused ResourceHolder cache override, attached CSV lifetime, and integration regressions. They were informed of the exact contracts above.
- Follow-up assigned by root: `OriginalJkrArchiveTests` covers all six retirement permutations of three native cache overrides; exact token counts despite copied handles; path/index/ID/size identity; independent cache mutations; invalid entry/folder/null replacements.
- `OriginalJMapResourceTests` covers actual fixed JKR archive attachment leases; shared copies versus independent attachment counts; raw/row identity; unpublication while readers are live; rebind/final release; expired callbacks and attempts to silently change a duplicate registration's lease policy. The generic registration now rejects changed policy or an empty required lease before decoding.

Independent preflight review confirms ResourceHolder reports its base archive token plus each override handle, LayoutHolder reports one, and the manager sums only exact archive identity entries selected by the same exact/Napa/GDDR3 heap predicates as removal. The combined FileUtil scan subtracts those exact validated releases while independent JMap/raw borrowers still reject. No registrations themselves contribute archive tokens.

`git diff --check` passes on this lane. No compilation or runtime result is claimed yet. Root coordinates the integrated build and real Gateway checks.

## Teardown diagnostic follow-up

Root reports the integrated process reaches 600 frames but still rejects archive teardown after initial Scenario/Particle/Stage cleanup. `JKRArchive::validateNativeRetirement` now reports authored archive root name, live borrower count, and exact expected holder releases. Formatting uses HostAllocationScope. No lifetime rule is relaxed. Reviewed DeferredJMap: cached decoded JMapInfo owns BcsvTable only; per-attachment owner references DeferredJMap and archive token, while the deferred callback holds only a weak token. There is no cache-to-attachment cycle. Root rebuild/runtime investigation remains pending.
