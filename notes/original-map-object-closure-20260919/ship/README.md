# Original SimpleMapObj ship closure

Audited native checkpoint `0a45132a7`. No native source, factory, test target or GPU process was changed or run by this lane. The only source edit restores `LodCtrl::update` in canonical `decomp/src/Game/LiveActor/LodCtrl.cpp`, following `decomp/AGENT_DECOMP_GUIDE.md` and existing unit conventions. The native implementation already contains the same behavior and remains unchanged.

## Concrete dependency result

No absent native PlanetLodCtrl implementation was found. The original field is named `mPlanetLodCtrl`, but its actual class is `LodCtrl`. The ordinary original constructor wrapper, real ModelObj children, animation initialization, clipping owner and actor-owned lifetime already exist. The missing native factory row is therefore a registration gap, with actual ship-specific resource/owner execution still requiring validation. No replacement LOD algorithm is needed.

| Boundary | Current original behavior and evidence |
| --- | --- |
| Creator | `decomp/src/Game/NameObj/NameObjFactory.cpp:4267` maps `KoopaJrNormalShipA` to `createNameObj<SimpleMapObj>`. Native catalog already includes both main and Low archive records. |
| Original setup | `SimpleMapObj::init` invokes normal MapObjActor initialization. `MapObjActorInitInfo.cpp:200` sets low-model movement type 34 for this catalog name as original authored policy. All three placement args are -1, scale is 1.5, no rail or switch is assigned. |
| Resource model | Main archive has seven-joint BDL, 11 shapes/materials, and a 15-frame loop-mode-2 BCK. Freshly extracted Low archive has the same counts and its own 15-frame looping BCK. Low has no KCL. File names, sizes and hashes are in `ship-resource-inventory.json`. |
| Collision | `MapObjActor.cpp:224` initializes real main KCL using the original body sensor and an optional Move joint matrix, then `tryCreateCollisionMoveLimit`. `OriginalCollisionPartsUtil.cpp:180` creates MoveLimit in original category 3. `CollisionPartsCompat.cpp:87` retains each resource, decodes real KCL, initializes original CollisionParts, registers the actual zone and category, and owns every extra part under the sensor host. |
| Low construction | `MapObjActor.cpp:312` detects the Low archive and invokes `MR::createLodCtrlPlanet`. `PlanetMapRuntimeCompat.cpp:119` preserves original child construction, thresholds 5000/10000, far clipping and each child's own animation startup. The ship's child receives draw buffer 5, movement 34, calculation type 1. These integers are from the original wrapper and catalog. |
| Child ownership | `LodCtrl::initLodModel` constructs actual `ModelObj`, borrows the high model's matrix, initializes then kills the child, configures model-bounds clipping and copies TRS. There is no proxy mesh or host LOD stand-in. |
| View/control | Original ClippingDirector → ClippingActorHolder → ViewGroupCtrl registers real LodCtrl and binds four original view flags. `MapObjActor::control` calls its LodCtrl. Update preserves hidden/forced-high/middle/low precedence, distance thresholds, and transform following. Model changes retain the original two-update appearance/hide handoff. |
| Retirement | `ActorRuntimeRegistry::adopt_actor_lod_ctrl` owns the real controller. Actor release removes its original view-list entry before releasing it; model children are real scene NameObjs. Collision ownership retains both main and auxiliary parts per actor. Actual complete construction and scene retirement must still be probed. |

The Low archive was extracted directly using the existing read-only disc utility. A same-tool query for `KoopaJrNormalShipAMiddle.arc` produced no matching extraction; no middle file was created. The original runtime `isFileExist`/`initLodModel` result is the authoritative future probe, not an assumed dummy middle model. Main/Low archive evidence is real retail data; these are not generated assets.

One inspection-only failure-path caveat remains: `LodCtrl` enters ViewGroupCtrl in its constructor before `createLodCtrlPlanet` adopts it. If later child construction throws and execution were to continue, the temporary unique_ptr would delete the controller without an actor-owned entry to remove the borrowed view pointer. This was not reproduced, and no error-path patch or claim of proven rollback is included here. The normal successful lifetime has the existing owner boundary described above.

## Canonical recovery

The native LodCtrl update existed without a corresponding canonical definition. Retail `0x80166F08..0x801670AC` establishes the dead/inactive gate, high-only branch, forced-view priority, distance comparisons, and final copy of host TRS to the selected child. Copying the already correct recovered body into canonical source closes that provenance gap; no high/low tuning or gameplay condition changes.

`python3 notes/original-map-object-closure-20260919/ship/verify-recovery.py` compiles only this canonical TU to a notes-local object. It does not invoke Xmake or replace shared native build artifacts. MWCC passes; recovered `update` (420 bytes) matches 90.2381%, and the full 3,064-byte text matches 98.64883%. All other methods are 100% except `initLodModel` at 99.88764%. `.sdata` matches 100%; `.sdata2` is 89.655174% and `.data` 81.48148%. These differences are reported rather than described as an exact unit match.

The reference object text is independently checked against the retail DOL (`SHA1 25c5959534b3c21246c6c7e42021b916b41fb578`) across `0x80166C70..0x80167868`, masking only the exact instruction bits affected by its 94 ELF relocations. `source-mirror-proof.json` verifies that native source is unchanged and now equals canonical source after the existing NO_INLINE attribute relocation; headers are byte-identical. The existing `LodCtrlCompatTests` source boundary therefore has a complete canonical body to compare against. Its legacy standalone runtime fixture is not claimed to pass here.

## Required actual-process gate

The parent owns factory activation and the probe. `ship-placements.json` retains the three real MiddleZone rows: common ObjInfo rows 4/5/6, l_id 15/16/17, actual zone instance 4. A useful gate must establish:

1. Exactly those three ordinary SimpleMapObj owners, high/Low resources, real Low ModelObj and original registered view control; no scene-specific constructor path.
2. Main CollisionParts and MoveLimit CollisionParts in categories 0 and 3, with real body sensor/zone ownership and scaled matrices. Query both actual geometries and preserve keeper/category separation.
3. Original LOD threshold configuration, child animation and matrix following. Distinguish naturally observed visibility from test-only direct view-flag exercises, and preserve the original two-update transition.
4. Complete scene retirement of actors, children, LodCtrl entries and both CollisionParts resources without stale borrowed state.

A passing build or this isolated compiler match cannot establish these runtime properties, full ship rendering or complete Gateway scene parity. Factory activation remains the parent's next step.
