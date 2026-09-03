# Original point collision and same-host queries — 2026-09-03

Recovered four complete root methods, covering 1,748 retail instruction bytes.

| Method | Retail address | Bytes | Objdiff |
| --- | --- | --- | --- |
| CollisionCategorizedKeeper::checkStrikePoint | 0x80173DE4 | 480 | 98.166664% |
| CollisionCategorizedKeeper::searchSameHostParts | 0x80174ACC | 120 | 100% |
| CollisionParts::checkStrikePoint | 0x80176568 | 532 | 99.766914% |
| KCollisionServer::checkPoint | 0x8018339C | 616 | 99.96753% |

The original compiler proof compares every instruction, branch, call, field
access and operation. Keeper point differs by one bijective register
allocation. Parts point differs only in two pairs of disjoint stack slots.
Same-host and KCL point relocate byte-for-byte to the DOL. Every relocation in
all four methods is also checked against its actual retail operand and constant
bytes. `verify-original.py` and `compiler-evidence.json` contain the precise
correspondences. DOL SHA-1: `25c5959534b3c21246c6c7e42021b916b41fb578`.

The keeper resets its hit count, walks original zones and active parts, applies
box/sphere rejection, and stops on the first successful point hit. The caller's
optional HitInfo pointer goes directly to the actual part; the keeper does not
substitute its line/sphere result array. Its count becomes one on success.

Parts point transforms the point through mInvBaseMatrix and derives scale from
that actual inverse matrix. For inverse scale greater than one, it checks a
single thickness sphere with radius 20 times scale and thickness twice that
radius, then computes depth against the returned prism face. Otherwise it calls
the original KCL point routine. Output stores the actual CollisionParts,
part-local prism index and HitSensor through Triangle::fillData, depth divided
by inverse scale, and the world-space point offset along the triangle normal.
It does not invent a collision feature or overwrite unrelated HitInfo fields.

KCL point preserves the original integer truncation and per-axis masks, visits
the actual octree leaf in its stored order, skips nonpositive prism heights,
checks all three edge planes, and accepts face depth from zero through scaled
thickness, inclusive. It returns the actual prism pointer. Same-host compares
mHitSensor->mHost pointers and returns the first match in keeper traversal
order. The original method has no active-parts test or self exclusion.

`verify-native.py` stages and compiles four complete TUs against current native
headers: CollisionCategorizedKeeper, CollisionParts, KCollision and the already
existing root KCollisionPlus. The latter closes the local-space helper provider
required by existing sphere/arrow code and the upcoming area query. KCollision
retains only the previously reviewed native setData/isBinaryInitialized raw
resource boundary; all query bodies are root-identical. The current native
matrix header takes precedence over old collision-owner header snapshots.
`native-incremental.patch` refreshes the earlier collision-owner staging;
`native.patch` is the complete delta against native production. Choose one.
No ActorRuntimeRegistry or lifecycle owner snapshot is included.

This checkpoint is original-instruction and isolated native compilation proof,
not a placed-collision runtime claim. Original Binder/Mario activation remains
gated on the complete query graph and the parent-owned camera/zone/scene cohort.

The remaining coherent area graph is five missing methods, 2,184 bytes:
CollisionCategorizedKeeper::createAreaPolygonList (440), its array variant
(352), CollisionParts::createAreaPolygonList (236), its array variant (272), and
KCollisionServer::checkArea3D (884). The original KCollisionPlus local-space
helpers and MR::createBoundingBox already exist. Mario spin collision requests
256 triangles from the pair-of-points entry; MarioShadow supplies eight corners
to the array entry. Original part arrays hold 512 prism pointers and 32 local
points. Recovery must preserve bounded capacities, actual local transforms,
original duplicate suppression and result order. No synthetic world-triangle
adapter closes these methods.
