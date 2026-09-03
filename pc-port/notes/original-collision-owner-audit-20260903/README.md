# Original collision ownership audit — 2026-09-03

This is a read-only source and retail-binary audit. No collision production
code was changed, no owner was activated, and no native build was run. The
recommended next unit is genuine collision resource/lifecycle construction,
followed by original query recovery before switching placement and movement to
it. Adding a partially initialized `CollisionParts` solely to make the three
triangle matrix getters return pointers would conceal the missing lifecycle.

## Current boundary

`pc-port/src/compat/CollisionPartsCompat.cpp` retains raw KCL/PA spans, an initial
matrix, sensor, zone provenance, and a registration lifetime for each actor. It
never constructs `CollisionParts` or assigns `LiveActor::mCollisionParts`.
`StageCollisionService::register_kcl` decodes the initial matrix into immutable
world triangles and a BVH. The service does not retain the KCL octree or a
per-part previous/current transform.

`StageCollisionService::move_sphere` does execute the original `Binder`; its
point/sphere/line queries still use `GameMapCollisionCompat`. That provider sets
`HitInfo::_88 = 1` for every contact. Original `HitInfo` distinguishes face 1,
edges 2–4, and corners 5–7, which the restored `Mario::updateBinderInfo` consumes.
Parts filters are explicitly unavailable because these surfaces have no parts.

`HitInfoCompat` makes triangles with null `mParts` and a global surface index.
It can return cached world vertices and PA attributes, but the original three
matrix getters are absent. Original `Triangle::fillData` instead stores the
actual part and its **local** prism index. An integration must restore both
together; a global BVH triangle index cannot become the original `mIdx`.
`calcAndGetPos` and `calcAndGetNormal` must also re-evaluate the current part
matrix, rather than retain the initial world-space cache.

`PlanetMapRuntimeCompat::try_register_auxiliary_collision` currently returns
null after registering auxiliary geometry with the same service. Original
main geometry uses category 0, Sunshade 1, WaterSurface 2, and MoveLimit 3.
The original auxiliary creation functions return separately owned real parts,
retain the follow matrix, and validate membership. Category separation is part
of this migration, not a name-based exception to the native query path.

## Actual lifecycle to preserve

1. Create the scene's actual `CollisionDirector`. Its existing root constructor
   creates `CollisionCode` and four real `CollisionCategorizedKeeper` instances,
   and registers movement type `0x20`. Native `MR::getCollisionDirector` already
   performs the original scene-object lookup, but the native scene-object
   factory and `StageHostScene` do not create that object yet.
2. Load a retained, decoded KCL resource and PA table. Construct the original
   `CollisionParts`, whose constructor creates the actual `KCollisionServer`.
   Run its original initializer: reset matrices; capture the sensor and current
   placement zone; obtain the real keeper zone; collect camera codes; calculate
   farthest vertices; apply the selected original scale mode and radius.
3. Preserve original creation differences. An explicit matrix passed to
   `createCollisionPartsFromLiveActor` is also retained as `_0` for following.
   The resource-holder overload uses an initial matrix but does not establish
   that follow pointer. `LiveActor::initActorCollisionParts` assigns the real
   main part and initially invalidates it. Scale-selection booleans must no
   longer be ignored.
4. Restore original validate/invalidate calls in actor appearance/death and
   auxiliary part ownership. `_CC` denotes zone membership; `_CD` enables normal
   updates and `_CE` enables one update. Zone removal must happen before the
   sensor, resource, or part is destroyed.
5. Restore original `LiveActor::calcAnim` collision-matrix submission. It writes
   the pending `CollisionParts::mMatrix`; it does not immediately change the
   committed base matrix. `CollisionDirector::movement`, before MapObj `0x22`
   and Player `0x25`, commits pending matrices via keeper movement. The current
   scheduler already runs movement before its later calc-animation phase.
6. Keep `mPrevBaseMatrix`, `mBaseMatrix`, `mInvBaseMatrix`, and `_D4` updated by
   original `CollisionParts::updateMtx`. The latter tracks successive unchanged
   matrices; original `Triangle::isHostMoved` tests `_D4 == 0`. The keeper only
   updates a part in its owning category, even if another category temporarily
   also contains it. Retained ground triangles must follow this same owner.
7. Release complete native-owned allocations at the scene/actor lifetime
   boundary. Retail destructors and scene-heap reclamation must be distinguished:
   do not assume the existing empty/generated destructors reclaim arrays,
   servers, map tables, and zones. `StageHostScene` currently destroys actors
   before collision deactivation and scene-object teardown, which is a usable
   insertion point. Any contacts retained past an actor's destruction need an
   explicit lifetime invalidation path before dereferencing their raw part.

Relevant source: `src/Game/Map/CollisionParts.cpp`,
`src/Game/Map/CollisionDirector.cpp`, `src/Game/Map/HitInfo.cpp`,
`src/Game/Util/LiveActorUtil.cpp:2266`, `:2740`,
`src/Game/LiveActor/LiveActor.cpp:48`, `:82`, `:164`, `:389`, and
`include/Game/Scene/SceneFunction.hpp:40`.

Original player callers require these transforms now:
`MarioEnforce.cpp::recordLastGround` converts the world contact into the part's
local frame and `getLastGroundPos` converts it back through the current base.
`MarioCollision.cpp::recordSafetyTrans` compares previous/current transforms;
`MarioHang.cpp` also stores coordinates through the inverse base. An identity
matrix is incorrect even for a static translated or rotated planet.

## Required resource and system closure

### Typed KCL, not a cast of archive bytes

The Wii `KCLFile` is a 56-byte big-endian header with four 32-bit offset/pointer
unions. Those unions widen on the native host; the raw header cannot be
reinterpreted as the native type. Decode the vectors, prism fields, thickness,
minimum, masks, and shifts into retained, aligned native storage.

Preserve the dummy prism at `mPrisms[0]`: actual prisms begin at `mPrisms + 1`.
The original `getTriangleNum` derives the count from the byte distance between
that first real prism and `mOctree`, so the native allocation must preserve that
adjacency (or use an explicitly justified architecture adaptation). Do not count
the dummy as a triangle or replace local prism indices with service indices.

The octree stores relative byte offsets, signed 32-bit node words, and 16-bit
leaf prism lists. Node and leaf payloads require different endian conversion;
blindly swapping every word is insufficient. Validate reachable offsets,
node/leaf extents, leaf termination, prism indices, and traversal bounds before
original pointer-based queries consume it. Detailed accepted malformed-input
boundaries must be derived from the recovered queries, not guessed in advance.

`calcFarthestVertexDistance` mutates nearly parallel prism heights, so the
decoded resource must be mutable and retained. This mutation belongs to decoded
storage, never the immutable big-endian archive. Shared resource lifetime must
outlast all servers using it; each instance still owns distinct part matrices.

The source `KCollisionServer::setData` contains a concrete reversed condition:
it relocates when `isBinaryInitialized` is true. Retail calls that predicate at
`0x801831A8`, compares to zero, then **branches past relocation if nonzero** at
`0x801831B0`. Root must first change to `if (!isBinaryInitialized(pData))` and
be verified with the original compiler before import. The retail predicate at
`0x80183390` is simply a sign-bit test of the first 32-bit word; that Wii cached
address heuristic cannot identify an initialized native 64-bit resource.

The native `setData` architecture boundary should resolve validated typed
resource ownership, rather than inspect the low half of a native pointer or
claim every raw asset is already initialized. The current custom native
`ResourceHolder` is not the original class with `mFileInfoTable`; coordinate
this with the ongoing typed J3D/resource ownership work instead of introducing
a second competing resource holder.

PA needs the same bounded resource lookup. Native
`JMapInfo::attach(const void*)` currently **always returns false**. Its existing
`from_bcsv(span)` parser can provide decoded table storage, but the original
unsized `attach` call must recover a real retained resource extent/owner. Do
not silently attach an empty table or assume a guessed buffer size.

### Zones and camera code ownership

Retail `CollisionCategorizedKeeper::getZone` lazily creates one `CollisionZone`
for every entry reported by `MR::getZoneNum`, then returns `mZones[zoneId]`.
The call at `0x80174B7C` is to `MR::getZoneNum` at `0x803F78A0`. It does not count
placed holder occurrences. Original `GalaxyStatusAccessor::getZoneNum` delegates
to ScenarioData. The native resolver reads a complete ZoneList but currently
returns tables/occurrences without retaining that list as a shared scenario
count provider. Retain the full catalog, including zones absent from the active
placement, and preserve the original 32-zone capacity at the validated input
boundary. Do not infer the count from placed objects or the largest placed ID.

Parts initialization also requires meaningful camera-code collection. Existing
root `GameCameraCreator` collects codes below 255 and creates actual group-ID
chunks using `(zoneId, actor name, code, 0)` in `CameraParamChunkHolder`.
The native slice has no corresponding active collection owner. Its chunk
constructor/holder dependencies include `CameraHolder` and original game chunk
creation. This must be coordinated with real camera catalog ownership; a no-op
would discard authored ground-camera identity even if collision tests passed.

## Bounded recovery and activation proposal

The first independent, reviewable recovery group is:

| Missing root routine | RMGK01 address | Bytes |
| --- | --- | ---: |
| CollisionCategorizedKeeper constructor | `0x80173B6C` | 152 |
| CollisionCategorizedKeeper::movement | `0x80173C04` | 224 |
| CollisionCategorizedKeeper::getStrikeInfo | `0x80174B44` | 16 |
| CollisionCategorizedKeeper::getZone | `0x80174B54` | 160 |
| CollisionCategorizedKeeper destructor | `0x80175090` | 88 |
| CollisionZone::calcMinMaxAndRadiusIfMoveOuter | `0x80174E54` | 236 |

This is 876 retail bytes, plus the small `setData` condition correction. The
keeper constructor/movement/getZone were inspected directly for this plan;
they have not yet been implemented or compiled. The destructor must be read
before deciding native heap cleanup. Existing `CollisionParts::makeEqualScale`
also has an apparently uninitialized `invScale` path in the current root source;
verify its retail control flow before importing it rather than inventing a
default for nonuniform scale.

Combine that root group with the typed KCL/PA resource owner, complete zone
catalog, actual director creation, camera-code collection owner, and native
allocation teardown. Test the actual lifecycle in isolation. This is a useful
foundation checkpoint, but **keep production placement/query activation off**
until the genuine query group is recovered and verified.

The remaining geometry source gap is substantial: the keeper source currently
contains only add/remove and zone helpers. It lacks all point/ball/line/area
queries. CollisionParts likewise lacks these queries, and KCollision lacks
their narrow-phase algorithms. From `config/RMGK01/symbols.txt`, the complete
missing keeper group is 4,064 bytes, the missing parts queries 3,952 bytes, and
the missing KCollision queries/helpers 9,324 bytes, plus the 236-byte zone
routine above: **17,576 retail bytes** in total. This includes the lifecycle
group and all area queries; it is a size estimate, not a match or effort claim.

Recovering the real point/sphere/thickness/line chain preserves contact-feature
codes, truncation/order, parts filters, category routing, and local transforms.
Then switch the existing MR query entry points to the actual director/keeper
chain, and import original Triangle getters and refresh methods. Retain service
resource names/global diagnostic indices separately if useful, but retire the
active static triangle response path. The existing original Binder can then
consume original collision contacts without a second collision response model.

## Verification requirements

- Raw KCL bytes stay unchanged; typed header/arrays/octree and PA decode valid
  little-endian values, including local prism and attribute identities.
- Two placed parts sharing one resource retain different matrices and stable
  local triangle identity through resource-cache and actor teardown.
- Translated/rotated/uniformly scaled parts expose correct base/inverse/previous
  matrices. Pending matrices change only when the owning keeper moves; a part
  temporarily in another category is not advanced twice.
- Appearance, death, reappearance, disabled updates, one-shot updates, and
  unchanged-matrix `_D4` progression match original source/retail.
- Main, Sunshade, WaterSurface, and MoveLimit categories remain distinct and
  filters run at the original stage relative to finite strike-buffer capacity.
- Face/edge/corner contacts produce original feature codes and penetration;
  moving-contact power uses previous-to-current part transforms.
- A recorded local ground point returns the new world point after movement;
  it must not read a destroyed part after lifecycle release.
- Camera-code collection preserves the real actor/zone/group IDs and a full
  scenario ZoneList supplies zones even when a zone has no placed occurrence.

`verify-audit.py` reproduces the small retail-word checks and records source
hashes in `source-evidence.json`. It does not claim functional verification of
the unrecovered routines. Raw disassembly remains ignored under
`build/original-collision-owner-audit-20260903/`.
