# Original KCL prism narrow phase

Recovered the three missing prism intersection routines in root
`src/Game/Map/KCollision.cpp` from the verified RMGK01 revision-0 binary.
Existing declarations were correct. No native provider, resource layout,
scene owner, or gameplay code was changed.

| Method | Retail address | Retail bytes | GC3.0a3 objdiff |
| --- | --- | ---: | ---: |
| `KCHitSphere` | `0x80184640` | 1,712 | 99.81309% |
| `KCHitSphereWithThickness` | `0x80184CF0` | 1,660 | 99.80723% |
| `KCHitArrow` | `0x80185448` | 704 | 99.77273% |

The group covers **4,076 retail bytes / 1,019 instructions**. The original
compiler produces identical instruction graphs for both sphere routines with
only computational `f30`/`f31` allocation exchanged. Their ABI save/restore
instructions are unchanged. After the subsequent octree traversal recovery,
the line routine also retains its original out-of-line `TVec3f::scale` call;
its compiled body is now the original 704 bytes and every canonical
instruction matches. At the initial narrow-phase checkpoint, the compiler
instead inlined that call into the incomplete TU (95.392044%, 24 bytes
longer). The verifier retains an explicit check for that earlier compiler
form, validating each expanded XYZ load, rounded multiply and store against
the actual retail callee at `0x800200D0`. Current evidence uses the
out-of-line form, without that normalization.

Run from the repository root:

```sh
python3 pc-port/notes/original-kcollision-query-20260903/verify-source.py
```

`source-evidence.json` contains hashes, exact normalizations, all canonical
instructions, and relocation targets. Original-compiler commands/logs,
disassembly, split retail objects, and objdiff output remain ignored under
`build/original-kcollision-query-20260903/`. Verified DOL SHA-1:
`25c5959534b3c21246c6c7e42021b916b41fb578`.

## Preserved sphere behavior

Both routines clear the feature byte, measure the center relative to the
prism's stored origin, and test its three edge planes. The third plane uses
the stored prism height. A plane distance equal to the sphere radius is
rejected. Face penetration is `radius - faceDistance`, and its negative result
is rejected after writing the distance output, matching retail.

The original comparison tree chooses the nearest edge/vertex region. It
retains the exact ordering and negated comparisons, including tie and unordered
comparison behavior; it is not replaced by a generic closest-point formula.
Inside all three edge planes, the face result uses feature 1 and the original
thickness limit. Edge results use features 2–4. Vertex results use features
5–7 and the original two-normal coefficient calculation and square-root calls.
Output mutation on rejected paths is retained.

The two routines have distinct contracts:

- Ordinary sphere queries scale `mFile->mThickness`. Edge-region offsets must
  not exceed the face distance. Vertex-region planar distance must not exceed
  the face distance and must remain strictly below the radius. Final
  penetration is `sqrt(radiusSquared - planarDistanceSquared) - faceDistance`,
  checked against zero and scaled thickness.
- Explicit-thickness queries scale the supplied thickness argument. They omit
  those edge/vertex comparisons against face distance. After the planar test,
  they additionally reject when
  `faceDistance + sqrt(radiusSquared - planarDistanceSquared) < 0`, then write
  and bound the final penetration. Collapsing these functions into the ordinary
  test with a different thickness value would change original behavior.

All scalar products and sums preserve their original non-fused single-precision
order. A future native import must retain that arithmetic contract rather than
permit reassociation or contraction.

## Preserved line behavior

`KCHitArrow` uses the original SDK vector subtraction and dot products. It
rejects starts at or behind the face, rejects segments whose endpoint remains
in front, and calculates the intersection fraction from the signed face
distance and movement. It then tests the three edge planes with the actual
`0.01f` tolerance (`0x3C23D70A` at `0x806BC05C`).

The third plane's rejection threshold is `0.01f + prismHeight`, while the
original edge-feature flag still compares its **raw** dot product against
`[0, 0.01f]`; it does not subtract the height for that flag. This unusual
distinction is present in retail and is preserved. The three edge flags feed
the original explicit feature table. Rejected calls write feature 0 but leave
the distance untouched; accepted calls write the intersection fraction and
feature 1–7.

## Remaining integration

These are prism-level tests. The subsequent root octree traversal recovery is
documented in `../original-kcollision-traversal-20260903/README.md`, including
cell stepping, duplicate suppression, output ordering and capacity contracts.
Original keeper point/sphere/line/area queries remain missing. The five
recovered CollisionParts wrappers and their source proof are documented in
`../original-collision-parts-owner-20260903/README.md`.

Native activation still requires actual placed CollisionParts, category
keepers, full scenario ZoneList ownership, camera-code collection through
genuine camera/chunk owners, and stable Triangle part-local identity and
matrix lifetimes. This tranche adds no empty owner or link-only matrix
provider. No shared xmake, GPU run, or native gameplay test was performed for
this root-only recovery.
