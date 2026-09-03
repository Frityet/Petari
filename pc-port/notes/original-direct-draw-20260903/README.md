# Original TDDraw shadow and footprint geometry

This bounded cluster supplies the actual original setup/view/projection and geometry operations needed by CollisionShadow, MarioActorShadow, and FootPrint. It uses real GX commands and the original MR camera/screen providers. No dummy draw implementation, scene recipe, or actor-specific branch is introduced.

## Root recovery and corrections

The sole root source changed is `src/Game/Util/DirectDraw.cpp`.

- Recover `drawFillBox3D` at RMGK01 `0x80403808` (828 bytes). Its four base corners are `position ± side ± depth`; the upper corners add the supplied height vector. Preserve all six original quad orders and every color. Full-TU compilation currently inlines the colored `sendPoint` helper at 21 sites where retail outlines it, producing a **66.251205%** raw fuzzy score. This is explicitly not a high-fuzzy claim. `verify-box-stream.py` symbolically evaluates both actual PPC instruction streams, including the vector constructor/add/subtract/assign operations and original sendPoint behavior: they emit **identical six quads, 24 vertex expressions and 24 colors**. The helper itself is a 100% retail match. No inline restrictions or algorithm changes were added to chase register allocation.
- Recover `fix2Dpos` at `0x804043F8` (156 bytes), **99.61539%**. Only for the original widescreen predicate, multiply x by `screenWidth / frameBufferWidth`, preserving original getter order and floating-point division. Leave y/z untouched.
- Correct the existing 2D `drawFillCircle` decompilation at `0x804028AC` (356 bytes), **99.55056%**. Retail evaluates `2 * (float(i) / segments * pi)` separately for cosine and sine. The previous source placed pi in the denominator. Preserve the original center-z snapshot before GXBegin and inclusive rim iteration.
- Correct `cameraInit2D` at `0x80403B48` (480 bytes), **91.041664%**. Retail allocates a complete 4x4 projection matrix (64 bytes from stack+0x8 to scratch at+0x48), not a 3x4 Mtx. Its three static TVec3 values use the compiler's guards; the prior explicit byte guards added a second guard layer. The new source retains the original static initialization, integer screen-size halving, near0/far-1 projection and viewport/scissor setup. The integer up-vector constructor is inlined into its three constant stores in the rebuilt object; all remaining direct calls retain their order.

## Original providers staged unchanged

`stage-native.py` extracts complete root bodies without token edits into `build/original-direct-draw-20260903/staged/compat/OriginalDirectDraw.cpp`. The 15 bodies are set/load/model/reset view matrix, close, setup, both sendPoint overloads, both filled-circle overloads, drawTexture3D, drawFillBox3D, both camera-init functions, and fix2Dpos. Native `DirectDraw.hpp` already equals root. Existing `ModelFogCompat.cpp` continues to own mixFogColor/setGXColor, so no duplicate providers are introduced. Unrelated unverified sphere and pixel-conversion routines are not imported.

Original GC3.0a3 checks: view helpers, close, sendPoint and cameraInit3D are **100%**; setup **99.92%**; 3D filled circle **99.73256%**; drawTexture3D **99.896904%**. The source correspondence JSON records each unchanged body hash. The native provider passes isolated LLVM compilation using the actual current Game target flags, with no root-header fallback or production-native edits. This is not a native link, GPU run, or playable-shadow claim.

Commands:

```
python3 pc-port/notes/original-direct-draw-20260903/verify-original.py
python3 pc-port/notes/original-direct-draw-20260903/stage-native.py
```

The verifier checks the RMGK01 DOL SHA1, actual original compilation, call sequences, recorded fuzzy bounds, and the box instruction-stream proof. Compiler objects remain in ignored build storage; the raw new/reviewed method disassemblies are included here. `native-compile.json` records the exact isolated native compile command. `native.patch` is relative to `pc-port/` and adds only the new compatibility TU. Parent owns production application, shared build/GPU gates and checkpoint.

The original mViewMtx is one retained TDDraw state object. Callers must continue to provide the real camera/view and scene GX ownership phases; importing these functions does not construct those owners or restore unrelated GX state. The static 2D values retain their original first-use lifetime, even though current projection dimensions are queried on each call.
