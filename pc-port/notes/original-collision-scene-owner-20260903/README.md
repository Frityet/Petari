# Original collision scene ownership staging — 2026-09-03

This package stages the complete recovered CollisionParts, CollisionDirector,
CollisionCategorizedKeeper, HitInfo/Triangle, KCollisionServer and CollisionCode
translation units. **It is not activated or linked as a running placed scene.**
Camera/zone ownership and original MapUtil query accessors remain independent
activation gates. No production native source, shared build, or GPU was changed.

## Root correction and recovery

| Method | Retail address | Size | GC3.0a3 match |
| --- | --- | ---: | ---: |
| CollisionParts::makeEqualScale | 0x801762F0 | 444 | 99.41441% |
| JGeometry::TRotation3::getScale | 0x80175D4C | 172 | 97.06977% |

The scale-difference sequence is now the actual `(x-y, y-z, z-x)` and both
inverse-scale setters target the original templated setter. No default inverse
scale was added: nonuniform input still requires the original auto-equal or
no-scale mode. `getScale` computes the three original matrix-column lengths.
Every call/constant relocation has the original offset, kind, addend and target.
Remaining differences are floating-register allocation and independent load /
multiply scheduling; neither routine is claimed byte-exact. `verify-original.py`
rebuilds both root TUs and `compiler-evidence.json` records all differences.

## Resource identity and actual objects

`KCollisionSourceRegistration` retains the bounded **actual archive bytes**.
The first original `KCollisionServer::setData(raw_identity)` decodes their typed
native KCL; later servers receive that exact same KCL/prism/octree identity.
Only the original server's raw-file attachment and initialization predicate are
native architecture bodies. Its query/geometry/farthest-vertex code is copied
from root. PA uses the already established original JMap source registration.
ResourceHolder keeps its original raw file table: no name or table rewrite is
needed. Registration is lazy and shared. KCL initialization mutations affect the
retained typed prism storage without changing the immutable archived bytes.
Native resource/registry allocations explicitly escape the Game heap scope.

The new CPU fixture runs all ten existing resource tests plus actual shared
server construction on real JKR scene heaps, raw-source alias retention and
rejection, shared mutable native prism identity, immutable archive preservation,
and original JMap heap disposer cleanup. **11/11 groups pass normally and under
ASan/UBSan**, without sanitizer diagnostics. It uses complete original
KCollision.cpp; unused camera/geometry functions are removed by ordinary dead
stripping. It does not mock camera collection or claim CollisionParts::init ran.

`CollisionSceneOwnership` retains the exact ResourceHolder allocation cohort.
Its `create_director()` constructs the whole actual director under that heap;
SceneObjHolder's normal registration transaction owns the director and all four
keeper NameObjs. Original keeper init allocates real hit buffers and zones.
The private original collision factory has two allocation-only hooks: enter its
scene/resource scope and capture the actual freshly constructed part/server/map
objects. The original scale/category/resource lookup/init body remains intact.
Records retain the actual ResourceArchiveOwner until part/server/map destruction.
The original JMap disposer unregisters when explicitly destroyed before heap
retirement. Plain code/zone/HitInfo arrays retire with the retained scene heap.

The staging also copies four complete LiveActor methods from root:
`initActorCollisionParts`, `calcAnim`, `makeActorAppeared`, `makeActorDead`.
These restore actual `mCollisionParts`, pending matrix publication, and original
zone validation/invalidation. Their whole original lifecycle dependencies must
be selected together; this package does not claim native sensor/effect/clipping
owners are fully restored merely because the methods compile.

## Teardown and activation contract

Stop scheduler/query consumers, call `CollisionSceneOwnership::quiesce()` while
the real director exists, then retire actors and their sensors. Quiescence
removes real zone memberships through original invalidation. Retain collision
objects/resources until all borrowed Triangle contacts and actor callbacks have
ended, then destroy the owner before releasing its original scene heap.
Individual C++ actor destruction in a still-running scene is explicitly rejected;
original logical appear/dead remains available. A constructor-failure unwinder
must perform this same quiescence before deleting any partially built actors.
No fake sensor/actor, null part, identity-matrix floor substitute, or fabricated
camera collector is introduced.

NameObj/actor registry registration and name mutation receive narrow host
allocation guards. These guards do not solve bulk retirement alone. The current
native JkrHeapFinalizer runs **after** original JKRDisposer cleanup; using it to
release the actor registry would double-destroy objects such as the owned
AudAnmSoundObject, itself a JKRDisposer. Model/sound owners also retain the cohort,
so waiting for final domain release can leave a registry/domain retention cycle.
The required scene transaction order is scheduler/borrower disconnection,
explicit native actor-owner retirement while original disposers remain live,
NameObj registration retirement, then Game heap disposal. Do not add automatic
calls to every derived Game destructor to freeAll. Parent owns this integration.

`native.patch` and `staged-files.txt` describe the exact staged delta. Current
stage is `build/original-collision-scene-owner-20260903/staged`; **13/13 TUs
compile** with the selected native Game flags (`compile-evidence.json`).
`verify-resources.py` rebuilds the isolated real heap/resource fixture.
The root checkpoint comprises only the two root source files, `root.patch`,
`verify-original.py`, and its compiler evidence; the remaining package is staged.

Before atomic activation:

1. Parent supplies actual CameraDirector/catalog/creator/chunk lifetime and full
   ScenarioData ZoneList identity/count. Original CollisionParts::init calls
   collection start/farthest scan/finish unconditionally.
2. Select the real director factory and establish/retain the collision owner in
   scene lifecycle; preserve movement category 0x20 ordering before actor motion.
3. Replace static GameMapCollisionCompat/HitInfoCompat query authority with
   recovered original Collision/MapUtil accessors. Point/area/same-host keeper and
   part bodies remain missing. The root first-hit wrappers are the next tranche.
4. Remove OriginalKCollisionCompat and OriginalCollisionPartsCompat after the
   full TUs replace their duplicate methods. Remove old CollisionPartsCompat
   registration and move its ResourceHolder service accessor to its own native
   boundary. Showcase.cpp and GatewayDemoScene.cpp are the only diagnostic
   consumers of old actor_collision_parts_resources; remove/migrate them at the
   same cut. Triangle assignment remains the literal body in MarioActor.cpp (or
   one extracted provider), never a duplicate HitInfoCompat definition.
5. Close the complete restored LiveActor lifecycle's sensor/effect/clipping
   providers before linking it. DynamicCollisionObj's separately generated KCL
   constructor needs its own typed generated-resource owner before selection;
   this package covers ordinary resource-holder-backed collision generically.

All existing movement/jump walking-slice gates remain in place. Actual part
identity and queries must be complete before original floor-relative movement
or jumping is enabled.
