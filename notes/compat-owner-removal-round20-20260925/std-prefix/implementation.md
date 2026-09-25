# Dismantle the forced Metrowerks catch-all

Deleted MetrowerksStdCompat.hpp. It has no replacement catch-all header. Game and the six test forced-include flags now include only canonical Aurora MSL_C/stdio.h, whose declaration-order requirement is independent of Game (formatter lane owns implementation). The 15 explicit old-prefix consumers also include that canonical header before other includes.

Original RVL revolution/types.h owns ARRAY_SIZE and compiler intrinsics. __fabsf preserves float sign clearing; __abs preserves the INT_MIN two's-complement result without signed overflow. __fabs now has the donor f64(f64) signature (the old prefix incorrectly narrowed input to f32). memcpy-based bit clearing preserves payloads and signed-zero behavior with no aliasing violation. Aurora macros.h owns the original degree conversion constants and single-precision operation order. Existing canonical NO_INLINE/alignment macros and JKRHeap placement allocation declarations replace their duplicate prefix definitions.

The mixed-long integer clamp overload now sits beside the original s32 clamp in Game/Util/MathUtil.hpp under TARGET_PC; its narrowing and original s32 dispatch are unchanged. getRandom(long,long) was already declared by this owner. The two bare DUMMY scratch functions are explicitly local static in their actual files.

Actual callers now include defining functional.hpp, JKRHeap.hpp, DirectDraw.hpp, math_types.hpp or LiveActor.hpp where source inspection shows dependence on the former forced includes. This first pass is deliberately bounded; the parent integrated build supplies any remaining concrete include diagnostics. No source logic changes beyond the two local scratch definitions and the existing overload relocation.

Manifest records all 66 owned paths and their exact before snapshots, including already-dirty CP932 include changes and shared Game/test xmake content. No build, test, staging or commit performed. Parent owns integrated validation and publication.
