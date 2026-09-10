# Native GX FIFO metadata

The FIFO SDK now retains CPU and GP attachment records by value rather than borrowing a caller's `GXFifoObj` address. `GXGetCPUFifo` and `GXGetGPFifo` copy live metadata into the output object and return actual attachment readiness. Getter snapshots retain their cursor values when later commands are emitted. Opaque 128-byte objects are accessed with memcpy, avoiding native pointer alignment and aliasing assumptions.

The native command stream exposes its existing monotonic producer, published, and decoded-byte positions. The decoder worker supplies the consumed watermark through its existing atomic. Snapshot capture follows the same serialized GX producer-thread contract as command emission; the SDK metadata lock does not make concurrent GX producers safe. Stream generations invalidate attached records after backend shutdown/reinitialization.

For the existing `GXInit(nullptr, 0)` native entry point, snapshots report Aurora's actual growable command allocation and current read/write offsets, including commands emitted but not yet published. This is a linear native stream: write/read pointers may point one past its current data or allocation end and are only borrowed snapshots until producer growth/drain/reinitialization. They must not be retained as completed GPU fence addresses. No secondary allocation is used merely to fabricate a non-null FIFO pointer.

For a caller-supplied Wii FIFO buffer, the descriptor preserves its base, size, watermarks, and logical ring address space. Producer/decoder byte progress advances the corresponding logical cursors modulo that size; command bytes continue to reside in Aurora's real stream. The count is the real unconsumed byte count, not a forced zero or modulo count. Native growable-buffer occupancy can exceed a supplied logical ring's capacity; watermarks then report that occupancy, without pretending hardware suspension occurred. The default high watermark is bounded for native buffers smaller than the Wii SDK's 16 KiB reserve.

Supported operations: initialization, initial pointers/limits, empty FIFO attachment/detachment, CPU/GP live snapshot, CPU save, pointer/base/size/count/wrap getters, and decoder-side status/occupancy. Attaching prefilled byte buffers or rebinding with pending commands is rejected explicitly. Independent GP storage is rejected because the current processor decodes one shared native stream. These checks preserve working initial shared CPU/GP attachment without pretending to import or route foreign command data.

## Remaining synchronization boundary

- `GXEnableBreakPt`, `GXDisableBreakPt`, and breakpoint callback dispatch are not implemented by this cohort. They require address-to-command-position tracking and real decoder suspension, including growth/drain lifetime rules.
- FIFO consumed/idle means the command decoder finished available commands. It does not mean submitted Metal/WebGPU work completed. GPU completion/fence callbacks must not be inferred from these snapshots.
- The existing `GXFlush` only emits dirty state; it does not itself publish to the worker. Original DrawSyncManager's token/breakpoint protocol still needs complete flush/token/breakpoint integration before its thread can safely execute. Non-null FIFO snapshots alone do not establish DrawSyncManager readiness.
- GX overflow/underflow interrupt latches, interrupt-driven suspension, independent FIFO command import/routing, and write-gather redirection remain outside this bounded metadata change.

References: `decomp/src/RVL_SDK/gx/GXFifo.c` (`GXInitFifoBase`, `GXInitFifoPtrs`, `GXSetCPUFifo`, `GXSetGPFifo`, `__GXSaveFifo`, `GXGetCPUFifo`, pointer/count/wrap getters); `aurora/lib/gx/fifo.cpp` actual publication/decoder/drain behavior; original `src/Game/System/DrawSyncManager.cpp::pushBreakPoint` and thread protocol.

No standalone build or test was run, as requested. The root coordinator owns the single incremental build and brief existing demo smoke. No Game source changed and no commit was made by this subagent.
