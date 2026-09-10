# Duplicate Player accessor removal — 2026-09-10

Removed 15 duplicate definitions from two compatibility slices. The original `MarioAccess.cpp` and `MarioActorGravity.cpp` already belong to the normal Game archive; only `Player/MarioSound.cpp` and `Player/MarioState.cpp` remain excluded from that directory. No Game source, reference source, build configuration, or fixture was changed.

## Exact removed providers

From `src/compat/MarioCameraAccessCompat.cpp`, now provided by `src/Game/Player/MarioAccess.cpp`:

- `MarioAccess::isOnGround(u32)`
- `MarioAccess::isInRush()`
- `MarioAccess::isFlying()`
- `MarioAccess::getCameraCubeCode()`
- `MarioAccess::isSwimming()`
- `MarioAccess::getGroundingPolygon(u32)`
- `MarioAccess::getLastMove()`
- `MarioAccess::getBaseMtx()`
- `MarioAccess::isOnWaterSurface()`
- `MarioAccess::getPlayerActor()`
- `MarioAccess::isInWaterMode()`
- `MarioAccess::isOnActor(const LiveActor*)`

From that same compat file, now provided by `src/Game/Player/MarioActorGravity.cpp`:

- `MarioActor::getGravityInfo() const`

From `src/compat/MarioStateAccessCompat.cpp`, now provided by `src/Game/Player/MarioActorGravity.cpp`:

- `MarioActor::getGravityVec() const`
- `MarioActor::getGravityVector(TVec3f*) const` (the pointer-output overload)

All 15 removed bodies were compared against their original source provider with whitespace ignored and were identical. Reading the existing debug `libsmg-pc-game.a` with `llvm-nm -A -C --defined-only` also confirmed strong definitions of every removed symbol in the corresponding original object, as well as the old compat duplicates. This is evidence about the archive before rebuilding; the post-cleanup single-provider check belongs to root's linked validation.

## Preserved surface and fixture considerations

The ten MR wrappers in MarioCameraAccessCompat remain byte-identical, including `isActorOnPlayer` and `isOnPlayer`. `Mario::getCurrentStatus` and `Mario::isStatusActive` also remain byte-identical because full MarioState is still excluded. Removed unused includes and replaced the stale comment claiming the original accessor TUs were unavailable.

Current production/test build files do not compile either compat slice as an independent source list: the files enter through the Game archive's compat glob. The camera-director, player-actor-bridge, and Mario Gateway walk targets all depend on that archive. There is therefore no identified fixture that requires adding the original accessor TUs to its own file list. Extracting the original archive objects can expose previously hidden dependency or duplicate-provider issues at link time; root will link the showcase and relevant existing fixtures. The older Gateway walk harness has separate pre-existing lifecycle assumptions and is not new runtime evidence for this removal.

`git diff --check` passed. No compilation, Xmake invocation, runtime, or commit was performed for this cleanup.

Frozen SHA-256:

- `src/compat/MarioCameraAccessCompat.cpp`: `113bf3b0830cb8314f6fb57bb782d1b14295166f85dbfb7f736231f7d8d13c9a`
- `src/compat/MarioStateAccessCompat.cpp`: `e07d273a2e00f1acffb4f2e372d759e7687e96d2049e5a2f86dce10d98efdb01`
