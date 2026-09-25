# Round17 canonical NameObj and planet factories

Baseline `6711b79ad` (full hash in manifest). Root centrally imported missing donor headers/sources first. This lane captured the resulting files before further edits; original untracked import status is recorded. Twenty-two paths are covered by before/after snapshots and `factory-only.patch`, including retained root imports whose content needed no changes.

## Actual Game providers

- `Game/NameObj/NameObjFactory.cpp` now exactly matches current decomp donor bytes. The previously excluded local copy had an added nerve definition/keepalive block; those factory-local additions were removed. Every original creator row, alias, area form, archive row, dynamic callback and row order is retained. No native creator availability allowlist or substitute actor was added.
- `ModelChangableObjFactory.cpp` uses the full root-imported donor unchanged, including its duplicate TripodBossRotateParts row. `PlanetMapWithoutHighModel.cpp/.hpp` use root's existing donor import, with its narrow CP932 literal conversion already present.
- `PlanetMapCreator.cpp` retains all original tables and selection methods, including exact stage/scenario force-low matching and unique planet creators. Its actual owner now retains the JMapInfo parser, releases generated submodel names/rows/row array/parser on destruction, and unwinds partially constructed rows safely. Rejected generated model names are freed before the original null assignment. No copied catalog, substitute strings or separate active publication remains.
- Synchronized the real PlanetMapCreator header to current donor, including PlanetMapFarClippable, then added the actual parser ownership/destructor members. Two targeted PlanetMap header declarations now match donor: the default second model argument and inline PlanetMapAnimLow(const char*) constructor. Existing native PlanetMap implementation/teardown is preserved.

Deleted the eight scene providers: nameobj/NameObjFactory pair, NameObjArchiveTable.inc, OriginalNameObjNames.inc, PlanetMapCatalog pair and AreaObjRuntime pair. Removed OriginalSceneSupport's planet catalog include/construction/member/reset. GameScene's existing original PlanetMapCreator creation supplies the process lifetime.

Root integration: remove Game/xmake's `NameObj/NameObjFactory.cpp` exclusion. Root owns removal of SceneObjAvailability and all native factory gates, so this lane made no availability-header edits. Game's recursive source selection already includes the three newly imported implementation files. No xmake files or source stubs were added here.

## Existing fixture cleanup

- Retired `PlanetMapCatalogTests.cpp`; root removes target `smg-pc-planet-map-catalog-tests`.
- NameObjFactoryPlacementTests retains its actual original-process wall creator/lifecycle case; the three standalone native-factory support-policy cases and their unused helper code were removed.
- AreaObjRealOrAbsentTests retains real manager/form/query checks. Removed descriptor-registry/preflight assertions; generic-effect cases directly construct their actual AreaObj/form rather than consulting the deleted descriptor table.
- SphereSelectorRealOrAbsentTests retires the manual factory blocking/audio-policy case; its independent existing owner/layout/camera cases remain.
- GravityRealOrAbsentTests uses the actual NameObjFactory creators with unique_ptr under its existing original-process fixture; all gravity initialization, rail, follower, query and retirement assertions remain.
- AuroraNativeTests keeps its authored DemoRabbit archive-selection assertions by calling original DemoRabbit::makeArchiveList directly. Removed standalone synthetic creator-availability and StarPieceGroup-absence assertions. Other test groups are untouched.

All fixture paths were clean when snapshotted. No test framework or new cases were added.

## Validation boundary

No builds, tests or runs were performed. Source comparison confirmed the two canonical NameObj factory implementation files equal donor; scoped whitespace checks passed. Source/test text search found no surviving include/call of the deleted scene factory/catalog/area APIs (the PlanetMapCatalog test target awaits root removal). Native Game compilation/link gaps from the broad import remain root integration work; this checkpoint does not claim those actor families execute successfully. Root owns the integrated build and bounded smoke.
