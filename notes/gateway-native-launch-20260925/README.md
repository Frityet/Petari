# Native launch and renderer frame ownership

The fresh-save original game completed the tower launch, landed on the BlackHole planet and collected chip799. Later ordinary movement crossed the crater edge; during the resulting black-hole death the renderer crashed in `submit_frame_prefix` with a null frame encoder. The full route and exact selected snapshots are documented in [partial-flight-evidence.md](partial-flight-evidence.md). This run predates the recorder fix below and does not validate restart or Grand Star completion.

## Change

Aurora now serializes frame-recorder publication, FIFO decoding, completion-prefix submission and frame closure. Encoder creation is queued before exposing a new recorder; closure cannot retire the packet while a prefix borrows it. The lock is released before guest callbacks, and render-worker tasks never take it. [restart-render-lifetime.md](restart-render-lifetime.md) explains the observed crash, the consistent ownership interleaving and lock ordering. This is general renderer ownership, without a game/death-specific bypass.

The existing Metal draw-sync test now also exercises24 helper-thread completions across12 actual frame publication/retirement cycles. Only the main test thread emits GX commands. An external45-second deadline bounds genuine deadlocks.

The notes-only navigator now checks the actual grounded movement bit for stall jumps. Its previous nonnull-triangle check could use stale collision data. [crater-detour.md](crater-detour.md) supplies an original-KCL route candidate around the observed fall; it has not been traversed live.

## Focused validation

- `xmake build -y -j16 smg-pc`: passed,7.48seconds.
- `xmake build -y -j16 smg-pc-aurora-draw-sync-pass-render-tests`: passed,3.82seconds.
- Actual Metal test: exit0,0.77seconds. Prior EFB callback and color/depth/stencil continuation checks pass, plus24 concurrent completions across12 frames. Cache-path warnings mean this run did not use persistent test caches; device-destroyed is teardown. No map-aborted or null-encoder failure occurred.
- Fresh-save original Gateway startup:120 completed frames,exit0,3.11seconds, no timeout, process reaped and binary unchanged. SHA256 `408def2af5ab6bdc62952a3ac381c6080b2876229217a91dd32148b119a423a2`. This short run checks startup/shutdown, not a replay of black-hole death.
- No broad fixture suite or second full-route replay was run after this fix.

The crash run used source `ff6bf1b4572b86a58d48c44738ac67e770c4de4d` and bundle hash `a0a0912c63f60d8197f2aae7142714eb71bfb9f0ba814f750c31f6b82a78b25b`. It proved the earlier raw Wii-FIFO trail fix worked through actual flight. Selected evidence is compressed; the307MB full actor trace, extracted disc archive, caches and NAND remain local. No game/controller process remains active at publication.

Remaining: validate original death/restart with the ownership fix, collect all five chips using actual terrain, traverse Small/Middle/Inside, obtain the Grand Star and observe the original stage handoff. `src/compat` and `src/scene` remain absent.
