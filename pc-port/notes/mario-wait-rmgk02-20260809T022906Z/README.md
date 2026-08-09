# MarioWait RMGK02 recovery

Timestamp: `2026-08-09T02:29:06Z`

This tranche reconstructs the root `Game/Player/MarioWait.cpp` unit needed by the grounded idle/walking path. The implementation was recovered from the RMGK02 retail assembly, supported by headless Ghidra pseudocode, and iterated against the relocatable object with `objdiff-cli`.

## Recovered behavior

- `MarioAnimator::controlWaitAnimation` selects and blends the four slope-idle tracks from Mario's front, floor-normal, and air-gravity vectors. The retail 45-degree cap and four-frame retarget cooldown are preserved.
- `MarioAnimator::stopWaitAnimation` returns the blend track to the run animation after its cooldown, except for the high-speed Bee-mode path.
- `MarioWait::checkStart` implements the 1,800-frame sleep-entry gate, including all input, status, motion, visibility, water, and `NonSleepCube` exclusions.
- `MarioWait::start`, `update`, and `close` select the two special wait variants, transition into the combat wait (210 frames for Boo Mario, 1,800 otherwise), resume normal movement on input, and stop all three sleep voice sounds on exit.
- `Mario::isBlendWaitGround`, `checkSpecialWaitAnimation`, and `resetSleepTimer` restore the player-facing integration points.

## ABI evidence

- The Mario constructor allocates `MarioWait` with size `0x18`; the stale `_18[3]` tail in `MarioWait.hpp` was therefore removed.
- The retail vtable proves `start`, `close`, and `update` overrides.
- `Mario::checkSpecialWaitAnimation` at `0x802CEE2C` is a `void` function; its only RMGK02 caller ignores a result. The prior `bool` declaration was corrected.
- Retail definitions at `0x802CE398` and `0x802CE6A0` prove the two missing `MarioAnimator` declarations.

## Fidelity

The complete original `.text` is `0xB90` / 2,960 bytes. Final fuzzy similarity is **98.38243%**. Six methods are 99% or better, four are exact, and the largest method is 98.12371%. `MarioWait::checkStart` compiles to the exact retail size and reaches 99.77778%; `MarioWait::update` is four bytes shorter and reaches 99.05738%.

The source is intentionally not activated in `pc-port`. This lane adds exact root decompilation only; no PC fallback, factory shortcut, or compatibility special case was introduced.

## Frozen review paths

- `src/Game/Player/MarioWait.cpp`
- `include/Game/Player/MarioWait.hpp`
- `include/Game/Player/MarioAnimator.hpp` (two declarations only)
- `include/Game/Player/Mario.hpp` (`checkSpecialWaitAnimation` return type only; the concurrent `checkWallPush` correction belongs to the MarioWalk lane)

See `verification.log` and `retail-symbols.txt` for the command-level evidence.
