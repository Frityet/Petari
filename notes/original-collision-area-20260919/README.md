# Original CollisionArea / AreaPolygon recovery

Recovered the three missing methods in canonical
`decomp/src/Game/AreaObj/CollisionArea.cpp` after reading the full retail unit,
AreaFormCube, and DynamicCollisionObj. Followed `decomp/AGENT_DECOMP_GUIDE.md`.
The existing header already declares the complete interface and correct fields;
it did not need edits. No native Game files, factory rows, or build rules were
changed by this recovery.

## Recovered behavior

- `AreaPolygon::init`: four vertices and two triangles (0,1,2 / 0,2,3), an eye
  sensor with zero offset, authored or supplied-matrix position, dynamic
  collision creation, and original appearance. Replaced the old commented
  partial attempt, whose allocation, sensor offset, and indices were wrong.
- `AreaPolygon::setSurface`: all six face orientations and winding orders,
  original matrix axes and scale, external-matrix Y offset, and the original
  non-Teresa ten-unit expansion. Transforms the selected quad through the
  normalized world axes while retaining translation in the actor position.
- `CollisionArea::hitCheck`: oriented-box broad rejection; original face,
  edge and corner branches; inside-volume penetration and tie ordering;
  contact and normal output; selected-face mask and surface synchronization.
- Removed the unused `FORCE_SCALE` matching dummy. It is absent from retail
  behavior and conflicts with an existing native compatibility symbol.

All behavior is shared by the original actor and its geometry classes. There
are no stage-name, actor-instance, coordinate or rabbit-specific conditions.

## Verification

`python3 notes/original-collision-area-20260919/verify-recovery.py` reproduces
the isolated MWCC compile, Objdiff report, and retail-reference comparison.
It does not run Xmake or write native sources.

| Check | Result |
| --- | --- |
| Original compiler | PASS |
| Full retail `.text` | 97.965225%, 5,636 bytes |
| `AreaPolygon::init` | 99.803925%, 408 bytes |
| `AreaPolygon::setSurface` | 97.8481%, 1,264 bytes |
| `CollisionArea::hitCheck` | 95.75703%, 1,992 bytes |
| Both actor virtual tables | 100% |
| `.rodata` and `.sdata2` constants | 100% |
| Canonical diff whitespace check | PASS |

The split reference text `0x800207b0..0x80021db4` matches the retail DOL
(`SHA1 25c5959534b3c21246c6c7e42021b916b41fb578`) after masking only the
instruction bits explicitly covered by 241 ELF relocations. The generated
object contains every original actor method. `.data` is 89.13% because of
literal/vtable ordering; individual virtual tables match fully.

Evidence: `wii-command.json`, `wii-compile.log`, `match-summary.json`, and
`objdiff.json.gz`. Remaining instruction differences include register and
temporary scheduling, calls supplying the unused `this` argument to the
existing `getBaseSize` declaration, and the explicit shift-sentinel guard
described below. No core vector or matrix headers were modified for matching.

## Retail details deliberately preserved

- Broad rejection uses `>=`; equality to expanded bounds is outside.
- A center inside the box retains `_3C == 0` for movement's penetration
  response, although the local contact-selection path then chooses one face.
- Equal penetration distances use the original comparison order, including
  the final Z choice; no alternative minimum-axis rule is introduced.
- The corner branch builds its corner relative to the origin, subtracts the
  supplied world position without adding the area's translation, and emits
  the normalized **positive** X+Y+Z axis sum. These are observable retail
  quirks, so the recovery does not silently correct them.
- The edge branch follows the original expanded-box test and output path;
  no additional Euclidean sphere-to-edge rejection is inserted.
- An invalid surface argument to `setSurface` skips the face assignment but
  still transforms the existing four positions, as in retail.
- The retail `slw` yields zero for the edge-contact surface sentinel `-1`.
  Native C++ shifting by `-1` is undefined, so the recovered mask check guards
  `surface >= 0` before `1U << surface`. The only generated values are `-1`
  and 0..5; this preserves the retail mask result for every path.

## Native support gaps at the audit boundary

This recovery is not evidence that CollisionArea is safe to register yet.
Gateway's two actual placements require the complete generated-collision path.
The separate native compatibility subtask owns the following work:

1. Import the complete existing DynamicCollisionObj interface/body. Its
   `createCollision` separately allocates position, normal, prism and octree
   arrays; this is a live mutable KCL, not an archive resource.
2. Provide explicit native ownership and counts for generated KCL. Current
   `KCollisionServer::setData` calls `require_native_kcollision_file`, which
   accepts only the retained decoded-resource registry. Current
   `getTriangleNum` subtracts unrelated prism and octree pointers; separate
   native allocations cannot supply the original count that way.
3. Preserve the single-root octree encoding across endianness. Original
   DynamicCollisionObj writes `u16[0] = 0x8000; u16[1] = 2`, followed by the
   triangle list and zero terminator. On Wii the first word is `0x80000002`;
   copying those two halfword writes to a little-endian host changes that word.
4. Register generated parts with the original scene/category/zone owner and
   publish geometry mutations. Current `CollisionPartsCompat` state lookup
   recognizes resource-backed parts only, and its publication updates
   transforms/membership, not the quad's vertices and prism normals.
   `setSurfaceAndSync` changes those arrays in place, then recomputes bounds.
5. Validate lifecycle, enabled/disabled membership, all face orientations,
   mutation and query behavior before adding factory rows. Preserve missing
   data errors; do not substitute a fixed collision plane.

Original player utility and AreaFormCube dependencies are already present in
the native tree, including real Mario area-push dispatch and Teresa state
queries. Their presence is source evidence only; no native link or gameplay
run was performed by this subtask.

## Published checkpoint

Committed only `decomp/src/Game/AreaObj/CollisionArea.cpp` as
`3787203d8ad835372a917795ee5ac0cc85025b49`
(`Recover original collision area surface and contact queries`). Push to
`origin/pcp-decomp` succeeded, and `git ls-remote` returned that exact SHA.
The pre-existing untracked `decomp/NPCUtil.d` is preserved.
