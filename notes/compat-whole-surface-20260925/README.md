# Remove src/compat — checkpoint, 2026-09-25

The active objective is complete deletion of `src/compat/`. Required native fixes belong at their actual Game owners; reusable Wii APIs belong in Aurora; JSystem/nw4r implementations belong in those libraries. This supersedes the earlier restriction against targeted native Game edits. This checkpoint removes **45 of the original 330 files**; **285 remain**. It does not complete the objective or prove the Gateway/Grand Star route.

## Inventory and actual changes

All 330 files have a concrete ownership/deletion disposition across the four `*-audit.json` reports. `coverage.json` verifies one entry per initial path with no omissions. These are migration reviews, not proof that every retained method is correct. The scene/game review explicitly distinguishes ownership triage from deeper algorithm validation.

- Restored canonical Scene, SimpleLayout, NameObj and NameObjFinder owners. Kept native registration, rollback and retirement directly in owning classes. Deleted unused SphereSelector code.
- Replaced the duplicate 66-manager AreaObj table and bespoke manager owner with original AreaObjContainer's complete **67-manager table**, including GlaringLightAreaMgr. The general scene construction transaction owns all managers. LightAreaHolder unregisters in its destructor. CubeCamera sorting now belongs to the original camera-load phase. Removed duplicate manager metadata from the remaining placement-creator catalog.
- Restored complete CameraTargetObj. Corrected CameraInnerCylinder's misnamed force-match helper to current upstream, removing a duplicate strong symbol.
- Restored complete original MathUtil, preserving native PPC arithmetic/ABI adaptations. Removed hidden random state: random draws and stage reseeding share GameSystemObjHolder::mRandom. Made native abs helpers self-contained, preserving negative-zero/NaN/INT_MIN semantics. Restored HashUtil with unsigned CP932 bytes and the original ASCII C-locale case folding.
- Restored 14 canonical JSystem source owners, plus nw4r LinkList. Ten SDK files are byte-identical donors; necessary native pointer width/destructor/diagnostic changes remain in four.
- Restored complete J3DPacket. The original allocation error now propagates, and its seven-entry register table includes the indirect-stage allocation budget. Existing display-list tests gained a regression for that missing budget.
- Deleted nine unused or duplicated Demo facade files and their two simulator-only tests. Actual DemoStartRequestHolder owns its records/proxy with exception rollback; real actor/request unlinking remains in the original demo owner path.
- Deleted five WPad/rumble providers. Actual WPad/Pointer/Holder destructors own child storage; holder callback scope retires before asynchronous write destinations. Speaker methods now live in their actual Game owners. The unused rumble adapter is gone. WPadOwnership's alternate pointer bootstrap remains explicitly pending.

`MercatorTransformCube.cpp` contains only the already-required unavailable method, now in its owning Game file. This is not a newly decompiled or implemented Mercator system.

## Validation

The final native `smg-pc` build passed. `final-builds.json`, `focused-builds.json`, `focused-runs.json` and `final-runs.json` retain initial failures and their actual outcomes rather than hiding them.

Passing focused checks include:

- Native math rotation/PPC boundaries and shared process RNG/reseeding, plus gravity math.
- Hash sorting and original CP932/case-fold behavior; original event sequences and pointer-width values.
- Five original J3D display-list/packet groups including indirect-stage budget.
- WPad repeated/nested owners, callbacks, read buffers and failure/retirement paths.
- Original typed demo queue/FIFO/capacity/record reuse and real holder ownership.
- Scene lifetime and original scene execution ownership.
- All 67 area managers, two scene generations, null/missing owner errors, exact camera-sort phase and reverse query, area movement, and unchanged Mercator unavailable behavior.
- Original camera holder's synthetic groups passed; its archive case was skipped without the disc variable.

The final executable (`af7e4a9a7997e6ec0c0dcae3a516c0fdf284d77c318940bfd47aade7120b3ce5`) completed a **fresh-save, real-disc 600-frame HeavensDoorGalaxy scenario 1 run**, exited zero, and retired the process. `final-owner-removal-600.json` records command, isolated save directory, binary hashes and completed-frame evidence. The frame-480 screenshot was visually inspected and shows Mario and the Luma in the wakeup scene. This does not prove reaching Rosalina or finishing the galaxy. Placement coverage still reports 187 supported, 51 known-unlinked, 0 unknown and 5 metadata rows.

The final actual archive scan reports **zero duplicate strong symbols**, unique source mappings, and no unavailable artifacts. The broader source-provenance gate remains red for older OriginalMapQueries anchor/token differences against merged upstream; it was not weakened. See `source-audit-final/provider-audit.json`. Initial duplicate CameraTarget force-match symbol was fixed using upstream's actual CameraInnerCylinder name.

## Remaining test and migration work

The entire AreaObj suite is not green: generic effect placement still lacks an original StageDataHolder fixture; disc-dependent area/light cases require the real disc and complete stage owners. OriginalCameraContext's standalone fixture also lacks the original GameSystem controller. Initial null-holder area crashes introduced by the migration were diagnosed and fixed; their focused checks now pass. Existing camera facade fixtures were already stale and now request real Demo owners, but need complete original process/resource fixtures for runtime validation. No fabricated test-only production state was added to bypass these dependencies.

Next removal batches should consolidate full J3D material owners, full CameraLocalUtil and canonical Game utilities; eliminate StageSession/StarPointer alternate bootstraps; move real lifetime responsibility out of sidecar registries into actual class destructors. General endian, allocation and SDK APIs must remain shared system support, not Game-specific policy hidden under another directory name.

## Preservation and publication

The original working tree included substantial unrelated staged/unstaged changes. `before-status.txt` and `before-index.patch` record that boundary. Only this work's changes are staged via a separate temporary index; shared dirty-file changes use recorded before/after patches. Four earlier dirty test rewrites already removed Demo dependencies, so the commit receives minimal HEAD-based removals instead of absorbing those full rewrites; the working files stay unchanged. This is documented under `demo-facade-removal/staged-test-adaptations/`.

Unfinished launch-driver/chip imports were shelved before this priority changed, under `pending-actor-imports/`; they are not selected by the production build. Existing Aurora and decomp dirty states are preserved. Raw NAND/save/cache files are not part of the publication.
