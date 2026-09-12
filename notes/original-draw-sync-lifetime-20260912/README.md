# Original DrawSyncManager SDK support

Aurora now exposes stable FIFO addresses and complete-command breakpoints to original SDK clients. Deferred CP/PE callbacks enter the guest CPU execution scope with the saved client allocation policy and interrupt mask. A cooperative SDK mutex allows callback registration/retirement to wait while an original callback blocks on an OS queue. Native FIFO waits temporarily release the guest CPU, allowing the original DrawSyncManager worker to run.

The new original-manager fixture compiles the unmodified Game/System/DrawSyncManager.cpp. Its actual SDK worker handles 128 GPU callbacks across both original token ranges, turns over its bounded queues, disables the final breakpoint, and joins during destruction. No replacement token queue or client-specific callback dispatch is used.

That real renderer path exposed a general ByteBuffer error: an empty append discarded the capacity of borrowed mapped storage. Empty appends now preserve storage, and append_zeroes clears newly exposed bytes even when capacity is reused.

## Validation

- Aurora OS execution/message/mutex suite: 31 tests passed.
- Aurora FIFO suite: 290 tests passed, including 13 breakpoint/callback regressions and three ByteBuffer tests.
- Six Xmake targets built and ran successfully against the actual Metal backend where rendering is required: original DrawSyncManager, EFB depth snapshot, draw-sync render pass, Z texture, clipping, and texture-object queries. Each run used a fresh cache directory. reviewed-gpu-results.json records exit status and exact executable SHA256.
- Independent review fixed cooperative callback-lock retirement and missing callback interrupt-mask entry before these final runs.

The compressed evidence contains final build/run logs, machine-readable results, test XML, the earlier mapped-buffer crash stack, and the independent review. It does not contain game assets or build binaries.

## Bounds

The manager fixture uses native allocations and explicitly frees its original raw helpers after shutdown. Actual retained JKR process ownership, rollback of partially initialized callback owners, and scene retirement remain the next integration work. This checkpoint does not establish original Talk/StarPointer lifetime or gameplay progression.

FIFO breakpoints support complete native command boundaries. Partial-command fetch, prefilled ring import, independent CPU/GP routing, and hardware overflow suspension remain unsupported. Guest interrupt-mask behavior is covered; complete PowerPC OSContext identity is not implemented. Callback retirement must drain GPU work and both original manager queues before clearing borrowed callback pointers.
