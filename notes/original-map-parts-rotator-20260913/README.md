# Original MapPartsRotator closure — 2026-09-13

Recovered all eight missing original methods in the reference, then copied the complete source and header into native Game. This supplies actual rotation velocity, angle/target progression, endpoint and acceleration behavior, signal-motion sequencing, and host/output matrices to the reusable MapObjActor initializer.

Retail `initRotateSpeed` passes `this + 0x18` to `getMapPartsArgRotateSpeed`, then scales that same base-speed field. The existing source accidentally wrote current velocity at `0x28`; fixed to `_18`. The two matrices are now `TPos3f`, preserving Wii storage and the original public `TMtx34f` base reference while exposing the actual SDK rotation operations. The previously recovered empty `MapPartsRotatorBase` destructor is retained unchanged.

Original `exeRotateStart` calls `MapParts::getMoveStartSignalTime()`. Root already imported that full owner; no extracted helper, duplicate duration, factory workaround, or altered gameplay sequence was introduced.

## Necessary validation

Original Metrowerks compiler and isolated native LLVM compilation both pass. Native and reference source/header pairs are byte-identical. Exact commands are in `reference-compile.json` and `native-compile.json`; retail comparison in `object-diff.json` with concise scores in `method-matches.json`.

New recovered methods: base matrix 99.74%, angle 99.35%, target 92.83%, reached-target predicate 94.03%, rotate nerve 92.60%, signal nerve 99.69%. Velocity is 86.91%: the emitted condition chain has the same branches and floating-point operations, but folds explicit boolean materialization and changes registers. Rotation matrix is 88.66%: the shared original SDK axis-rotation helper emits the same products, additions, normalization and concatenation with a different register/scheduling pattern. The existing axis-column helper is 63.80% because native reference headers inline `TVec3::set` instead of the retail tail-call; its selected column and component stores are unchanged. These lower scores are functional comparisons, not matching claims. Corrected speed initializer is 99.76%. No additional matching polish or tests were performed.

This is source and compile closure, not evidence that the original Gateway sequence has run. Parent owns the next integrated build/run, staging and publication. No shared build files or index changes were made here.

Exact four production/reference paths and their frozen hashes are in `source-manifest.json`. Other map-part owners and utility/posture files remain in their separately frozen cohorts.
