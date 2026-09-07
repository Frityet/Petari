# Mario draw and bee wall owner recovery, 2026-09-07

Six methods are newly reconstructed from the fresh RMGK01 assembly in the original reference source tree, then their bodies are copied byte-identically to the PC source tree. Four were unresolved in the native showcase; the paired original state producers are recovered too. No inline assembly or invented stage logic is used.

| Method | Retail address | Size | Fresh PPC fuzzy match |
| --- | --- | ---: | ---: |
| updateBeeStickMode | 0x802BA4E8 | 772 | 99.1658% |
| calcSpinEffect | 0x802C3820 | 328 | 97.743904% |
| drawSpinEffect | 0x802C3968 | 832 | 81.52404% |
| initDarkMask | 0x802C3D64 | 116 | 90.68965% |
| updateDarkMask | 0x802C3DD8 | 176 | 100% exact |
| drawDarkMask | 0x802C3E88 | 948 | 97.31223% |

The unchanged original DOL is SHA1 25c5959534b3c21246c6c7e42021b916b41fb578. Target functions and constants come from fresh dtk split output. Original GC3.0a3 compiles both affected complete reference TUs. LLVM23 compiles both complete port TUs with current Game build flags. This validates source and linkage prerequisites, not a runtime or visual outcome.

`updateBeeStickMode` preserves the actual Fur/Normal wall-code and BeeWallShortDistArea queries, original Mario draw/movement flags, two cancellation radii, jump/rush ordering, frame countdown, original normal impulse and jump-vector projection. The canonical area/name strings are read from DOL bytes at0x805B86E9 and0x805B86FE, not inferred from names in other games. No host-only branch is involved.

`calcSpinEffect` preserves the actual selectAction string スピン回復エフェクト (DOL0x805B8F35), recovery-stage gates, timer window and original radius/height arithmetic. `drawSpinEffect` emits both original 64-segment rings (65 samples each), including the two sets of65 random draws, rotating the direction continuously between passes, and preserves the original sin-fourth-power color arithmetic and GX line/blend state. Its lower fuzzy score is mostly the current MathUtil.hpp inlining MR::sin and its JMath lookup instead of the retail out-of-line call, plus register/constant placement. The arithmetic and calls are retained; no exact instruction-match claim is made for this method.

The mask routines operate on both original8x8 texture images. Update toggles the current image, draws two original random coordinates, writes0xF0 to both images, flushes only the new image and increments the original u16 timer. The u16 update argument is unused in retail. Draw preserves the actual alpha-only GX pass, texture coordinates0..4, screen/framebuffer rounding, and restores the original depth/alpha settings through TDDraw. The retail code subtracts half the width from both projected coordinates, even though it computes height separately; this quirk is deliberately retained. The otherwise-unused getRealPos(Spine1) call also remains, as observed at0x802C3F44.

The reference MarioActor header now types the adjacent _B80/_B84 texture pointers as their original two-element array, already present in the PC header. Two reference initialization sites use this typed array. Separate GC3.0a3 compile assertions verify the two-image array size8 and offsets _B80=0xB80, _B88=0xB88, _1C4=0x1C4. The affected original ActorDraw and ActorInit TUs also compile. No texture owner or allocation substitute is introduced. Actual texture creation and ownership remain in the original actor draw initialization.

Full native showcase execution and rendering still require the parent's original Player/provider activation and shared GX/helper closure. Existing unrelated special-draw bodies that remain undecompiled are not silently completed by this change.
