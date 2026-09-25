# Complete compat removal — second owner checkpoint

The user's active priority is complete removal of src/compat. This batch removes another **28 files**, bringing the total to **73 of the original 330**, with **257 remaining**. Required behavior lives in actual Game or JSystem owners; deleted unused catalogs/facades have no successor service. This is a checkpoint, not completion of the removal or Gateway route.

## What changed

- **Gravity (six files):** restored the complete original GravityUtil method set, with consistent native-width identity comparisons. Removed two unused camera helper pairs. A regression verifies that different pointers sharing their low 32 bits do not exclude each other's gravity fields.
- **J3D (nine files):** restored six complete material, block, animation, table and texture-matrix owners. This restores missing light updates and actual virtual diffLight dispatch, while retaining native endian/FP handling. Real GD command-buffer checks cover sparse light slots and the original masks.
- **Resources/system utilities (nine files):** deleted the unused StageResourceBinding catalog. Tests now use actual original stage owners. Consolidated 38 utility definitions into Game/Util/MemoryUtil.cpp and SystemUtil.cpp, including original font/process/heap access. Removed the alternate scene-registry fallback from system scene operations.
- **Attribute groups and fixed positions (four files):** replaced the host string-set sidecar with original GroupChecker hash tables and placement-time sorting. Actual owning destructors release their child objects and table buffers with constructor rollback. Effect/Mario table cleanup delegates to HashSortTable. Deleted the unused FixedPosition parser; retained copyRotate in its actual Game owner and tested the original CSV readers and matrix behavior.

Focused LiveActorUtilGravity and LiveActorUtilGroup translation units hold their actual Game utility slices while the full LiveActorUtil.cpp still has other providers. No group/gravity replacement registry was added. Full consolidation of that utility remains pending.

Detailed correspondence, native adaptations and explicit remaining dependencies are in gravity/, j3d/, resources/ and groups/. coverage.json lists every removed and remaining initial path.

## Verified behavior

The combined native smg-pc build passed. The final binary SHA256 is `b3d83217bce60af07b97d8bee30aa4a313b9d3cb9a65758673eabbcb6d4ebc5a`. A fresh-save, real-disc **600-frame HeavensDoorGalaxy scenario 1** run exited zero, completed all frames and retired its process. Its frame-480 screenshot was visually inspected and shows Mario and the Luma in the wakeup scene. The executable hash was unchanged during the run. No scripted input was supplied. This does not prove reaching Rosalina or the Grand Star.

Passing focused checks include:

- Original hash/CP932 behavior, empty hash buckets, group hash collisions and duplicates, actual placement sorting and individual reclamation in an original ExpHeap; 16 original SolidHeap scene lifetimes.
- All four FixedPosition CSV/matrix tests; gravity query tests including full-width exclusion identity, and gravity math.
- Six J3D material-block groups, six material-table groups, four texture-matrix groups, four material-resource groups and five geometry-resource groups. Real Mario BDL material/geometry cases ran with the actual disc.
- Original camera-holder tests, including 42 Gateway and 111 EggStar archive chunks.
- Real original-process stage-resource test: **10 starts and 168 camera rows**, with archive identity checked against StageDataHolder and byte content against the independent archive reader. That stage has zero camera-usage rails; this run makes no positive rail-path claim.
- Real original-process NamePos test: all **7 authored positions**, exact links/transforms/heap ownership, first-match semantics and ordinary retirement. Both process resource tests completed 120 frames with fresh saves.
- NPC attribute-group fixture through `--groups-only`, using actual scene/controller and clipping owners, with no model/runtime bootstrap.

The actual archive provider scan reports **zero duplicate strong symbols**, zero stale mappings and no missing artifacts/owner inputs. Its broader provenance gate remains red for older source-anchor/token discrepancies (9 exact, 11 unresolved, 2 differing source checks); the gate was not weakened.

## Limits and initial failures

Logs retain initial failures. The first build attempt overlapped an in-progress source migration and encountered a removed source; the fully wired integrated build passed. Resource pointer tests initially compared pointers from different archive owners; corrected to use the actual original owner for pointer identity and independent bytes for content. A new group free-space assertion initially used SolidHeap, whose original individual free is deliberately a no-op; the corrected individual-free check uses original ExpHeap, with separate SolidHeap lifetime checks preserved. FixedPosition's old unsupported-construction fixture dereferenced a model-less actor through original APIs requiring a model; replaced that invented exception expectation with valid original CSV and supplied-matrix checks. Borrowed fixture names now outlive all NameObj references.

The complete older standalone test collection is not green. OriginalResourceHolderTests still calls an obsolete ResourceArchiveOwner constructor and does not compile; that pre-existing API migration is outside this batch. Full NPCActorRealOrAbsent reaches its unrelated RuntimeContext/model setup and fails because it lacks the original FileLoader; the passing group-only run is explicitly scoped. SceneSchedulerHeap now builds and binds actual original process owners, but its full run reaches an older sensor-retirement expectation that fails. The focused category diagnostic also fails its old exact trace-count assertion after the actual sensor checker participates. Neither scheduler diagnostic is represented as passing. These are not bypassed with production fallback state.

## Preservation

Changes are assembled with a separate temporary Git index. Six pre-existing dirty tests/build files receive only exact before/after patches. Two other earlier test rewrites had already removed the retired helpers; their committed variants get minimal HEAD-based removals instead, with the full working files unchanged. Their other edits and existing staged Gateway notes remain untouched. Current Aurora/decomp submodule work remains unchanged. Before snapshots stay local unless needed for the focused evidence. Raw NAND saves and renderer caches are excluded from publication.
