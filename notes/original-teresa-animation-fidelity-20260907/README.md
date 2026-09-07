# Original Teresa animation fidelity, 2026-09-07

This separate checkpoint follows motion recovery `8f6d3b49d`. It changes only
four `MarioActor` method bodies in decomp `MarioTeresa.cpp`, using the independent
retail instruction/data review preserved here. Source-span comparison confirms
all other methods, table definitions, headers and native source remain unchanged.

The recovered bodies preserve independent sleep-wake probes, branch-local and
repeated virtual `getLastMove` calls, gravity-before-move evaluation, owner-field
reloads across callbacks, and the intermediate `_9B0` store before clamping.
The original decisions, constants and tables were already present. These changes
restore call and evaluation semantics; they are not evidence of a demonstrated
ordinary-gameplay animation failure in the current pure getter implementations.

Verified literals replace the historical `cTeresa*` global-pointer loads inside
these four methods. Retail uses direct string addresses, and its duplicate basic
and run strings have distinct addresses. Name/hash lookups and string comparison
do not require pointer identity. Existing table definitions remain intact.

Fresh full Wii compilation and comparison against the verified RMGK01 retail
objects pass. The baseline uses the same `MarioTeresa.cpp` basename and current
headers to preserve static-initializer symbol identity.

| Method | Before | After | Retail / candidate bytes |
| --- | ---: | ---: | ---: |
| initTeresaMarioAnimation | 73.33% | 99.35% | 240 / 240 |
| runTeresaBaseAnimation | 52.38% | 99.26% | 136 / 136 |
| changeTeresaAnimation | 76.59% | 99.41% | 264 / 264 |
| updateTeresaAnimation | 58.49% | 98.27% | 1,112 / 1,108 |

No paired function score regresses. Minor score improvements in unchanged
methods reflect pooled-string/constant relocation correspondence; source-span
comparison confirms those methods were not edited. Commands, source hashes,
baseline, independent DOL data/strings, and full object diffs are retained here.
The reviewed DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`.

The reference initializer retains its original `u32` pointer stores. Any later
native mirror must preserve the existing typed pointer-width correction; this
lane did not edit or activate native Game code. Full Teresa state ownership,
animation runtime behavior and native player movement remain separate validation.
