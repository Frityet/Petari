# Original mirror material owner, 2026-09-13

The real SimpleMapObj/MapObjActor constructor links mirror support even for authored configurations that do not select a mirror. Recovered all six MirrorReflectionMtxSetter methods in the canonical MaterialCtrl header/source, then copied the same class and method bodies into native. Restored DisplayListMaker::addMirrorReflectionMtxSetter through the original new/push/return operation.

The original owner discovers MirrorTex texture names, selects the actual materials using that texture, and retains the original eight-slot set of active projection texture matrices. Its update reads the real camera's mirror-model texture matrix each frame. All original table scans, projection-mode masks, matrix-use queries, ordering and typed pointer widths are preserved. The SDK setter's non-const source signature requires an explicit const cast for the borrowed matrix; it only reads that source.

All seven recovered methods compile with the original and native compilers. Six match retail at 100%; the texture-coordinate selection method is 97.37%. Commands and first comparison summaries are adjacent. Existing non-mirror MaterialCtrl/DisplayListMaker methods were preserved. No per-actor fallback or bypass was introduced, and no tests were added or run. This proves source closure, not a rendered mirror.

Owned files: decomp/include/Game/LiveActor/MaterialCtrl.hpp, decomp/src/Game/LiveActor/{MaterialCtrl,DisplayListMaker}.cpp, native src/Game/LiveActor equivalents. Other mirror camera/texture helpers are documented in their separate owner notes.
