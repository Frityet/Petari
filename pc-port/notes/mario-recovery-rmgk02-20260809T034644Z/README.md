# MarioRecovery RMGK02 reconstruction evidence

Captured 2026-08-09 from the retail RMGK02 split object and the reconstructed
source object.

## Scope

- `src/Game/Player/MarioRecovery.cpp`
- `include/Game/Player/MarioRecovery.hpp`
- The declaration-only correction `Mario::doRecovery(): void -> bool` in
  `include/Game/Player/Mario.hpp`

No PC activation, fallback implementation, configuration change, or protected
file edit is part of this reconstruction.

## Assembly-proven declarations and layout

The retail function returns `false` from five guard paths and `true` after
starting either recovery mode, so `Mario::doRecovery()` is `bool`, not `void`.
The `MarioRecovery` virtuals `start`, `update`, and `close` also return `bool`.

The recovered state layout spans offsets `0x11..0x89`: two booleans, five
`u16` state/timer fields, eight `TVec3f` fields, and three `f32` fields. The
declared offsets are recorded directly in `MarioRecovery.hpp`.

## Recovered behavior

All eight retail code symbols are present: `Mario::doRecovery`, the
`MarioRecovery` constructor, `calcFirstVector`, `updateJump`, `start`, `update`,
`close`, and the translation-unit static initializer. The implementation
reconstructs the status guards, warp fallback, last-safe-floor target,
gravity-projected curved return path, bounded movement, shadow-plane landing
detection, special pre-position phase, camera/effect/sound calls, and movement
state restoration.

## Focused object comparison

Generated with:

```sh
ninja -j12 build/RMGK02/src/Game/Player/MarioRecovery.o
build/tools/objdiff-cli diff \
  -1 build/RMGK02/obj/Game/Player/MarioRecovery.o \
  -2 build/RMGK02/src/Game/Player/MarioRecovery.o \
  -o pc-port/notes/mario-recovery-rmgk02-20260809T034644Z/objdiff-final.json
```

Section results:

- `.text`: 99.01377% over 3,776 target bytes
- `.ctors`: 100%
- `.sdata2`: 100%
- `.data`: 43.90244%

Function results:

- `Mario::doRecovery`: 99.9375%
- `MarioRecovery::MarioRecovery`: 100%
- `MarioRecovery::calcFirstVector`: 99.546394%
- `MarioRecovery::updateJump`: 99.11377%
- `MarioRecovery::start`: 99.51219%
- `MarioRecovery::update`: 99.173225%
- `MarioRecovery::close`: 91.76471%
- `__sinit_\\MarioRecovery_cpp`: 100%
- `MarioRecovery` vtable: 100%

The low `.data` score is isolated from the matched executable behavior. The
retail split contains an unused pooled `PullBack` string plus trailing split
data; no artificial source reference was retained merely to force that pool.

The raw comparison is in `objdiff-final.json`. `target-disassembly.txt` and
`source-disassembly.txt` preserve both object disassemblies.

## Full-build verification

`ninja -j12` completed successfully, including the repository's
`CHECK config/RMGK02/build.sha1` step (`build/RMGK02/main.dol: OK`). An
independent SHA-256 comparison also matched:

```text
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf  build/RMGK02/main.dol
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf  orig/RMGK02/sys/main.dol
```

`git diff --check` passed for the two dedicated files and the shared
`Mario.hpp` declaration. This evidence directory is intentionally ignored by
`pc-port/.gitignore` (`notes/`) and therefore requires an explicit force-add if
it is to be committed.
