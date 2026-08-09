# MarioAnimationEfx RMGK02 recovery

Date: 2026-08-09 UTC

## Outcome

`src/Game/Player/MarioAnimationEfx.cpp` is restored as a complete, assembly-validated translation unit. It implements the animation callback table and every retail callback used for spin, stage entry, throws, squat-spin cancellation, and walk-in/result cleanup.

Repository history contained a prior reconstruction in `f645080a0`; this lane recovered that source, recompiled it with the configured `GC/3.0a3` compiler, and checked it against the current RMGK02 object rather than treating history as authoritative by itself.

## ABI correction

`MarioAnimator.hpp` now exposes the retail callback members at their proven offsets:

- `_10F` callback-running flag
- `_11C` signed callback-table index
- `_120` `HashSortTable*`
- the four callback methods previously missing from the declaration surface

`MarioAnimatorData.hpp` now declares `luigiAnimeSwapTable` as `extern`; its sole definition belongs to this retail unit. This prevents a duplicate definition and restores the original object ownership.

## Fidelity

- project report `.text`: 2,000 retail bytes, 99.212% fuzzy
- focused objdiff `.text`: 98.912%
- `.ctors`: 100%
- `.sdata2`: 100%
- all 14 retail functions are implemented; `initCallbackTable`, `runningCallback`, `walkinClose`, and the generated initializer are 100%
- remaining per-function scores are 91.90% to 99.79% and reflect register/branch/relocation shape, not omitted callbacks

The raw `.data` split includes adjacent string/table ownership and relocation pairing, so its aggregate percentage is not used as a behavior-completion claim. Both required callback and Luigi animation-swap tables are emitted with the retail entries.

## Integrity

- focused `MarioAnimationEfx` and dependent `MarioAnimator` objects compile
- full RMGK02 build succeeds
- `build/tools/dtk shasum -c config/RMGK02/build.sha1` passes
- rebuilt DOL SHA-256 equals retail: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`
- no PC activation, factory route, fallback, audio, protected, or configuration file was added
