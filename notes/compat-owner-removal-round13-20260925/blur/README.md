# Original fullscreen blur owner

Round13 baseline: `d24ec238e577ebc971998e6d5ae9f5fc6ad485f5`.

Removed `CapturedFrameBlurService.cpp/.hpp` and its scene binding allocation, accessor and lifetime. The canonical `Game/Screen/FullScreenBlur.cpp` already matches the complete decomp source byte-for-byte. Root must remove its `src/Game/xmake.lua` exclusion so both original `MR::drawFullScreenBlur` overloads are linked.

The removed service was an alternate renderer: a separately allocated RGB565 texture, lazy history state and an Aurora offscreen framebuffer downsample replaced the original sequence. The original renders the current capture into hidden EFB rows using `GXSetScissorBoxOffset(0,1584)`, blends the current and previous frames, then copies `(0,464,128,64)` into Mario's original `_B7C` texture. That texture is allocated as RGBA8 by `MarioActor::initDrawAndModel`; `PlayerUtil` supplies the actual owner. No Game algorithm changed, no history-valid shortcut retained, and no replacement service introduced.

`JUTTexture` already registers its exact object with the containing JKR heap's finalizers. The parallel texture-owner migration retains this and places native MEM1 allocation ownership in JUTTexture, including GPU-copy invalidation and draining before byte reuse. Consequently the original scene heap owns the history lifetime without a blur-specific adoption hook.

The existing, initially dirty `CenterScreenBlurRealOrAbsentTests.cpp` keeps its prior real-process rewrite. This lane changes only obsolete service contracts: lazy allocation/counters/custom errors become actual RGBA8 texture and identity checks; existing public action, nerve, demo registration, two draws and retirement checks remain. `before/` preserves the original dirty bytes, and `owned-working-delta.patch` records this lane's exact delta. No test was added or executed.

## General backend dependency (root-owned)

At baseline Aurora declared `GXSetScissorBoxOffset` but did not implement it, and BP decoding only handled scissor registers 0x20/0x21, not 0x59. `map_logical_scissor` also clamps the EFB-copy source to the displayed framebuffer; a 456-row backing cannot capture physical EFB rows464–527. Correct restoration requires general scissor-offset wrapping, viewport translation and physical EFB backing/copy extents. Dolphin's `VideoCommon/BPFunctions.cpp` and `BPMemory.h` document the offset register's masked bits and wrapped ranges. Root owns those Aurora changes; this lane does not reproduce the deleted offscreen workaround.

Source checks confirm exact donor equality and no retired service references under `src/` or `tests/`. These checks do not establish rendered blur correctness. Integrated build and bounded runtime validation are root-owned.
