# Depth sampling for physical EFB backing

The full EFB extension exposes rows below the configured VI display. Existing depth sampling mapped the entire source texture onto the display-sized output: a 640×528 EFB and 640×456 snapshot would either stretch Y by528/456 or introduce a 36-row FIT offset. This would move original GXPeekZ/DrawSync depth away from the drawn pixels.

Each `SnapshotCapture` now records the published physical EFB logical extent alongside its existing dimensions and viewport policy. Tagged snapshots capture it while recording; direct physical-EFB captures capture it before worker dispatch. The legacy path also records complete geometry in the render pass rather than reading mutable VI/GX geometry later on the render worker. The worker maps EFB coordinates with `sourceSize / recordedEfbExtent` and no letterbox offset. Output dimensions remain unchanged. Native policy retains its direct coordinates; an inactive VI has a zero extent and retains prior FIT/STRETCH behavior. Tagged offscreen requests remain rejected as before.

The existing test stub receives only the signature update. No new tests, builds or test executions were performed. The manifests, before/after snapshots and exact delta cover six Aurora files. `recording.cpp` is shared: its before snapshot includes root's earlier EFB target changes; this lane changes only snapshot capture blocks.

## Read-only integration review

- VI publication and main color/depth target allocation preserve separate display and physical EFB dimensions. Original VI presentation selects the completed XFB display copy, so hidden EFB rows are not stretched into presentation.
- Found `window::resize_swapchain` returning before comparing physical backing dimensions when window dimensions were unchanged; root fixed that path.
- Reported that NATIVE↔STRETCH changes also need to queue a resize because both map to `aspect_fit=false`, whose setter otherwise returns early. Root owns that correction.
- Scissor-offset rendering and full-EFB color copy remain root-owned. This source review establishes no blur pixel proof.
