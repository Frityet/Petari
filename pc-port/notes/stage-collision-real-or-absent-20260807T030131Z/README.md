# Stage collision: real request or absent

## Outcome

Stage collision is no longer inferred from placement rows or harvested from object archives. A stage starts with an empty active `StageCollisionService`; collision can enter it only through the explicit resource-level `add_kcl` API that a real `Game/CollisionParts` request can call with its exact KCL bytes, transform, and source identity.

No file under `pc-port/src/Game` was changed.

## Removed fallback behavior

- Deleted archive enumeration and case-insensitive KCL candidate matching.
- Deleted the “use the only KCL in the archive” branch.
- Deleted placement-driven archive loading for every factory-supported object.
- Deleted exception swallowing around guessed optional collision resources.
- Removed placement/archive counters that existed only for the harvesting path.

Unsupported or not-yet-implemented `CollisionParts` behavior now has no collision representation. It is absent rather than approximated.

## Preserved generalized boundary

- `StageCollisionService::add_kcl` still accepts caller-provided decompressed KCL bytes, a row-major 3x4 transform, and caller-provided source identity.
- `build`, line, sphere, binder-motion, stats, activation, and teardown APIs remain available.
- `add_kcl` does not resolve, normalize, scan, or substitute resource paths.

## Focused test

`StageCollisionRegistrationTests.cpp` checks both sides of the boundary:

1. A built but unregistered service has zero meshes/triangles, misses line and sphere queries, and leaves binder movement unobstructed.
2. A malformed exact resource remains absent.
3. A valid explicitly supplied KCL becomes queryable only after explicit registration and `build()`.

Run with:

```text
xmake run smg-pc-stage-collision-registration-tests
```

See `verification.log` for captured build/test output.
