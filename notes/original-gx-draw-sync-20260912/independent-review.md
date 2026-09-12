# Independent draw-sync submission review

Reviewed `frame.cpp::submit_frame_prefix`, `recording.cpp::complete_draw`, FIFO token callback dispatch, callback retirement and pipeline waiting. Review performed by the compat-audit agent; production fixes belong to root.

Two concrete issues found and fixed by root:

1. The deferred EFB pass splice could keep pre-offscreen EFB draws outside the command encoder while a token in the offscreen pass completed. The callback could therefore read an earlier EFB state. `suspend_efb` now always enqueues the previous EFB pass, and restoration resumes it with Load.
2. Completion used an EFB-specific continuation helper for arbitrary color/depth/stencil passes. Stencil load/store metadata was lost, and a caller's final Discard policy discarded attachments before the continuation loaded them. The generalized helper now copies target, depth and stencil metadata, explicitly loads existing attachments, preserves final store policy on the continuation, and forces Store only on the interrupted prefix.

The final reviewed implementation uses a deque for render passes, so creating the continuation does not invalidate the reference to the prefix before its stores are changed. Prefix staging transfers occur after render-worker synchronization; retained byte offsets and upload high-water marks remain valid. The worker holds the frame through submission/fence and remaps the prior buffer afterward. Draw-sync callbacks run under a recursive mutex so self-unregistration works and external unregistration waits for callback completion and snapshot release.

Cold pipeline waits use the independent compilation worker and release the condition-variable mutex; the non-worker backend creates synchronously. Waiting removes dropped first-use draws at the cost of first-use latency. No deadlock found in the reviewed paths.

New standalone actual-GPU regression: `aurora/tests/gx_draw_sync_pass_render_test.cpp`. One test opens offscreen rendering before the first token and verifies its callback sees the preceding EFB depth; another splits a Depth24PlusStencil8 pass with final Discard policies, verifies original color retention, depth rejection and stencil acceptance through GPU color readback. Root integrated the source into the upstream CMake GPU suite and the focused root Xmake target. The focused target builds and passes on actual Metal: `pass-boundaries-build.log` and `pass-boundaries-metal.log`. GPU assertions verify the submitted depth and color results, not just recorder metadata. No further production changes were required after the two fixes.
