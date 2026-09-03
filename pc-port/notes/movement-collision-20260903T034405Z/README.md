# Triangle filtering and collision geometry

This change fixes shared map-collision behavior needed by original actor and
Mario movement code. No PC `Game/` gameplay code was changed.

Previously `Binder::bind` resolved the sphere against every candidate and only
then applied its `TriangleFilter`. A rejected floor or wall could still push or
stop the actor while leaving no retained ground/wall record. The line-query
provider also stopped at the nearest unfiltered prism and reported a miss if
that triangle was rejected, even when a valid farther prism existed. Sphere
filters ran after the global output capacity had already been exhausted.

`StageCollisionService` now accepts an optional triangle predicate for line,
sphere, thickness, and movement queries. Rejected triangles cannot contribute
to closest-hit selection, accepted contact capacity, sweep stopping, or the
reaction vector. Both original `TriangleFilterBase` providers use the same
adapter and expose source geometry to the filter. `Binder` explicitly reports
the absent exact `CollisionPartsFilter` provider when one is requested; it no
longer silently ignores that different filter type.

The host previously populated all three `Triangle::mPos` entries with a query's
single contact point, and left the three edge normals zero. It now retains the
three KCL prism vertices and all four original normal axes when registering
the collision resource. `HitInfoCompat` creates the same complete `Triangle`
snapshot for Binder, map line queries, and strike-info queries. `mHitPos`
remains the individual query contact. The source normal axes are transformed
by the matrix's linear part and normalized, independently of the geometric
plane normal used by the existing affine sphere solver.

## Retail evidence

The verified RMGK01 revision 0 DOL used for inspection is
`build/compat-math-oracle/main.dol`, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`. Symbol addresses come from
`config/RMGK01/symbols.txt`. Reproducible inspection uses
`python3 build/compat-math-oracle/disassemble_dol.py ADDRESS SIZE OUTPUT`.
Generated inspection files remain local in `build/compat-movement-collision/`.

- `Triangle::fillData`, `0x80182998`, size `0x1a8`: calls
  `PSMTXMultVecSR` (`0x804b8d14`) four times at `0x80182a44`, `a54`, `a64`,
  `a74`, then normalizes each result. It obtains KCL vertices 0, 1, 2 and
  transforms them with `TMatrix34::mult` (`0x80016950`) at `0x80182b04`,
  `b14`, and `b24`.
- `TMatrix34::mult`, `0x80016950`, size `0x90`, loads and adds all three
  translation values at matrix offsets `0x0c`, `0x1c`, and `0x2c`; the
  additions are at `0x800169d0` through `0x800169d8`. Thus vertices must
  include translation. The root decompilation incorrectly used
  `PSMTXMultVecSR` for these three position transforms. Root
  `src/Game/Map/HitInfo.cpp::fillData` is corrected to `TPos3f::mult`.
- `Triangle::calcAndGetPos`, `0x80182e30`, size `0x8c`, calls the same
  translated transform at `0x80182e9c`. Its existing root source is already
  correct and required no modification.
- `CollisionParts::checkStrikeBallCore`, `0x80176a48`, size `0x1a8`, fills
  the Triangle at `0x80176b18`, invokes the original virtual triangle filter
  at `0x80176b38`, and skips the accepted HitInfo counter increment at
  `0x80176ba8` when rejected.
- `CollisionParts::checkStrikeLine`, `0x801771a4`, size `0x1ec`, likewise
  fills at `0x801772f8`, invokes the filter at `0x80177318`, and skips the
  accepted HitInfo counter increment at `0x80177340` when rejected.

## Validation

New standalone `CollisionTriangleFilterTests.cpp` covers:

- Rejected floor and wall return the exact requested displacement with zero
  correction and no retained/classified contact; removing the same filter
  restores contact resolution.
- Thirty-two rejected registrations cannot hide a later accepted
  registration in the original sphere/thickness strike-info APIs, or consume
  a one-plane Binder's accepted contact capacity.
- Rejecting the nearest line hit returns the farther accepted KCL prism.
- A rotated, nonuniformly scaled, translated source prism exposes all three
  exact vertices and all four normalized source axes through original
  `Triangle` getters, refresh getters, line queries, strike-info queries,
  and Binder. A filter itself reads and validates this geometry before
  accepting it. The contact point and prism centroid are deliberately
  different, so a duplicated-contact shortcut cannot satisfy the assertions.

`git diff --check` passes. The new native collision-triangle-filter test
build and all four cases pass on macOS arm64 with LLVM 23. The real-disc
Mario stand/walk/release/recreate fixture also passes with these changes:
325.684 units, retained ground through the seam, and Wait -> Run -> Wait.
Its tested camera was the temporary native tracking implementation, which is
being replaced with the actual original camera controllers separately. This source correction has been
verified against the retail calls, but a byte-matching root object build has
not been claimed.

## Remaining limits

This is not complete original collision or Mario movement support. The host
still flattens registrations into a BVH and deterministic source-prism order;
the original per-part raw KCollision candidate limits and octree encounter
order are not reproduced. This change preserves accepted output capacity
across registrations but does not claim that overflow inside one original
KCL part has identical ordering. Dynamic CollisionParts matrices, part
identity/filtering, moving-platform reactions, exact edge/corner HitInfo
classification, and the full original Mario action/jump state path remain
separate work. Existing sphere sweep/skin/reaction policy and the PC Mario
walk slice's grounded force substitutions were left intact.
