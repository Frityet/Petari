# Original collision keeper broad-phase queries

Recovered the sphere, explicit-thickness sphere, and line query methods in
root `src/Game/Map/CollisionCategorizedKeeper.cpp`, together with their missing
`isSphereOverlappingWithBox` helper. Existing method declarations were correct.
The only added includes provide the actual original CollisionDirector accessor
and CollisionParts filter interface. No native Game imports, resource changes,
scene owners, or production query activation are included.

| Method | RMGK01 address | Retail bytes | GC3.0a3 objdiff |
| --- | --- | ---: | ---: |
| `checkStrikeBall` | `0x80173FC4` | 588 | 98.84354% |
| `checkStrikeBallWithThickness` | `0x80174210` | 604 | 99.602646% |
| `checkStrikeLine` | `0x8017446C` | 700 | 100% |
| `isSphereOverlappingWithBox` | `0x80174A40` | 140 | 100% |

All **2,032 retail bytes / 508 instructions** are verified. Both sphere methods
differ only in complete GPR allocation permutations. Every call, vtable offset,
branch destination, comparison, arithmetic operation, field offset, stack slot
and memory access is otherwise identical. The line query and box helper also
match every retail byte after their verified relocations are applied.

## Preserved query behavior

Each query calls the actual `MR::getCollisionDirector` accessor before resetting
the keeper's hit count. Its return value is unused in retail, but the original
scene lookup is retained. The code does not manufacture a director or remove
that owner dependency.

Queries visit the stored zone array in order. The first array entry is the
original global zone and skips zone-level broad-phase rejection, irrespective
of the zone object's numeric ID. Later zones are culled before their parts are
visited. Each accepted zone's part count is captured once; stored parts retain
their original order. Invalid parts (`_CC == false`) are skipped, followed by
the actual virtual `CollisionPartsFilterBase::isInvalidParts` callback. A true
filter result rejects the part **before** part geometry tests or narrow phase.

Sphere queries first test the expanded zone box and then the squared distance
to the zone's bounding sphere. Part tests preserve the original sequence of
three separate `getTrans` calls, absolute XYZ-distance rejections, and final
squared-distance rejection. They pass the original radius and motion-reaction
flag to `CollisionParts::checkStrikeBall`, or radius and explicit thickness to
its distinct thickness method. Thickness does not enlarge the broad-phase
radius in retail.

The sphere hit limit is 32, matching the keeper constructor's actual array.
Each part receives the current output suffix and remaining capacity. Results
append in traversal order; they are not sorted or deduplicated at keeper level.
The keeper stores `_10` and returns immediately when the count reaches 32, or
stores the final count after all zones finish.

Line queries derive an axis-aligned segment box by adding each negative offset
component to the minimum and each nonnegative component to the maximum. Zone
and part tests first use their bounding spheres against this box, followed by
the original `MR::checkHitSegmentSphere` with the actual endpoint
`rPos + rOffset` and null direction output. That existing shared math body is
already decompiled in root `src/Game/Util/MathUtil.cpp`; it is not replaced by
a new intersection formula here.

A line capacity argument of zero selects 32. Other values are passed through
without clamping, and each accepted part receives `capacity - count`.
Completion occurs when `capacity <= count`, using the original signed
comparison. Valid callers must respect the keeper's actual 32-entry backing
array and the lower-level positive-capacity contract. This restoration does
not invent handling for negative or oversized capacities.

Despite its name, `isSphereOverlappingWithBox` is an axis-by-axis expanded-box
check, not a closest-point sphere/AABB test. It rejects a position strictly
outside `[minimum - radius, maximum + radius]` on any axis, in XYZ order.
Touching boundaries and the original unordered-comparison behavior are kept.
The additional zone/part sphere or segment test provides the subsequent cull.

## Verification

From the repository root:

```sh
python3 pc-port/notes/original-collision-keeper-query-20260903/verify-source.py
```

This passed with the final source. The script invokes the original GC3.0a3
compiler with the repository Game flags and Shift-JIS wrapper, splits the
verified retail DOL, runs objdiff, resolves calls/constants against the actual
binary, and compares all instructions. No instruction is discarded and no
control-flow normalization is needed. `source-evidence.json` retains source
hashes, exact register permutations, canonical instructions and relocations.

Commands, compiler logs, disassembly, split objects and objdiff output remain
ignored under `build/original-collision-keeper-query-20260903/`. Verified
RMGK01 revision-0 DOL SHA-1:
`25c5959534b3c21246c6c7e42021b916b41fb578`.

This completes the root sphere/thickness/line keeper-to-part-to-KCL query
chain recovered in the preceding CollisionParts and KCollision tranches.
Keeper point/area queries and same-host lookup remain separate missing source
work. Native activation still requires actual placed CollisionParts, complete
zone ownership, genuine scene/camera/chunk owners for initialization, and
Triangle part-local identities with retained current/previous/inverse matrices.
The original line array feature-output caveat remains documented in
`../original-kcollision-traversal-20260903/README.md`.

No shared xmake, native runtime, GPU test, or gameplay-success claim is part of
this root-only recovery.
