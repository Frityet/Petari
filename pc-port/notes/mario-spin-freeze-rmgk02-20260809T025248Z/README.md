# MarioSpin and MarioDamageFreeze RMGK02 recovery

Timestamp: `2026-08-09T02:52:48Z`

## Outcome

This tranche reconstructs the complete root `Game/Player/MarioSpin.cpp` and
`Game/Player/MarioDamageFreeze.cpp` retail translation units. It also recovers
the existing `MarioFreeze` state declaration and corrects the one shared task
prototype proven by the retail callback ABI.

No PC activation, compatibility path, factory route, staging, audio subsystem,
SaveIcon, or TriggerChecker file was changed.

## Recovered spin behavior

- Tornado-state clearing and reset, including the spin timer, tilt, angle, and
  jump-cancel state.
- Stick/player-mode-aware tornado tilt blending using the retail Mario constant
  table, normalized head/pad vectors, and the near-facing blend accumulator.
- Spinning wall reflection, rebound timer/front-vector update, sensor-type `0x55`
  suppression, wall sound/effect emission, and forced tornado termination.
- Rotation task registration and the `ヘリコプタージャンプ` rise/fall angle
  update using `mTrampleBegomaRotRise` and `mTrampleBegomaRotFall`.

## Recovered freeze behavior

- `Mario::doFreeze` guards `_1F`, invincibility, the current Freeze state, and
  the close cooldown before resetting damage/player state and entering Freeze.
- `MarioFreeze` start/close/notice/update behavior: animation, damage sounds,
  vibration, life decrement, freeze-model visibility, spin escape, timed thaw,
  grounded/airborne game-over routing, gravity/jump-vector bounce behavior, and
  the 120-frame re-entry cooldown.
- The retail `Mario::setJumpVec` implementation owned by this unit.

## ABI and layout evidence

- RMGK02 `Mario::taskOnRotation(u32)` at `0x802F4204` returns `0` when the
  rotation animation is absent and `1` otherwise. `Mario::Task` is
  `bool (Mario::*)(u32)`, and `startRotationTask` passes this member pointer to
  `pushTask`. The stale declaration in `include/Game/Player/Mario.hpp` was
  therefore corrected from `void` to `bool` without changing the mangled name.
- `MarioFreeze` overrides `start`, `close`, `update`, and `notice`. Retail stores
  prove `u8` at `0x11`, `f32` at `0x14`, and `u16` fields at `0x18`, `0x1A`, and
  `0x1C`; the class size is `0x20` after natural alignment.
- Existing concurrent `Mario.hpp` corrections, including `tryCrush(): bool`,
  `checkWallPush(): void`, and `_71C: u8`, were preserved byte-for-byte.

## Authoritative evidence

- `build/RMGK02/asm/Game/Player/MarioSpin.s`
  (`0x802F3DA0..0x802F4324`, `.text` size `0x584` / 1,412 bytes)
- `build/RMGK02/asm/Game/Player/MarioDamageFreeze.s`
  (`0x802D8AB8..0x802D9134`, `.text` size `0x67C` / 1,660 bytes)
- Focused objects:
  `build/RMGK02/src/Game/Player/MarioSpin.o` and
  `build/RMGK02/src/Game/Player/MarioDamageFreeze.o`
- Objdiff units: `main/Game/Player/MarioSpin` and
  `main/Game/Player/MarioDamageFreeze`
- Full checksum manifest: `config/RMGK02/build.sha1`

Retail assembly determined every branch, call, constant-table field, bitfield,
member offset, timer, scalar, string, virtual dispatch, and return value. MWCC
and objdiff were used to verify source shape after each material adjustment.

## Secondary and ignored evidence

- An m2c pass was used only to label stack regions and confirm broad control-flow
  blocks. It inferred incomplete structures, incorrect function prototypes, and
  misleading vector-call signatures; those guesses were ignored wherever they
  were not independently proven by retail assembly and current headers.
- Repository history and public-source discovery did not contain implementations
  for either missing unit. No third-party pseudocode was treated as source.
- The retail `.data` splits contain neighboring pooled strings, padding, and weak
  Nerve artifacts not semantically owned by these functions. They were not
  recreated as fake arrays merely to inflate data-section similarity.

## Fidelity and integrity

- `MarioSpin`: 1,412-byte `.text`, **99.65156%** fuzzy match.
- `MarioDamageFreeze`: 1,660-byte `.text`, **99.821686%** fuzzy match.
- Every reconstructed function has the exact retail byte size. All residual text
  differences are register choice or relocation identity; no retail behavior is
  intentionally omitted.
- Focused MWCC compilation succeeds with only the repository's pre-existing
  `MarioActor` nontrivial-union warnings.
- The complete RMGK02 build succeeds. `build/tools/dtk shasum -c
  config/RMGK02/build.sha1` reports `build/RMGK02/main.dol: OK`.
- Rebuilt DOL SHA-1:
  `54b71431af0d509097bfdef4ec28617afc487e89`.
- Rebuilt and original DOL SHA-256 are identical:
  `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.

See `objdiff-summary.txt` and `verification.log` for exact metrics and commands.
This evidence directory is intentionally ignored by git.
