# Original PlanetMapCreator data and archive queries

This checkpoint restores the missing original makeSubModelName method and
corrects the force-scenario sentinel in isDataForceLow and isScenarioForceLow.
The sentinel is the empty string at retail address 0x806B1C60; "Low" at
0x806B1C61 is the separate generated model-name suffix. The previous source
used "Low" for both purposes. Submodel names now follow the original flag
read, name read, exact-size allocation, concatenation and null-result branch.

The root edit is limited to src/Game/Map/PlanetMapCreator.cpp. All surrounding
constructor, data-table and archive-query algorithms remain complete original
methods. The existing getCreateFunc body and incomplete creator-pointer table
are outside this checkpoint and are not selected by its staged native provider.

## Original-code proof

verify-original.py compiles the root TU with the original GC 3.0a3 flags and
compares 11 complete data/query methods, 1760 retail bytes. Seven methods
(804 bytes) are relocation-aware byte exact, including the new 196-byte name
builder and corrected 108-byte empty-sentinel helper. All instructions in the
remaining four methods compare canonically after narrowly verified compiler
differences: the constructor's vtable temporary and independent pointer load /
vtable store ordering; two disjoint archive-query register-allocation ranges;
a register exchange and resolved string-pool offsets in addTableData; and an
omitted CR1 clear before the integer/string-only snprintf in isScenarioForceLow.
No branch, argument, allocation, field access or call is discarded by the
comparison. Raw fuzzy scores and complete canonical instructions are recorded
in compiler-evidence.json. The lowest score, 92.96875% for the unchanged
constructor, is explained by that small scheduling/register difference.

All 26 pointers/strings in the 13-entry unique-archive table and both static
archive/file-name pointers are verified against the retail DOL. The imported
original NameObjArchiveListCollector methods and isExistModel body are also
byte exact. The original archive-name CSV overload is verified separately;
its native holder lookup uses the existing resource-owner service described
below. imported-provider-evidence.json records those complete method proofs.

## Native provider and ownership boundary

native.patch adds OriginalPlanetMapData.cpp containing the complete original
constructor, table creation, lookup and archive-query methods, with the exact
unique-archive data. It imports the real PlanetMapCreator header, the original
NameObjArchiveListCollector body, and complete MR::isExistModel. That model
existence method formats the original /ObjectData/%s.arc path and queries the
actual DVD service. It does not infer existence from a native catalog.

OriginalArchiveCsvReader.cpp supplies the archive-name overload already called
by the original constructor. It replaces only the unavailable
ResourceHolderManager singleton lookup with the existing ResourceHolderService
owner; absent ownership fails explicitly. The returned actual ResourceHolder
is then passed to the unchanged original resource-holder CSV overload. The
original overload does not forward its optional variadic arguments, and this
provider preserves that behavior. No fabricated ResourceHolderManager or
additional resource registry is constructed.

The existing SceneObjHolder transaction creates and owns the real
PlanetMapCreator. Its actual pointer array, PlanetMapData rows, generated
names and temporary JMap parser allocate in the selected JKR scene cohort.
The transaction retires the registered NameObj first. ResourceHolderService
then retires its real resource holders; the original JMap disposer keeps the
shared decoded table and raw source alive until scene-heap disposal. There is
no second planet-data owner or catalog in the fixture.

## Actual-disc runtime evidence

verify-native.py (with SMGPC_REAL_DISC) runs the actual constructor through
SceneObjHolder::create and its existing ownership transaction. Both normal and
address/undefined-behavior-sanitized runs pass two construction/retirement
cycles. Each traverses all 258 authored rows, verifies all five submodel slots
and eight force-scenario fields, retains 135 generated model names, and checks
1155 disabled slots. The disc contains no enabled-but-missing model in this
table; that branch is covered by original instruction correspondence rather
than claimed runtime coverage. Ten rows contain a forced scenario and 248
start with the empty sentinel. One duplicate empty-name row verifies original
first-match lookup. The original WormEatenPlanet archive query returns eight
archives in exact order, including all six unique extra names.

A constrained 1024-byte scene cohort also exercises the real constructor's
failure path after creating its CSV parser. The original 0x818-byte allocation
fails, the scene slot remains unpublished, NameObj registration returns to
baseline, and original freeAll disposes the remaining parser. The one
allocFromHead diagnostic is expected. Both complete cycles verify that table
metadata survives resource-service retirement and expires with the actual
scene heap. No GPU or shared build is used.

native-manifest.json lists the seven staged source/test files; native.patch is
reviewable against the current native baseline. Parent owns source selection,
regular target integration and production application. The legacy
scene/nameobj/PlanetMapCatalog and supported NameObjFactory path are not
retired here. Full StageDataHolder initialization still requires complete
original factory/archive callbacks, and the CameraDirector/placed-collision
runtime remains gated on that graph. This is a verified original planet-data
constructor and query cohort, not full stage placement or Mario gameplay.
