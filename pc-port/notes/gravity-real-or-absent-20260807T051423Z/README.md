# Gravity: real manager semantics or explicit absence

## Result

The host gravity boundary no longer treats a missing gravity system as an empty gravity manager. A query or registration without a scene-owned `StageGravityService` now throws `std::logic_error` and leaves caller output untouched.

When a service exists, it implements the retail `PlanetGravityManager` rules:

- `PlanetGravity` registration, descending priority ordering, and the retail capacity of 128
- activation, follower validity, appearance, type-mask, and requester-host filtering
- equal-highest-priority vector combination and final normalization
- `GravityInfo` selection by strongest contributing field
- real gravity power inspection through `MR::isLightGravity`
- exact `Range`, `Distant`, `Priority`, `Gravity_id`, `Gravity_type`, `Power`, and `Inverse` JMap parsing

Point and parallel sphere/box/cylinder placements construct real `PlanetGravity`-derived fields with the corresponding original formulas. The six remaining retail gravity classes (cube, cone, disk, disk-torus, segment, and wire) are rejected by name with an explicit exception until their real implementations are linked; no approximate field is installed.

Even for a supported geometry class, placements using stage switches, sleep switches, or a base-matrix follower are explicitly rejected until the real `GlobalGravityObj` lifecycle is linked. A static field is never substituted for dynamic gravity data.

## Game-source boundary

`pc-port/src/Game/Util/GravityUtil.cpp` and `.hpp` remain byte-identical to the root decompilation. They remain excluded from the host build because the full `PlanetGravityManager`/scene-object closure and its Wii-width requester pointer ABI are not yet portable. The newly added `Game/Gravity/PlanetGravity.hpp` and `GravityInfo.hpp` are byte-identical copies of the root headers. Portable behavior is generalized in `src/scene` and `src/compat`; no stage, route, or actor-name workaround was added to `src/Game`.

## Removed fallback behavior

- no silent `registerGravity` no-op
- no silent JMap setter no-op
- no constant-false `isLightGravity`
- no zero vector written merely because the scene service is absent
- no null actor interpreted as zero gravity
- no active scene gravity owner silently replaced by another
- no unknown gravity class approximated as point or parallel gravity

An active, real manager with no applicable field still returns `false` and writes zero. That is original manager behavior and is tested separately from service absence.

## Verification

- `cmp` against root: both added `Game/Gravity` headers exact
- `g++ -fsyntax-only tests/GravityRealOrAbsentTests.cpp`: passed
- `xmake -vD smg-pc-aurora-native-tests`: linked successfully after the gravity changes
- pre-existing Aurora-native binary: 25/26 tests passed before updating its obsolete missing-service expectation; the sole failure was the newly intentional explicit-unavailable exception
- isolated focused runtime linked against the last known-green game archive: 4/4 passed
- updated aggregate test translation unit: compiled successfully
- normal focused xmake target: temporarily blocked by another agent's in-flight exact `GameDataHolder` header migration versus the old `UserFile.cpp`; this is unrelated to gravity

## Focused coverage

`GravityRealOrAbsentTests.cpp` covers:

1. explicit service absence and untouched destination data
2. real empty-manager false/zero behavior
3. priority combination, type filtering, host filtering, `GravityInfo`, and light power
4. duplicate/null registration rejection
5. complete JMap parameter application
6. real point placement construction
7. explicit rejection of dynamic switch/follower placement data
8. explicit rejection of an unimplemented cube gravity
9. exclusive scene ownership
