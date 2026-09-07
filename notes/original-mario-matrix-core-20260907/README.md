# Original Mario matrix core recovery — 2026-09-07

Recovered the four missing reference methods in `decomp/src/Game/Player/Mario.cpp` from the original Korean Wii `Mario.o`, following `decomp/AGENT_DECOMP_GUIDE.md`. This is reference source recovery; the parent task owns the later complete native Mario source replacement and runtime validation.

| Method | Retail bytes | Recovered bytes | Objdiff match |
| --- | ---: | ---: | ---: |
| `createDirectionMtx` | 388 | 388 | 99.793816% |
| `createCorrectionMtx` | 668 | 668 | 99.940120% |
| `createAngleMtx` | 832 | 808 | 96.716350% |
| `fixHeadFrontVecByGravity` | 1828 | 1812 | 98.971550% |

The final complete reference TU compiles with the original MW compiler and ordinary reference include paths, without the temporary overlay. A fresh compile of the immediate pre-append source gives 46 paired existing functions; none changed match score. The four recovered methods retain all 157 direct calls in the original order. The remaining score differences are instruction/register/temporary selection; the two larger methods do not claim an exact binary match.

## Source and behavior

- Added the actual `HitSensor` declaration include needed to read sensor positions; removed the old commented partial direction-matrix draft. Existing input and grounding recoveries were preserved by appending method spans to the current reference file.
- `createDirectionMtx` reconstructs axes using the original side/up/front cross-product order, normalization choices and zero-vector fallback, then clears matrix translation.
- `createCorrectionMtx` preserves the two independent easing counters, original camera-side sign test and Talk exclusion. Counter decay multiplies before division. Its reference declaration is corrected from `void` to `bool`: retail explicitly loads `r3=0` on the non-fixed-head identity return and `r3=1` at the common exit. The recovered `bool` function has the exact 668-byte size and a 99.94% score. `checkForceGrounding` was already `void` and was left unchanged.
- `createAngleMtx` uses the original general target sensor precedence (ordinary, throwing animation, punching), direction and posture matrix ordering, local yaw constraint, matrix pair and timer decay. The second `acos`, sign calculations and eventual zero roll are retained because they are present in retail, even though the pitch result is discarded. The animation literal `投げ` is from the original data at `0x805B79C0`.
- `fixHeadFrontVecByGravity` preserves original stick/camera flags, gravity-dependent turn rates, ground/air side-axis continuity, early normalization failures, front/head margin tests and saved-surface rotation. Fields and Mario constant members are typed; no substitute axes, actor-specific native checks or additional null guards were introduced.

## Evidence and reproduction

- Retail object: `decomp/build/original-player-state-recovery-20260907/retail/obj/Game/Player/Mario.o`.
- Retail DOL SHA-1: `25c5959534b3c21246c6c7e42021b916b41fb578`.
- `methods.cpp` is the exact four-method source payload. The four `*.retail.s` files retain the original split disassembly.
- `reference-compile-command.json` and `reference-objdiff-command.json` record the successful final commands; run them from `decomp/`. `reference-function-proof.json` records the table above. `Mario.reference.objdiff.json` and `Mario.reference.o` are the local detailed comparison artifacts.
- `before-append-compile-command.json`, `before-append-objdiff-command.json` and `baseline-comparison.json` record the fresh baseline check. Section-data scores change as the new methods introduce constant and string uses; the 46 pre-existing function scores are unchanged.
- `call-proof.json` records matching direct-call sequences (80, 23, 20 and 34 calls respectively for gravity/head, direction, correction and angle).
- `probe.py`, `Mario.baseline.cpp`, `Mario.candidate.cpp` and the overlay preserve the initial isolated recovery workflow. `methods.first.cpp` is the historical lower-score angle candidate, not the final implementation.

No root Xmake build, GPU run, native source mutation, staging or commit was performed for this reference-only tranche. Passing compilation and the retail comparison do not establish complete movement or camera behavior on PC.
