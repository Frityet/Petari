# Native layout allocation boundaries

## Observed failure

The real Gateway showcase reached its control message and original StarPointerDirector drawing. A1MiB host texture copy then attempted to allocate from the director's small original Game heap and threw bad_alloc. Exact stack: notes/demo-camera-jump-20260910/after-gx-end-stack.log, LayoutRuntime::draw -> ensureTextureUploads -> AuroraRenderer::create_rgba8_texture -> std::vector<u8>::assign -> routed operator new. Parent separately owns the renderer API boundary repair.

## General native boundary fix

LayoutRuntime owns native parser data, decoded textures/fonts, animation names, pane override/follow maps, text templates/rasters and render batches. Added explicit JkrHostAllocationScope at its allocating native public entry points, lazy parser and helpers also entered directly by native record/debug access. This includes draw itself, so all first-upload/text-composition/quad scratch allocation remains host-owned. Scalar-only property methods retain their existing implementation. Existing pane scale/rotation scopes remain. scoped-methods.json names the52 newly guarded methods/overloads; both constructors are also guarded.

Constructor name/layout-name initialization now occurs inside the guarded body; a body-only guard would be too late for allocating std::string member initializers. The optional archive path is moved into its member inside that body. The API's by-value archive-path argument is still supplied by the caller; the regression passes an already-owned path by move. No signature/ABI or original Game code changed.

The host scope changes global-new routing temporarily and restores it on return; it does not change the selected original JKR heap/domain. These native operations do not invoke allocating original Game callbacks. RuntimeContext effect registration here uses the native SimpleLayout metadata path. Font access reads its native host-resource handle. Existing Nw4rLayoutRecords::synchronize already has an Aurora HostAllocationScope, including its direct mutations of LayoutRuntime private maps, so no Nw4r change was needed.

## Regression

The existing OriginalLayoutGroupTests fixture adds a64KiB real Game child heap around native LayoutRuntime construction, actual synthetic RARC/BRLYT parsing, pane alpha/visibility/follow/rotation mutations and debug snapshots. It checks the long retained name and returned snapshot allocation are outside JKR, that these operations consume zero caller-heap bytes, and that a subsequent ordinary allocation routes back into the original Game heap. It then releases the Game domain and checks layout matrices, snapshot names and mutable pane metadata remain usable. This checks retained ownership, not only allocation size.

The same fixture's real-disc FlyMeter/SubMeterLayout test now calls original setLifeRatio and draw under its actual original Game domain, asserting zero Game-heap consumption during first texture/font/text uploads and subsequent draws at three ratios. Renderer begin/end remains in its existing host call context. This reproduces the general native API entry from original Game code without a StarPointer-specific bypass.

## Validation state

Final LayoutRuntime.cpp and OriginalLayoutGroupTests.cpp compile0 separately with the exact root LLVM flags (native-compile.json, fixture-compile.json). Source hashes are recorded. No global build was run by this agent. Parent then built the official smg-pc-original-layout-group-tests target successfully (notes/demo-camera-jump-20260910/native-layout-group-build.log). The resulting binary ran with the exact Korea RVZ and exited0 in1.175s. It passed the new64KiB Game-heap ownership/retirement regression,32 synthetic group/3D teardown cycles and actual FlyMeter original update/draw at three life ratios under its Game scope with zero Game bytes consumed. Exact command, environment and binary SHA256 fb731157c387ae0b110a6d507a71db48ceeac336794f814fa29768f82223792d are in runtime.json, with runtime.log. This is actual fixture draw evidence; the full showcase run remains parent-owned.

A final scope audit retained guards only at native work boundaries, allocating lazy/query helpers and native error-string construction. The purely scalar update/transformation/dead-state getters and debug counts remain unchanged. A few nested guards are intentional: public queries need to own their temporary/error allocations while loadRenderData must also protect direct friend access. They only save/restore the allocation routing flag and do not change the selected Game heap. No callback was added, invoked in a new phase, or moved to another heap.
