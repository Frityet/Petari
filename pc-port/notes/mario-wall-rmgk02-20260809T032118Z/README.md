# MarioWall RMGK02 recovery

Timestamp: `2026-08-09T03:21:18Z`

## Outcome

`src/Game/Player/MarioWall.cpp` now reconstructs the complete 26-function
retail translation unit. It restores Mario's wall-contact gate, wall-position
and facing correction, ordinary and triangle wall jumps, back jumps, wall
sliding, wall punches, ice-wall handling, cancel state, and the four
`MarioWall` state callbacks.

No PC activation, compatibility fallback, audio implementation, protected
`SaveIcon` / `TriggerChecker`, configuration source, or build metadata was
changed.

## Retail behavior recovered

- `Mario::checkWallStick` retains the bee-mode path, wall-jump-code precedence,
  moving-surface velocity correction, and ordinary transition into
  `MarioStatus_Wall`.
- Wall eligibility retains all player-mode, movement/draw-state, animation,
  height, polygon-angle, map-code, rising, input, collision-ray, and Karikari
  cling guards from retail.
- `MarioWall::start`, `update`, and `close` preserve the landing, cancel,
  jump, rush, sliding, side-motion, ice-wall, effect, animation, and velocity
  state machine.
- Normal, triangle, and back wall jumps retain the retail impulses, timing,
  effect/sound selection, and state-bit updates. The triangle-jump rotation
  calls the retail out-of-line `JMAAcosRadian` symbol.
- Wall position/facing/top correction and wall-punch dispatch preserve the
  original front/back/side collision triangles and swimming behavior.

## ABI evidence

Retail call sites and return-value tests prove the shared `Mario.hpp`
corrections for `checkStickWallSide`, `fixWallingPosition`, `fixWallingDir`,
`fixWallingTop`, `checkWallFloorCode`, `checkWallCodeNorm`, `fixWallingDist`,
and `tryWallPunch`. The `MarioWall` vtable at retail `0x805CB9E8` proves the
`start`, `close`, `update`, and `notice` overrides. Retail calls prove the
additional `initTriangleJump`, `isCancel`, and `startBackJump` declarations.

The shared declarations remain size-neutral. Existing concurrent `Mario.hpp`
corrections, including `doSideStep(): bool` and the `_574` / `_6C8` layout
fixes, were preserved.

## Authoritative evidence

- Retail assembly: `build/RMGK02/asm/Game/Player/MarioWall.s`
- Retail address range: `0x802F5760..0x802F7BD0`
- Retail `.text`: 9,328 bytes (`0x2470`)
- Retail functions: 26
- Retail sections: 4-byte `.ctors`, 648-byte `.data`, 80-byte `.sdata2`
- Focused source object: `build/RMGK02/src/Game/Player/MarioWall.o`
- Objdiff unit: `main/Game/Player/MarioWall`

The assembly determined the branch ordering, bitfields, member offsets,
constants, collision triangles, strings, virtual calls, return values, and
state transitions. A temporary m2c pass was used only to label stack storage
and broad control flow; types and calls were checked against assembly and
repository headers.

## Match and integrity

The freshly generated project report records **93.43654% fuzzy `.text`
match**. All 26 retail functions are present, 13 are 100% fuzzy matched, and
24 are above 90%. The focused one-shot comparison records **93.25643% `.text`**
with a 9,120-byte candidate. `.ctors` and `.sdata2` are both 100% in the
focused target-side report.

The source emits the 496 bytes of strings and vtable referenced by recovered
Wall code. The retail split attributes a further 152-byte adjacent retained
string blob to this unit; no artificial filler or unreferenced payload was
added to inflate the data score.

The complete RMGK02 build succeeds and the rebuilt DOL passes its retail SHA-1
manifest. Rebuilt and original DOL SHA-256 values are identical:

`8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`

See `objdiff-summary.txt` and `verification.log` for the focused metrics and
exact verification commands.
