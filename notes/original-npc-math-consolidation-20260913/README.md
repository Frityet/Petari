# Original shared NPC math, 2026-09-13

Deleted the entire 190-line `NPCActorRuntimeCompat.cpp`. NPC callers now use the existing shared provider files; no native Game source or build-list changes are required.

- `GameMathCompat.cpp` imports the existing original `makeQuatRotateRadian`, `makeQuatRotateDegree`, `makeAxisFrontUp`, `isSameDirection`, `isOppositeDirection`, `clampVecAngleDeg`, and `turnQuatYDirRad`. The Y-direction turn delegates to the already original shared `turnQuat`. Both vector `rotateVecDegree` overloads now also use their original matrix implementation.
- `MtxCompat.cpp`, `GameGravityCompat.cpp`, and `OriginalActorModelAccess.cpp` receive the original matrix translation, actor gravity, and random Bck-frame helpers.
- The unchanged native CSV-shadow entry point moved beside its existing owner in `ActorShadowCsvCompat.cpp`. Its underlying CSV compatibility implementation was not recovered or replaced in this batch.
- The missing SDK quaternion Euler setter was recovered first in `decomp/src/Game/Animation/AnmPlayer.cpp`, where retail owns the symbol, then copied into the existing native `TQuat.cpp`. Three small native template declarations/inlines expose the existing original SDK API. Native Game remains unchanged.

This removes invented finite/zero-vector exceptions, fallback axes, additional quaternion normalization, vector-length preservation during clamping, and the separate turn-completion rule. The original turn uses its own opposite-direction perturbation and completion threshold of 0.015 radians; the original clamp returns when its rotation axis normalizes to zero.

## Evidence and scope

All six changed native translation units compile with the existing LLVM 23 project flags (`native-proof.json`, individual logs). Existing unrelated header warnings remain in `OriginalActorModelAccess.cpp`. No component tests or shared builds were run in this batch; root owns the next production build/run.

The required reference Metrowerks compilation passes into this notes directory (`reference-compile.json`). `setEuler__Q29JGeometry9TQuat4<f>Ffff`, retail address 0x800161D0, is 324 bytes in both objects and scores 89.604935% in `setEuler.objdiff.json`. The decoded retail sequence performs float-rounded half angles, six double-precision cosine/sine calls with float-rounded results, then the same ordered products and sums used in the recovery. There are no branches or normalization step. Remaining differences are register/store scheduling; this is source/retail branch and arithmetic review, not a native bit-exact oracle claim. Source was frozen after the first successful compiler check.

`source-equivalence.json` records whitespace-normalized equality for the imported existing original bodies and the reused shared turn. `source-manifest.json` contains the exact frozen production paths/hashes. The `before/` snapshot preserves the starting state. Shared `GameMathCompat.cpp` already contained other root work; that starting content is preserved.
