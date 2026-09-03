# Original area collision query graph — 2026-09-03

Recovered the five remaining complete area-query methods, covering 2,184 retail
instruction bytes, root first.

| Method | Retail address | Bytes | Objdiff |
| --- | --- | --- | --- |
| Keeper createAreaPolygonList | 0x80174728 | 440 | 99.59091% |
| Keeper createAreaPolygonListArray | 0x801748E0 | 352 | 99.48864% |
| Parts createAreaPolygonList | 0x80177390 | 236 | 100% |
| Parts createAreaPolygonListArray | 0x8017747C | 272 | 100% |
| KCollisionServer::checkArea3D | 0x80183604 | 884 | 98.579185% |

Both parts methods relocate byte-for-byte to the DOL. Keeper differences are
one r20/r21 register exchange. KCL area differs in callee-saved registers,
disjoint stack slots and temporary-register reuse for the same block-width,
mask and per-axis remaining-distance calculations. The verifier asserts the
complete instruction sequence after these explicit correspondences. Every
relocation operand and constant is checked against the DOL. No call, branch,
field, operation, comparison order or arithmetic expression is omitted. DOL
SHA-1: `25c5959534b3c21246c6c7e42021b916b41fb578`.

Keeper methods form the original world-space box, walk zones and active parts
in their stored order, reject nonoverlapping bounding spheres, and append up
to the caller's remaining capacity. They pass the original endpoints or point
array into each actual part. The parts transform the inputs through their
actual inverse base matrix and fill Triangle entries from real prism indices,
CollisionParts and HitSensor pointers. The original unused copied matrix and
zeroTrans call are preserved. Temporary arrays retain the original 512 prism
and 32 point limits; the Game callers retain their original preconditions.

KCL area orders the two endpoints and expands a zero-width axis by one unit on
each side. Existing outCheck clamps the integer box to the collision resource.
The original octree traversal skips its cached prior list and suppresses
repeated prism identities. Nonpositive-height prisms are rejected. Each
candidate's actual three vertices form a box tested against the query box,
with touching boundaries retained. It returns immediately when the original
capacity is reached. Result order follows octree traversal, not a new sort.

`root.patch` changes only CollisionCategorizedKeeper.cpp, CollisionParts.cpp
and KCollision.cpp after point checkpoint `4ba3e1425`. `verify-native.py` stages
those complete TUs plus existing original KCollisionPlus and compiles all four
against current native headers. KCollision keeps only the reviewed native raw
resource setData/isBinaryInitialized boundary. `native-incremental.patch`
refreshes the point package; `native.patch` is the full delta against native
production. Choose one. No registry/lifecycle snapshot or production mutation
is included. The earlier fast-query note separately corrects “mode 1” to the
actual one-hit capacity.

All original declared query methods in CollisionCategorizedKeeper,
CollisionParts and KCollisionServer now have source providers (the local-space
helpers are in KCollisionPlus). This establishes source and isolated compile
closure. Actual placed-query runtime still requires the original camera-code
collection, scenario zone catalog, resource owner, HitSensor and scene cohort
transaction, followed by query quiescence before actors and resources retire.
Native Binder and Mario activation remain gated on that runtime graph.
