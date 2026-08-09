# MarioSideStep RMGK02 recovery

Timestamp: `2026-08-09T03:16:48Z`

## Outcome

`src/Game/Player/MarioSideStep.cpp` now reconstructs the complete seven-function
retail translation unit. It restores wall-jump floor-code dispatch, SideStep
entry validation, animation and BAS lifecycle, gravity-relative wall movement,
jump/rush/drop exits, and the MarioActor nerve initializer.

No PC activation, compatibility fallback, audio implementation, protected
`SaveIcon` / `TriggerChecker`, or configuration source was changed.

## Retail behavior recovered

- `Mario::checkWallJumpCode` recognizes wall floor codes 6, 7, 8, 11, and 34;
  stops walking when required; applies the retail wall/gravity jump impulse;
  initializes the triangle jump; and selects normal, back, or special jump
  types with the original precedence.
- `Mario::doSideStep` rejects incompatible status, wall-code, actor, draw,
  player-mode, polygon-angle, wall-angle, floor-code, contact-state, and moving
  wall conditions before entering `MarioStatus_SideStep`.
- `MarioSideStep::start` / `close` restore the wall-push sound, wall animation
  blend, animation cleanup, and BAS reset.
- `MarioSideStep::update` projects input into the gravity plane, removes the
  wall-normal component, selects push/idle/left/right animation, applies the
  six-unit tangential and wall-hugging velocity, and preserves the retail jump,
  rush, drop, and wall-position exit paths.

## ABI evidence

The SideStep vtable at retail `0x805C92AC` contains `start`, `close`, and
`update` immediately after the inherited `proc` slot, proving the three
overrides added to `MarioSideStep.hpp`.

`Mario::doSideStep` at `0x802F1588..0x802F177C` returns explicit zero and one
values, proving the size-neutral `void` to `bool` declaration correction in
`Mario.hpp`. The same retail unit compares the return from
`Mario::checkWallFloorCode(u16) const` repeatedly and consumes the return from
`Mario::fixWallingPosition(bool)`; those shared declarations were coordinated
with and frozen by the concurrent MarioWall recovery.

The retail truth-normalization sequences after `MarioWall::startBackJump` and
`Mario::fixWallingPosition` are reproduced by ordinary C++ conditional and
double-negation expressions. Their declarations correctly remain `bool`; no
integer-return ABI workaround is present.

## Authoritative evidence

- Retail assembly: `build/RMGK02/asm/Game/Player/MarioSideStep.s`
- Retail address range: `0x802F12C8..0x802F1C44`
- Retail `.text`: 2,428 bytes
- Retail functions: 7
- Retail data: 4-byte `.ctors`, 600-byte `.data`, and 48-byte `.sdata2`
- Focused source object: `build/RMGK02/src/Game/Player/MarioSideStep.o`
- Objdiff unit: `main/Game/Player/MarioSideStep`

The retail assembly determined all guards, branch ordering, member offsets,
bitfields, floor codes, animation strings, constants, vector operations,
virtual calls, and return behavior. A temporary m2c pass was used only to label
stack regions and broad control flow; all types and call signatures were
confirmed against assembly and repository headers.

## Match and integrity

The freshly generated project report records **99.96375% fuzzy `.text`
match**. Four functions are 100% fuzzy matched; the remaining three are at
99.89189% or better. The stricter focused one-shot comparison records
**99.691925% `.text`**, with every function at its exact retail size and both
`.ctors` and `.sdata2` at 100%.

The retail split attributes 460 bytes after the SideStep strings/vtable to this
unit (a 164-byte gap and a 296-byte adjacent animation-string blob), although
none of the seven SideStep functions references it. The source object therefore
has 300 bytes of meaningful `.data` versus the split's 600 bytes; no filler or
unreferenced blob was fabricated to inflate the data score.

The complete RMGK02 build succeeds and the reconstructed DOL passes its retail
SHA-1 manifest. Rebuilt and original DOL SHA-256 values are identical:

`8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`

See `objdiff-summary.txt` and `verification.log` for focused metrics and exact
commands.
