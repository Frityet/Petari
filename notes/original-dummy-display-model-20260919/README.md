# Dummy display model table and selection recovery — 2026-09-19

This recovers the original shared dummy-item model data and creation path in the decompilation. This note records decompilation proof. The later native import and bounded original-process model/retirement proof are recorded in `notes/original-crystal-cage-20260919/`; cage gameplay is not established.

## Problem and original evidence

`DummyDisplayModel.cpp` had an empty 14-row table and selected it with the fourth creation-helper parameter, `itemIdx`. That parameter is the draw-buffer category: ordinary creation passes `-1`, while crystal-item creation passes `0x21`. The authored model ID is the third constructor parameter selected from placement argument 7 (or the explicit fallback). Using draw category as the table index produced an unrelated row or an out-of-bounds access.

The verified RMGK01 DOL contains 15 rows of 28 bytes at `0x80533150`. Retail `tryCreateDummyModel` at `0x801D085C` multiplies the model ID by `0x1C`, selects that row, and uses its field at `+0x10` when the requested draw category is negative. The original `-1` no-model guard and dark-comet coin suppression remain unchanged. No new fallback, clamping, or guessed item behavior was added.

The real Gateway center `CrystalCageM` placement (`l_id72`, HeavensDoorMysteriousZone) requests model ID 3: `SuperSpinDriver`, with its `Freeze` animation. Skipping this dependency would omit the authored model displayed inside that cage. This placement evidence is recorded separately in the WarpPod audit; the recovered table supports all 15 item types.

## Changes

- Populate the 15 original table rows, including names, three-component offsets, draw categories, optional animations, and color-change flags.
- Select the table row by model ID and honor its default draw category when the caller passes a negative category.
- Replace only the table-record offset type, `TVec3f`, with the SDK aggregate `Vec`. Both occupy the original 12 bytes, but `TVec3f` construction introduced a global initializer absent from retail. `Vec` preserves literal static table data and converts through the existing `TVec3f(const Vec&)` constructor at the unchanged use site.
- Retain all existing actor methods and archive collection behavior. No shared vector implementation was changed in this recovery.

## Validation

Run from the repository root:

```sh
python3 notes/original-dummy-display-model-20260919/verify.py
```

The script uses the configured original MWCC GC/3.0a3 compiler and SJIS wrapper, the existing retail split object, and local verified retail DOL (`SHA1 25c5959534b3c21246c6c7e42021b916b41fb578`). Exact compiler arguments are in `candidate-command.json`. Large generated objects and complete objdiff output stay under ignored `build/original-dummy-display-model-20260919/`.

Result:

- All 592 instructions in the retail module assembly agree with the corresponding DOL bytes.
- All 15 compiled table rows agree with retail, including all floating-point, category, flag, null-pointer and padding bytes after pointer relocation normalization; all 19 string references resolve to the same strings.
- No global initializer is introduced.
- Recovered creation helper: **99.508194%**, original and candidate both **244 bytes**. The six remaining instruction differences consistently exchange registers 29 and 30 (model ID and table-row pointer). Substituting that register allocation makes every instruction identical; all relocations and direct calls are identical.
- Before recovery the creation helper matched **64.81967%**, and the baseline table was 14 empty rows.
- All 16 compared module functions are at least **99%**. The unchanged matrix method retains its prior **99.55639%** result; archive collection now matches **100%** with the recovered table.

`verification.json` records the table, function results, hashes, relocation evidence and the bounded claim. The intermediate attempt to initialize the existing nontrivial `TVec3f` field was rejected during verification because it emitted a new `__sinit` and moved the table to writable data; that attempt was not published.

Native import, linkage closure and actual model construction were subsequently validated separately in the CrystalCage notes; cage activation remains a gameplay validation step. No actor behavior or full Gateway progression is claimed by this decompilation proof.

Published decomp checkpoint: `d1ddacad6a59a61ae2bc26e433bc2ce9a9fbab29`; remote `pcp-decomp` SHA was verified equal.
