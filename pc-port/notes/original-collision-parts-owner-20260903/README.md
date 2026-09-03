# Original CollisionParts sphere and line queries

Recovered five missing root methods in `src/Game/Map/CollisionParts.cpp` from
the verified RMGK01 revision-0 DOL. The existing declarations were correct.
The only added include is `Game/Util/TriangleFilter.hpp`, needed for the
original virtual filter call. No native source, provider, actor lifecycle, or
scene query was changed in this group.

| Method | Retail address | Bytes | GC3.0a3 objdiff |
| --- | --- | ---: | ---: |
| `checkStrikeBall` | `0x8017677C` | 716 | 99.5419% |
| `checkStrikeBallCore` | `0x80176A48` | 424 | 98.962265% |
| `checkStrikeBallWithThickness` | `0x80176BF0` | 440 | 98.13636% |
| `calcCollidePosition` | `0x80176DA8` | 840 | 100% |
| `checkStrikeLine` | `0x801771A4` | 492 | 100% |

This is **2,912 retail bytes / 728 instructions**. `verify-source.py` compiles
the complete root TU using `configure.py`'s Game flags, splits the verified
DOL with DTK, and compares every instruction, external relocation, constant,
and branch destination. The remaining differences are limited to explicit
register allocation changes, the ordering of three independent zero stores,
and the scheduling of independent floating-point argument preparations.
Both 100% methods also relocate byte-for-byte to the actual DOL. The feature
switch's eight data relocations are separately checked against its loaded
retail jump table; a zero-filled ELF relocation slot is not treated as a
literal runtime pointer.

Run from the repository root:

```sh
python3 pc-port/notes/original-collision-parts-owner-20260903/verify-source.py
```

`source-evidence.json` records source hashes, precise normalizations, all
canonical instructions, and relocation targets. Compiler commands, object
files, DTK output, objdiff JSON, and disassembly stay ignored in
`build/original-collision-parts-owner-20260903/`. DOL SHA-1:
`25c5959534b3c21246c6c7e42021b916b41fb578`.

## Source behavior established

- Ball queries transform the center by the actual inverse base matrix and
  scale the radius by the mean of that matrix's three extracted scales.
  Contact penetration returns to world units through its reciprocal.
- A requested moving reaction only takes the sweep path when `_D4 == 0`.
  The path uses the inverse previous matrix to locate the old local center,
  then sweeps toward the current local center in
  `static_cast<s32>((1.0f / 35.0f) * movement.length()) + 1` subdivisions.
  It samples both endpoints, returns the first nonempty accepted contact
  set, and disables the movement-facing normal rejection on the last sample.
  The actual float at `0x806BBE84` is `0x3CEA0EA1`, the rounded reciprocal of
  35. No native sweep distance or timestep was invented.
- `checkStrikeBallCore` obtains at most the caller's candidate capacity from
  the KCL server **before** triangle filtering. It fills the actual part,
  local prism index, and sensor, then rejects invalid triangles and optionally
  normals with a positive movement dot product. Accepted contacts are compacted.
  A filtered candidate still consumed a KCL candidate slot, as in retail.
- The core records the original feature byte and projects world movement power
  onto the world face normal in `HitInfo::_7C`. `_70` is not written here.
- Thickness queries pass the requested thickness through unchanged while
  scaling the radius and penetration. They leave `_70` and `_7C` untouched.
- Feature 1 projects onto the face. Features 2/3 project onto the first/second
  edge plane through vertex 0; feature 4 uses the third edge plane through
  vertex 1. Features 5/6/7 return vertices 0/1/2. Other bytes leave the position
  unchanged. Jump table `0x80588F00` confirms this exact mapping.
- Line queries transform both endpoints, derive the local displacement, and
  call the original server's `checkArrow`. They transform the fractional hit
  position back to world space, record world line length times hit fraction,
  retain the server's feature byte, and apply the triangle filter at the
  original stage. `_70` and `_7C` are untouched.
- Each wrapper retains its original 64-entry local candidate arrays. The
  original keeper's hit buffer is 32 entries. No new capacity clamp, contact
  bias, zero-length fallback, or radius adjustment was added.

## Scene-owner dependencies before native activation

The committed typed `KCollisionResource` and actual server constructor/data
registration provide native KCL and PA storage. They do not establish placed
collision-part ownership. The next independent recovery is the missing KCL
narrow phase, followed by original keeper queries and the remaining point/area
methods. Production scene activation must also close the following owners:

1. The actual `CollisionDirector` creates `CollisionCode` and four category
   keepers. Keeper movement must commit the actual pending/current/previous
   matrices before actors move. The current world-triangle service has no
   corresponding mutable part matrices or part-local prism identity.
2. `CollisionCategorizedKeeper::getZone` lazily creates zones from the full
   `MR::getZoneNum()` count. Root `SceneUtil` delegates through
   `GalaxyStatusAccessor::getZoneNum()` to the actual ScenarioData. Native
   `StagePlacementResolver::resolve_stage_placement_tables` reads the complete
   ZoneList but does not retain it as a shared scenario-count provider. Counting
   only placed holder occurrences would lose unused zones and duplicate
   repeated instances. Native SceneUtil is excluded from the Game target.
3. `CollisionParts::init` unconditionally obtains the current placement zone,
   starts camera-code collection with the actual sensor host name and zone,
   runs `KCollisionServer::calcFarthestVertexDistance`, and finishes collection.
   That server routine inspects PA camera IDs and mutates near-parallel prism
   heights; it cannot be replaced by a bounds-only calculation.
4. The root collection path is
   `MR::init/register/termCameraCodeCollection` -> `CameraDirector` ->
   `GameCameraCreator`. The latter records codes below 255, creates group IDs
   `(zone, actor name, code, 0)`, and asks the actual `CameraParamChunkHolder`
   to create each chunk. The holder constructs `CameraParamChunkGame` for
   group IDs; the base chunk constructor immediately calls the real
   `CameraHolder::getIndexOfDefault()`. Passing a null or fabricated holder
   would not satisfy that lifecycle.
5. `CameraHolder`'s original constructor creates every camera and translator
   from its full source table before resolving the default XZ camera. The
   native selected-controller wrappers do not supply that complete catalog.
   Importing only the collection forwarding methods would leave a missing
   owner. The full director also owns the actual holder, chunk holder, creator,
   target/rail/register holders, managers, and view components; existing native
   components must be assembled coherently rather than represented by an
   invented partial director.
6. Actual part/sensor/resource ownership must remain stable for borrowed
   `Triangle::mParts` pointers, or invalidate retained contacts before teardown.
   Only then can the original matrix getters and refresh methods replace the
   current null-owner/global-triangle service representation.

The existing `CollisionParts::makeEqualScale` was also inspected at retail
`0x801762F0` (444 bytes). Its equal-scale test subtracts `(x-y, y-z, z-x)`;
the current root source permutes two of those tests. More importantly, retail
really does leave inverse-scale locals uninitialized for a nonuniform matrix
when neither `_D0` nor `_CF` is set. The near-equal path returns immediately;
the two flagged modes initialize the factors. This audit made no change and
does not introduce an identity-factor fallback. Any owner must preserve the
original scale-mode/input contract before this existing method is activated.

No native tests or shared xmake build were run for this source-only recovery.
It prepares actual collision behavior; it does not claim restored jumping,
dynamic grounding, or scene-wide collision ownership.
