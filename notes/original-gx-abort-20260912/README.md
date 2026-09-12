# GX command abandonment

The original MainLoop watchdog invokes GXAbortFrame from an OSAlarm callback. This is a scheduler-disabled interrupt: the entry point must never wait for a guest callback, FIFO decoder, render worker or GPU. GXDisableBreakPt and GXGetGPStatus are part of that same call path and must not acquire the decoder lock.

The implementation separates FIFO address/storage lifetime from a cancellable command epoch. Aborting invalidates pending command and callback delivery immediately, records the logical write cursor to discard, and wakes the decoder. Before any subsequent command executes, the decoder retires old queued renderer work and resets the unsubmitted recording state. The logical ring and monotonic cursor values survive.

Renderer packets and native GPU completion waits capture the command epoch. Pending work is discarded, with tagged depth requests and unsubmitted completion closures retired. Actual submitted native GPU resources retain their owners and completion callbacks. Display-copy versions retain the preceding submitted contents when a pending replacement is abandoned.

This is command abandonment, not native device-loss recovery. WebGPU does not expose cancellation of already submitted GPU work. Native device errors remain explicit failures; no fabricated completion or performance counter is supplied.

The focused CPU and actual GPU checks below passed. Publication belongs to the parent agent, which also owns shared builds and GPU runs.

## Implementation checkpoint

- `GXAbortFrame` publishes a command-epoch abandonment and logical write-cursor floor without guest waits. Breakpoint disabling and decoder status no longer require the decoder mutex. Breakpoint arming releases FIFO binding ownership before its cooperative decoder lock acquisition.
- Decoder acknowledgment retires old render-worker packet references before clearing packet containers or executing later commands. Repeated epoch changes are rechecked around retirement/floor publication.
- A stable atomic registry arbitrates each native encoder's commit versus abandonment. Its capacity is the actual two frame packets plus one prefix replacement. Completed history retains finalized results without consuming registry slots. An abort scans atomic identities rather than dereferencing encoder-owned objects.
- Unsubmitted render passes/tasks/completion closures and tagged depth requests are discarded. Submitted readbacks and upload buffers retain asynchronous map ownership. GPU fences, pipeline waits and prefix upload-buffer acquisition observe cancellation.
- Copy destinations retain their prior version until the replacement commits. Size-keyed allocation caches retain their own prior version, so changing copy dimensions does not restore an incompatible texture under a new key. Cached dynamic-palette conversions also retain submission ownership; an abandoned conversion is regenerated even if source/TLUT version numbers are reused.

`source-manifest.json` freezes the exact Aurora source/test cohort. It excludes the other agent's DVD changes. No shared build definitions changed.

## Validation

- Fourteen production/test syntax probes pass (`syntax-second.json`), followed by the staging-cancellation and three palette probes.
- An isolated 10,000-iteration simultaneous commit/abort test passes with AddressSanitizer and UndefinedBehaviorSanitizer while retaining all historical outcomes and repeatedly reusing the bounded registry (`submission-race.run.json`). This validates CPU ownership arbitration, not GPU rendering.
- The final shared build of all three targets passed (`shared-final-build.json`). Parent runtime receipt `shared-root-results.json` records:
  - `gx_fifo_tests`: all 294 tests passed, including the original alarm interrupt context, stopped FIFO abandonment, callback acquisition race and post-abort progress; SHA-256 `3be6ebf5a71eade559fdd8cc22d520bdf9e6ffa8028ff7cd17a9e79e3bd5ef94`.
  - `gx_texture_cache_tests`: all 22 tests passed, including regeneration of an abandoned palette conversion with unchanged copy/TLUT versions; SHA-256 `b57fa558743ba5f14ff1e21711765d8b507feee5f48b100ce25488915f9fd31a`.
  - `gx_vi_scanout_render_test`: actual Metal passed. GPU pixel reads verify that abandoning pending blue over submitted green preserves green, committing blue then abandoning pending red preserves blue, and existing VI flush/retrace/black/retirement/reuse behavior still works. The abandoned tagged depth request reports dropped, not completed. SHA-256 `9b3f90f87ba4ff60efed681d957e2f1e4d352d5dffb9a165625089c24dc99dab`.
- Source hashes were rechecked against the frozen manifest after these runs with no drift.

These are command-lifetime and rendering proofs, not a hardware GPU hang/device recovery test or a full original MainLoop/GameSystem startup proof. An already committed native queue submission remains owned and may finish after abort returns; the API cannot revoke it or repair native device loss. The implementation introduces no Game, actor, stage or demo-specific branch.
