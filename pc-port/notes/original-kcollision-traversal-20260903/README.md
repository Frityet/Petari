# Original KCL octree traversal

Recovered `KCollisionServer::checkSphere`, `checkSphereWithThickness`, and
`checkArrow` in root `src/Game/Map/KCollision.cpp`, plus the previously
commented `isInsideMinMaxInLocalSpace` in its original
`src/Game/Map/KCollisionPlus.cpp` translation unit. Existing declarations were
correct. Existing `outCheck` and `objectSpaceToLocalSpace` required no changes.
No native collision query, owner, resource layout, or gameplay path is activated.

| Method | Retail address | Retail bytes | GC3.0a3 objdiff | Change |
| --- | --- | ---: | ---: | --- |
| `checkSphere` | `0x80183978` | 724 | 98.25414% | Restored |
| `checkSphereWithThickness` | `0x80183C4C` | 748 | 98.31016% | Restored |
| `checkArrow` | `0x80183F38` | 1,800 | 99.113335% | Restored |
| `isInsideMinMaxInLocalSpace` | `0x80185B7C` | 80 | 100% | Restored |
| `outCheck` | `0x80185BCC` | 296 | 100% | Existing body verified |
| `objectSpaceToLocalSpace` | `0x80185CF4` | 100 | 100% | Existing body verified |
| `V3u::setUsingCast` | `0x80185AF4` | 72 | 100% | Existing body verified |
| `std::find` prism-pointer specialization | `0x80185B3C` | 36 | 100% | Original template instantiation verified |

The four restored functions cover **3,352 retail bytes / 838 instructions**.
The complete verification covers **3,856 bytes / 964 instructions**. Every
instruction is checked, including calls, branch destinations, constants and
loads/stores. The sphere wrappers differ in local-stack and GPR allocation;
a 17-instruction leaf-stride block additionally reuses scratch registers.
The verifier symbolically checks every load/arithmetic/comparison/branch in
that block and every live output register. Arrow differs only in local-stack,
GPR and one disjoint FPR allocation range. No branch or arithmetic operation
is discarded from the comparison.

The repository's replacement `<algorithm>` has both mutable-reference and
const-reference `find` overloads. The explicit const-reference argument selects
the actual retail specialization while retaining a mutable local prism pointer.
That specialization is independently byte-exact after relocation.

## Sphere traversal contract

Both sphere wrappers build the center-plus/minus-radius box, clip it with
`outCheck`, and walk X within Y within Z. They ask the original `searchBlock`
for each leaf and use its width to choose the next X coordinate and minimum
remaining Y/Z strides. Coordinates retain unsigned traversal comparisons;
stride minima retain signed comparisons. The original initial stride is
`1000000`, not a replacement octree-size bound.

Prism lists start with an unused halfword, followed by indices ending in zero.
The query remembers the nonempty list with the greatest remaining Y span and
skips that same list on the next row. It rejects nonpositive prism heights and
checks the accepted-output list for duplicate prism pointers before and again
after the original narrow-phase call. Successful contacts retain leaf/list
order and write prism, penetration and feature together. Reaching capacity
prevents additional writes but does not terminate traversal or skip later
narrow-phase tests. Explicit thickness uses the distinct original
`KCHitSphereWithThickness` routine; it does not substitute the ordinary sphere
algorithm with a parameter change.

`outCheck` subtracts the stored KCL minimum and truncates floats toward zero.
It clamps the lower coordinates to zero and the upper coordinates to each
complemented mask, then rejects an inverted interval. It does not floor
negative fractional coordinates. The inside test preserves its original
three mask comparisons.

## Line traversal contract and caller preconditions

The line wrapper first rejects an exactly zero offset, then normalizes a copy
with the original `MR::separateScalarAndDirection`. The normalized direction
is used only for octree traversal. Every prism test still receives the original
position and full offset.

When the start is outside the KCL bounds, the original code tests entry through
X, Y, then Z planes and accepts the first candidate whose truncated coordinates
are inside. Positive direction chooses boundary zero; negative direction
chooses the complemented axis mask converted as **unsigned** to float. It is
not replaced with a generic slab intersection.

At each leaf the routine computes signed forward/backward cell strides. A zero
stride becomes +1 or -1 according to the direction. Components within the
original `0.001f` epsilon use the `1000000000.0f` step sentinel. It selects the
minimum of the three distances, stops when the remaining segment length is
less than or equal to that distance, and advances the retained float cursor
with the original vector copy/scale/add operations. All ordering and strict
comparisons are retained.

Several unusual retail output contracts are deliberately preserved:

- Scalar mode (`pPrisms == nullptr`) returns the nearest fraction below 1 in
  the first traversed leaf containing a hit. It writes one distance and feature.
  Its output count remains zero.
- Array mode writes each accepted distance and prism in traversal order, with
  **no duplicate suppression**. It increments the count, then tests equality
  with capacity; callers therefore need a positive capacity and sufficient
  backing arrays. Capacity zero is not a safe empty query.
- Array mode **never writes the feature array**. Only scalar mode writes
  `*pFeatures`. Retail stores at `0x80184444`–`0x80184454` and
  `0x80184498`–`0x801844A8` establish the distinction. The recovered original
  `CollisionParts::checkStrikeLine` passes a local feature array and later reads
  it, so this is an unresolved original indeterminate-value contract at native
  activation. No feature value is invented by this restoration.
- The per-leaf nearest fraction resets to 1 while the returned prism pointer
  persists, so the array-mode return value is not a global minimum across all
  leaves. Callers receive the complete distance/prism array and count.
- Degenerate-direction and failed-entry early returns leave `pCount` untouched.
  Final traversal and capacity exits write it only when the pointer is nonnull.

These contracts should guide an actual owner/query integration and its fixtures;
they are not permission to add scene-specific behavior or fabricated contacts.

## Verification and scope

From the repository root:

```sh
python3 pc-port/notes/original-kcollision-traversal-20260903/verify-source.py
python3 pc-port/notes/original-kcollision-query-20260903/verify-source.py
```

Both passed after the final source changes. The original compiler is GC3.0a3
using the repository Game flags and Shift-JIS wrapper. `source-evidence.json`
contains source hashes, exact normalizations, all canonical instructions and
relocation targets. Commands, compile logs, raw disassembly, split retail
objects, and objdiff output are ignored under
`build/original-kcollision-traversal-20260903/`. Verified RMGK01 revision-0 DOL
SHA-1: `25c5959534b3c21246c6c7e42021b916b41fb578`.

Adding the full traversal restores the original out-of-line `TVec3f::scale`
call in the earlier `KCHitArrow` body. Its current objdiff is 99.77273%, its
compiled size is now the original 704 bytes, and every canonical instruction
matches without the earlier inline expansion normalization. The prior
narrow-phase verifier now accepts either observed compiler form and its
source evidence has been refreshed.

The sphere/thickness/line KCL server chain is now present in root. Its native
activation still requires actual placed `CollisionParts`, category keeper
queries, complete scenario ZoneList ownership, genuine camera/chunk owners for
camera-code collection, and stable Triangle part-local identities with current,
previous and inverse matrices. Keeper `checkStrikeBall`,
`checkStrikeBallWithThickness`, `checkStrikeLine` and their broad-phase helpers
are still missing in root. KCL point/area traversal also remains separate work.
No shared xmake, native runtime, GPU test, or gameplay-success claim is part of
this source-only tranche.
