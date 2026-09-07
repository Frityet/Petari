# Original collision ownership — 2026-09-07

This cohort replaces the native snapshot-only actor registration with actual original `CollisionParts`, `KCollisionServer`, `CollisionDirector`, four categorized keepers, zones, and collision codes. It preserves the original pending/current/inverse/previous matrix policy and zone membership calls. No Mario, planet, Gateway, or actor-type branch was added.

## Source and ownership

`root-paths.json` lists the exact native cohort, including the two independent query helpers and their tests. `decomp-paths.json` lists the only new reference change for this cohort: `MR::isExistMapCollisionExceptActor` in MapUtil.cpp. The other Game imports already exist in the reference. Five whole native Game translation units are byte-identical imports; `original-import-provenance.json` records them.

- `OriginalCollisionPartsCompat.cpp` imports all completed original CollisionParts bodies. Native publication is appended only after original zone add/remove and after committed/reset matrices. `src/Game/Map/CollisionParts.cpp` stays excluded to keep one symbol owner.
- `OriginalCollisionPartsUtil.cpp` retains the original MR wrappers and flags, including the scale enum, bound-versus-unbound matrix behavior, one-shot immediate update, and repeated validation semantics. Only the raw-resource helper enters the native resource decoding/lifetime boundary.
- `LiveActor` restores the original `initActorCollisionParts` body and the original collision calls in `calcAnim`, appearance, and death. Existing unrelated native owner boundaries remain.
- `CollisionPartsCompat` owns the actual part/server/JMapInfo in the caller's Game domain and decoded resource/control state on the host. It retains `ResourceArchiveOwner`, so both KCL/PA spans and the real ResourceHolder survive a resource-service release until the parts retire. Registration follows original `_CC` zone membership rather than an independently inferred actor-dead flag.
- `CollisionDirectorOwnership` leaves actual NameObjs to SceneObjHolder and reclaims their raw non-NameObj children afterward. It snapshots the complete original graph before NameObj deletion. SceneObj factory rollback also snapshots before deletion and clears the helper afterward, allowing a retry in the same binding. During normal actor retirement, original invalidation removes the zone entry; during whole-scene or provisional-owner rollback, the owning zones are retired as a group.
- Each auxiliary category has a separate instance of the same native KCL query engine, owned with the original director lifetime. Category 0 remains the actual stage map service. The old PlanetMap shortcut that mixed auxiliary meshes into map collision and returned null was removed. Original MoveLimit, WaterSurface, and Sunshade wrappers retain their actual category numbers and return actual parts.
- `Triangle` now retains the actual part pointer and local prism index for actor-owned geometry. Geometry-only service probes retain their existing global triangle identity. Original fill/recompute methods are used for actual parts; matrices and attributes remain accessible while a part is alive but temporarily absent from its zone. Released native ownership rejects stale borrowers without dereferencing their former part. Arbitrary original part filters now receive actual parts; sensor/actor filters can also apply to explicit sensor-backed geometry probes.
- Original `SunshadeMapHolder` and camera-code collection wrappers are activated. Sunshade queries inspect category 1 geometry through the same ordered native line engine, preserving ordinary map category isolation. The remaining auxiliary category query APIs are not claimed implemented merely because category 2/3 ownership exists.

## Independent helpers

`MR::calcDistance(const HitSensor*, const HitSensor*, TVec3f*)` is the original center-to-center vector/length function, including the zero direction at coincident centers. `MR::isExistMapCollisionExceptActor` excludes all sensors of the specified actual actor before accepting the first original ordered line hit. It does not repurpose a nearest-only hit or subtract one contact afterward.

## Verification

- `query-native-syntax.json`: the initial two helper providers and focused tests compile with LLVM 23.
- `owner-native-syntax.json`: all 19 affected native implementation/test TUs compile independently with LLVM 23. This is a compilation claim, not runtime validation.
- `prove-reference.py` and the `*-compile-command.json` manifests record ten fresh full-TU Wii compilations, all exit 0. The raw objdiff files and compact `reference-proof-summary.json` retain source/compiler evidence.
- Newly recovered `isExistMapCollisionExceptActor`: 112 bytes, 100% retail. Existing sensor distance: 164 bytes, 99.39024%.
- Original Director functions and MR lifecycle wrappers: 100%. Part ctor: 99.88372%; init, zone add/remove, reset/set, scale-selection init wrappers: 100%; updateMtx: 99.93827%; makeEqualScale: 99.34234%; farthest-vertex/camera-code walk: 99.234695%. The original imported force-move helper's current full-TU score is 83.52941%; its matrix behavior already has focused runtime evidence in `notes/original-collision-lifecycle-20260907/`. Existing projectToPlane is a low-score reference function (32.511112%) imported in the complete TU but not used to substitute any host query.
- `OriginalCollisionPartsOwnerTests.cpp` is ready for coordinated build/run with a real `SMGPC_REAL_DISC`. It uses actual scene/camera/resource owners and tests nested factory rollback/retry, Game allocation provenance, original pending/committed matrix timing, continuous/off/one-shot flags, real retained Triangle identity, and two scene lifetimes. Parent owns its Xmake target and runtime build lane.
- Existing line and sensor targets include the independent helper regressions. No full native build or GPU/runtime execution was performed by this subagent for this cohort.

## Remaining limits

The original keeper query bodies are not all decompiled. Their native category queries use the existing generalized KCL service; category 2/3 ownership does not imply complete water/movement-limit query APIs. The previously completed line and area query order/refit tests are prerequisites; the existing sphere query still documents its own source-prism capacity-order limit. Original part construction still relies on the existing scene/allocator exception boundary for an allocation failure *inside* its retail constructor; the new explicit rollback proof targets failures after complete construction. Native scene registration has no extra Game-specific collision mode or actor-position override.
