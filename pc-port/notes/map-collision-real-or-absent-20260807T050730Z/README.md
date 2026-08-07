# Map-collision query ownership

## Change

`GameMapCollisionCompat.cpp` now requires an active, scene-owned
`StageCollisionService` before answering the original `MR` map-collision
queries. A missing collision owner is unknown/unavailable state, not proof that
a line missed every polygon.

When a real active service contains no registered KCL, a query still returns an
ordinary miss. Misses leave caller-provided hit position/normal storage
untouched; the compatibility layer no longer manufactures zero-valued hit
outputs.

## Evidence

`smg-pc-stage-collision-registration-tests` covers both sides of the boundary:

- no active service throws explicitly;
- an active service with zero registered KCL returns a real miss;
- the miss does not overwrite output vectors;
- explicitly registered valid KCL remains queryable.

Verification on 2026-08-07:

```text
[ok] collision absent without registration
[ok] only explicit valid KCL registers
2 stage collision registration test(s) passed
```
