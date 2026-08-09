# MarioMove25D RMGK02 recovery

Timestamp: `2026-08-09T02:55:42Z`

## Outcome

`src/Game/Player/MarioMove25D.cpp` now reconstructs the complete retail
translation unit: pipe-area basis setup, gravity-relative 2.5D mode selection
and hysteresis, movement-axis reconstruction, constrained input projection, and
the MarioActor nerve initializer.

No PC activation, compatibility, factory, audio, protected `SaveIcon` /
`TriggerChecker`, or configuration source was changed.

## Retail behavior recovered

- `Mario::set25Dmode` rotates the pipe-local Z/Y axes from the containing area,
  derives the perpendicular axis, normalizes it, and activates movement state
  `_3A`.
- `Mario::update25Dmode` projects inverse gravity away from the pipe axis,
  classifies its dominant signed basis direction, applies the retail
  `0.965`/`0.9397` side-mode hysteresis, and changes the mode only after the
  saved stick direction turns by at least `pi / 4`.
- `Mario::updateAxisFromMode` handles parallel and nonparallel gravity/pipe-up
  cases, preserves the retail sign rules for modes 0 through 3, and writes the
  constrained movement axes.
- `Mario::calcMoveDir25D` is exact and computes
  `_6B0 * stickX - _6BC * stickY`.

## ABI correction

Retail instructions pass `this + 0x6C8` to both
`JGeometry::TVec2<f>::set(float, float)` and
`MR::diffAngleAbs(const TVec2f&, const TVec2f&)` (notably at
`0x802ED970..0x802ED978` and `0x802ED994..0x802ED9A8`). The two stale scalar
members at offsets `0x6C8` and `0x6CC` were therefore corrected to one
size-neutral `TVec2f _6C8` member. `MarioInit.cpp` now initializes it with
`_6C8.zero()`; its focused retail object remains 100% matched in `.text` and
`.sdata2`.

## Authoritative evidence

- Retail assembly: `build/RMGK02/asm/Game/Player/MarioMove25D.s`
- Retail address range: `0x802ED6B0..0x802EDE68`
- Retail `.text`: 1,976 bytes
- Retail data: 4-byte `.ctors` plus 32-byte `.sdata2`
- Focused source object: `build/RMGK02/src/Game/Player/MarioMove25D.o`
- Objdiff unit: `main/Game/Player/MarioMove25D`

The retail assembly determined every branch, call, member offset, flag, mode,
constant, sign rule, and vector operation. A temporary m2c pass was used only
to label stack regions and broad control flow; its inferred types and call
signatures were not used without assembly/header confirmation.

## Match and integrity

The freshly generated project report records **99.79757% fuzzy `.text` match**.
Four of five functions are 100% fuzzy matched; `update25Dmode` is 99.35897%.
The stricter focused one-shot diff records 99.64575% for `.text`, 100% for
`.ctors`, and 100% for `.sdata2`. The remaining source-object difference is one
functionally redundant retail range-check branch plus constant-pool relocation
identity; no retail behavior is intentionally omitted.

The complete RMGK02 build succeeds and the reconstructed DOL passes its retail
SHA-1 manifest. Rebuilt and original DOL SHA-256 values are identical:

`8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`

See `objdiff-summary.txt` and `verification.log` for focused metrics and exact
commands.
