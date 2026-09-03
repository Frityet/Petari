# Original Binder collision response and zone dependencies

This tranche recovers the original root `Binder` response path. It replaces no
gameplay algorithm with a new collision solver. The supplied RMGK01 DOL has SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`; binary artifacts remain ignored under
`build/original-binder-reaction-20260903/`.

## Root source and evidence

`src/Game/LiveActor/Binder.cpp` now contains the missing `bind`,
`moveAlongHittedPlanes`, `findBindedPos`, `moveWithCollisionParts`,
`storeCurrentHitInfo`, `obtainMomentFixReaction`, `storeContactPlane` and
`copyPlaneArrayAndSortingSensor` bodies. The header's `compSensor` declaration and
definition now take `const HitInfo*`, matching the retail symbol and original
sort comparator. No other root Binder header change was necessary.

Run from the repository root:

```sh
python3 pc-port/notes/original-binder-reaction-20260903/verify-runtime.py
```

This uses the configured GC3.0a3 original compiler and unchanged Game flags,
`sjiswrap`, DTK 1.8.3 and objdiff-cli 3.6.1. It splits the hash-verified DOL into
its own ignored build folder. `runtime-evidence.json` records all 816 retail
instructions in the nine compared methods. All instructions agree after actual
relocation resolution and the narrowly enumerated differences below.

| Method | Retail address | Bytes retail/compiled | Raw objdiff |
| --- | --- | --- | --- |
| copyPlaneArrayAndSortingSensor | 0x8015D718 | 296/296 | 99.797295% |
| compSensor | 0x8015D840 | 24/24 | 100% |
| bind | 0x8015D858 | 1188/1180 | 97.340065% |
| moveAlongHittedPlanes | 0x8015DCFC | 260/260 | 99.846150% |
| findBindedPos | 0x8015DE00 | 388/388 | 99.896904% |
| moveWithCollisionParts | 0x8015DF84 | 148/148 | 99.864870% |
| storeCurrentHitInfo | 0x8015E018 | 228/228 | 99.649124% |
| obtainMomentFixReaction | 0x8015E1D4 | 476/476 | 99.445380% |
| storeContactPlane | 0x8015E3B0 | 256/256 | 98.828125% |

Five methods (`copyPlaneArrayAndSortingSensor`, `compSensor`,
`moveAlongHittedPlanes`, `findBindedPos`, `moveWithCollisionParts`) are byte-identical
after verified relocations, including actual SDA constants and HA/LO pairs. The
remaining differences are explicitly asserted by the verifier:

- `bind`: equivalent conversion of the one-bit flag to boolean; loop tests a
  valid `bool` as nonzero instead of equal to one. No different movement branch.
- `storeCurrentHitInfo`: two integer additions reverse their source operands.
- `obtainMomentFixReaction`: three pairs of independent scalar loads exchange
  order. Arithmetic, comparison operands and branch destinations are unchanged.
- `storeContactPlane`: a consistent exchange of registers r29 and r30.

No matching-only compiler pragmas or shared math/header changes were introduced.
The earlier isolated reaction-only compile produced extra inlining; compiling
the complete recovered group naturally restored the original outlining. The
checked-in evidence describes the final complete source, not that intermediate.

`evidence.json` additionally records 276 bit-identical executions of both actual
retail instructions and relocated original-compiler reaction instructions: 20
independent fixed cases plus 256 seeded finite combinations. Every conditional
branch in each reaction version executes both ways. The fixture models only
simple existing helper contracts (vector copies, addition, integer construction,
normal lookup and ordered `isNearZero`); it does not claim native SDK, KCL query,
moving-platform or complete gameplay execution.

## Exact reaction contract

The method reads the supplied `pPlane` array from `start` up to the Binder's
`mPlaneNum`. Its second `u32` argument is unused. Capacity enforcement belongs to
`storeCurrentHitInfo`, not this method. No normal normalization, depth clamp,
feasibility solve, averaging or accumulated-contact summation occurs.

For each contact it reads `mParentTriangle.getNormal(0)`, multiplies each
component by `_60`, then retains the largest positive and smallest negative
value independently for each component. Final output overwrites the destination
with the positive vector plus the negative vector. Opposing values can cancel.
For normals `(1,0,0)` and `(0.6,0.8,0)` at depth one, the result is `(1,0.8,0)`;
the previously active minimum-norm solve produced `(1,0.5,0)`.

When `_1EC._1` is set (retail byte mask `0x40`), a contact's `_7C` also contributes
to those extrema if `MR::isNearZero(_7C)` is false. The retail instruction at
0x8015E2E0 loads the default epsilon through `r2 + signed(0xBFD4)` from
0x806BBBF4, bits `3a83126f` (0.001). If any component lies outside the inclusive
epsilon box, all three components are considered. This term is not multiplied
by depth. The case set covers the exact epsilon and its next representable
positive/negative values, flag gating, ignored capacity, member count, suffixes,
non-unit normals, negative depth and output aliasing an already consumed input.

## Sweep and contact lifecycle

- `findBindedPos` takes `int(length * float(1/35)) + 1` movement steps and checks
  positions from step zero through the final step, inclusive. It zeros its
  movement output first and accumulates only executed steps.
- `_1EC._3`, or the retry's explicit argument, skips the initial query. It does
  not skip the corresponding later movement step.
- `_1EC._1` selects the moving-reaction sphere query. Filters pass through to
  the collision query unchanged.
- The 1.2-unit margin is added to **returned** hit depths by
  `storeCurrentHitInfo` at 0x8015E0B0–0x8015E0B8. It does not enlarge the sphere
  query radius or allow a separated sphere to produce a hit.
- `_1EC._5` is copied then cleared at `bind` entry. When set, it suppresses the
  depth margin and projected retry for that call. All other flags persist.
- After the initial reaction, one projected retry removes only the inward
  component of remaining movement along the normalized aggregate reaction. A
  backwards result is rejected. A full plane array still allows collision
  detection to stop the retry, but appends no contact or reaction.
- Offsets use the raw matrix columns, including their scale. Without a matrix,
  the default offset changes world Y. Local explicit offsets transform through
  the three columns; world explicit offsets add directly.
- `storeContactPlane` chooses the greatest-depth contact in each classification
  by the original floor/wall predicates; unclassified-as-floor-or-wall contacts
  become roof contacts. Equal depths retain the earlier contact.
- `_1EC._0` permits grounded moving-part transport after contact classification.
  That motion changes position/output but is excluded from `mFixReactionVector`.
- `_1EC._2` and a non-null extra part add it to the actual category-zero keeper
  before querying and remove it afterward. These original calls are retained.
- A Binder with capacity zero uses 32 temporary planes during `bind`.
  `copyPlaneArrayAndSortingSensor` subsequently returns the valid cached
  ground/wall/roof contacts, sorted by the original sensor comparator. A Binder
  with allocated planes returns pointers to all stored planes. The copy API's
  capacity argument is also unused by retail.

## Native data mapping and remaining system boundaries

The current native surface's `normals[0]` maps to the Triangle face normal;
the sphere contact's penetration maps to the pre-margin `_60`. Query contact
position and surface sensor/provenance remain separate fields. Static registered
geometry has no part displacement, so its moving-reaction `_7C` is zero.

Retail `CollisionParts::checkStrikeBall` initializes the static motion vector to
zero at 0x801769C8–0x801769E8. With moving reactions enabled and `_D4==0`, its
other branch compares the query position in current and previous part spaces,
steps that relative motion, and transforms the remaining part displacement to
world space (0x80176830–0x80176938). `checkStrikeBallCore` copies that vector and
projects it onto the face normal at 0x80176B6C–0x80176BA4. A unit face normal is
therefore not a valid replacement for `_7C`. Parent integration corrects that
native field independently; this recovery does not invent moving-part state.

Separate follow-up boundaries identified by the current-source audit:

- Native sphere contacts currently label every hit as face (`_88=1`), whereas
  retail KCollision returns face/three-edge/three-corner feature codes. Original
  `MarioCollision::createAtField`, `checkBaseTransPoint` and wall logic consume
  those distinctions. Recovering the prism sphere test and collide-position
  contract remains necessary for faithful edge/corner behavior.
- Native triangles cache world vertices and report no host movement. Real
  current/previous CollisionParts matrices and owner lifetime are required for
  moving-floor queries and original matrix accessors.
- The original `vecKillElement` projects using its raw direction; the old native
  helper normalized it first. Parent owns the shared math correction, along
  with restoring the direct original `PSVECNormalize` call.

## CollisionZone source closure

`src/Game/Map/CollisionCategorizedKeeper.cpp` additionally recovers
`calcMinMaxAndRadius` (0x80174C9C/0x1B8), `addAndUpdateMinMax`
(0x80174F40/0xDC), and `eraseParts` (0x8017501C/0x74), needed by the real extra-part
keeper calls. Bounds derive from each actual part's translation and `_D8` radius.
The center is the union AABB midpoint; radius is the maximum center-to-part
distance plus that part's radius. An empty zone resets both bounds, center and
radius to zero. The first bounds update uses the retail 0.1 sentinel.

Erasure finds the first matching pointer, replaces it with the last active
pointer and decrements the count; it does not preserve order or recalculate
bounds. An absent pointer changes nothing. No CollisionDirector or placeholder
part is constructed by this recovery.

After the Binder verifier, run:

```sh
python3 pc-port/notes/original-binder-reaction-20260903/verify-zone.py
```

`zone-evidence.json` records `calcMinMaxAndRadius` at 98.636360% raw objdiff
(440/440 bytes), `addAndUpdateMinMax` at 100% (220/220 bytes), and `eraseParts` at
86.896550% (116/104 bytes). Bounds calculation preserves every instruction after
two independent loads exchange order and its second-loop pointer uses r30 in
place of r31. Bounds accumulation is byte-identical after verified relocations.
Erasure's compiler reuses the end-pointer byte offset to load the last entry;
retail subtracts one from count and scales again. All remaining state effects
agree. A strict integer interpreter executes both actual instruction streams on
56 independent array cases, including empty/absent/first/middle/last/duplicate
entries, compares the entire array (including inactive slots) and count, and
covers both outcomes of every erasure branch.

Parent imported both root `.cpp` files byte-for-byte into native `Game` and
reported successful linkage through the real Binder; this agent verified the
mirror bytes and source hashes against the evidence. Parent owns runtime tests
and collision-provider integration. This directory's original-compiler evidence
does not claim the whole original Mario update or all moving collision actors
are already activated on PC.
