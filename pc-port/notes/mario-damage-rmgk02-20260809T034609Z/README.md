# MarioDamage RMGK02 reconstruction evidence

## Scope

- Reconstructed the complete 56-function `src/Game/Player/MarioDamage.cpp` unit from `build/RMGK02/asm/Game/Player/MarioDamage.s`.
- Recovered the assembly-proven state layouts and virtual/helper declarations in the six dedicated damage headers.
- Corrected only the ten assembly-proven `void` to `bool` declarations in `Mario.hpp`.
- Did not alter decomp activation, PC factories/compatibility code, protected files, staging, or commits.

## Focused verification

- MWCC object build: pass (`build/RMGK02/src/Game/Player/MarioDamage.o`).
- Object SHA-256: `b19d03fcbcde47f66d3fddce7ed6d9ba04313f8230f711a03e578621a546b6fd`.
- Target `.text`: 12,756 bytes; objdiff fuzzy match: **98.96457%**.
- 56 functions recovered; mean function match: **99.16357%**.
- 54/56 functions are at least 95%; 47/56 are at least 99%; 28/56 are exactly 100%.
- `.ctors`: 100%; `.sdata2`: 100%; all six vtables: 100%.
- `.data`: 52.427185%; see the provider note below.
- The final section/function summary is reproduced below and in
  `verification.md`.

The focused compiler emitted only the two pre-existing `MarioActor.hpp` warnings about nontrivial union members; there were no errors.

## Full-DOL verification

- `build/RMGK02/main.dol` SHA-256: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.
- `orig/RMGK02/sys/main.dol` SHA-256: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.
- Byte comparison: identical.
- Owned-path whitespace/diff check: clean.

The DOL check verifies that the overall RMGK02 baseline remains exact. This task intentionally did not activate the newly reconstructed object in `configure.py` or in the PC port.

## Cross-unit string provider implication

The target `MarioDamage` object gets these four animation-name pointers from global data label `lbl_80539AC0` in the target `MarioEffect` object:

- `中ダメージ空中`
- `中ダメージ着地`
- `中後ダメージ空中`
- `中後ダメージ着地`

The current source uses equivalent local string literals so the reconstructed object remains self-contained and linkable without modifying `MarioEffect.cpp`. That duplicates 64 bytes locally and shifts subsequent string data, explaining the low `.data` fuzzy score despite the 98.96% text result and 100% vtables. Exact data/relocation recovery later requires a source-side `MarioEffect` provider for `lbl_80539AC0` (or an ABI-equivalent shared string table), followed by replacing the four local literals with offsets into that provider.

## Supporting artifacts

`verification.md` records the exact final commands and summarized results.
The multi-megabyte raw objdiff JSON and temporary Ghidra output remain local
ignored work products rather than repository artifacts.
