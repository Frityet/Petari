# General GX polygon clipping control

GXSetClipMode(GX_CLIP_DISABLE) now reaches decoded XF state and a real polygon rendering path. The recovered original FurDrawer first exposed the previous explicit abort; no game, actor, material, or stage-specific condition was added.

## Semantics and source evidence

Dolphin VideoCommon/XFMemory.h:275 separates bit0 clipping detection from bit1 trivial rejection and bit2 clipping acceleration. Its VideoBackends/Software/Clipper.cpp:76 computes six original clip outcodes, and lines292/348–365 perform trivial rejection before optionally bypassing polygon clipping when all W values are nonnegative. Thus replacing depth clip distances with positive constants alone would be wrong: a wholly outside triangle must still be rejected. The native GX API emits bit0 only. Other raw XF bits remain explicit unsupported controls.

Aurora retains original polygon topology indices in host storage and draws three vertices per instance only for clip-disabled polygons. A shared generated projection helper loads the actual position attributes and original per-vertex position matrix index. All three projected positions produce a common outcode intersection. A wholly outside triangle is rejected; otherwise nonnegative-W triangles bypass polygon clipping. Eye-plane crossings retain six homogeneous clip planes using the same conservative condition as Dolphin software. Lines and points retain their existing paths, matching the fact that this software polygon switch does not change their processing.

WebGPU fixed XY clipping surrounds the complete attachment in this path. A shader transform preserves the authored viewport inside it, allowing otherwise visible pixels outside the viewport within the attachment/scissor. Draw records retain that original viewport and restore it immediately after each draw. Existing depth-range mapping, reversed Z, culling, scissor, and the enabled-mode two-plane shader remain in place. The geometry/viewport union preserves the 128-byte inline draw payload budget. Existing immediate padding carries a triangle-storage offset, with UINT32_MAX tagging ordinary draws. GX pipeline cache version is15.

Both retail FIFO topology conversion and the GXBeginIndexed/optimized display-list extension use this path. Disabled polygon draws are not merged with ordinary draws. Vertex arrays and index storage remain owned by existing host staging. No guest resource storage is adopted by a new persistent container.

Independent review found and verified corrections for three additional boundaries: top-only viewport moves now invalidate uniforms; indexed extension draws retain their real triangle indices; offscreen start/restore reconcile decoded render viewport and invalidate target-size-dependent uniforms. The renderer tests exercise all three boundaries.

## Validation

The root Xmake target smg-pc-aurora-clip-mode-render-tests builds and exits0 on Metal with73 rendered checks: seven geometry cases in both modes across quads, triangles, triangle strips, triangle fans and GXBeginIndexed (70), plus top-only viewport movement, offscreen EFB restoration, and byte-identical surviving pixels for a triangle crossing the eye plane (3). Cases cover near/far crossings, complete near/far rejection, viewport guardband pixels, actual per-vertex matrix indices, and native indexed position arrays. Each primary case also checks immediate re-enable and original viewport restoration. This is real framebuffer pixel validation, not shader syntax alone.

The existing CMake gx_fifo_tests target builds and all256 tests in six suites pass, including ordinary display-list optimization, FIFO ordering, depth range, and new clip-state/depth-viewport tests. Prior explicit rejection tests now assert the implemented state semantics. Test-only debug-label stubs were also updated to the existing string_view API from the earlier allocation-ownership change. Seven initial modified TUs passed standalone syntax; the final complete Xmake and CMake builds validate later integration edits.

Parent independently rebuilt and ran the original FurDrawer test successfully after these changes. The root repository retains all commands, logs and source hashes in notes/gx-clip-mode-20260907; results.json summarizes the tested source. No root or decomp index was touched by this Aurora checkpoint.

## Remaining hardware uncertainty

Dolphin explicitly documents that the extreme hardware guardband limit and exact condition that re-enables clipping differ from its software approximation. This implementation preserves that known conservative nonnegative-W rule;73 native checks establish its stated behavior, not unmeasured Wii rasterizer parity at those extreme boundaries. Raw XF bit1/bit2 controls remain unsupported. Full original actor fur rendering and gameplay completion are separate runtime milestones.
