# FIFO commands split by a GP breakpoint

The live original Gateway process PID 6264 aborted in the Aurora FIFO worker while processing a seven-vertex triangle strip. LLDB inspection proved that the source command was complete: the active ring breakpoint capped the decoder one byte before the vertex payload ended. This was the original `input-light-plant-ui` process under LLDB. Its runner requested 4500 frames and later reported exit 0, but that result was spurious: macOS LLDB attach reparented the inferior, so it ceased being a waitable child of the runner. The log lacks the original-loop completion marker and contains this FIFO abort. Classify this as a debugger run with a captured failure, not a completed 4500-frame run. The initial separate-session interpretation was corrected after parent-PID and completion-marker evidence.

## Captured evidence

The read-only `capture.lldb` script was sourced by the parent debugger; its output is in `../gateway-compat-20260919/input-trace.log`, beginning at line 2220. The process was killed and LLDB quit after inspection.

- `process_to` target and stream written/published counters: 6863771982.
- Decoder cursor: 6863134786. Buffer base: 6862018835; staged bytes: 1753147.
- Active breakpoint cursor and capped processing end: 6863209267.
- Reader start: 0x46011072f, length: 74481, vertex payload begins at reader offset 74426.
- Seven vertices, eight bytes each: 56 required, 55 before the breakpoint.
- The missing final byte was present at 0x460122a20: `62`, followed by the next valid draw header `98 00 06`. `boundary-bytes.bin` preserves the inspected 256-byte region.

Thus neither malformed original geometry nor a missing producer write caused this failure. A passed saved ring address is rearmed on the next lap, where it can fall inside a different command.

Local Dolphin reference supports the separation: `Source/Core/VideoCommon/Fifo.cpp` retains unread buffered bytes across fetches, and `OpcodeDecoding.h` checks the full three-byte draw header plus count times decoded vertex size before invoking any primitive callback.

## General implementation

Aurora now tracks fetched and decoded cursors separately. The GP read pointer and breakpoint comparison use fetched bytes. The decoder advances only over complete commands, and synchronous completion advances after their callbacks return. A breakpoint can stop at any byte; its unfinished command prefix remains in the stream storage. Disabling or rearming resumes from that original command start and executes its effects once.

Streaming preflight covers BP, CP, XF, indexed XF, nested display-list headers, ordinary variable-width vertex draws, and all current Aurora extended payloads, including 64-bit pointers, sized/indexed draws, and variable-length labels. It probes a copied reader before any state changes. The default complete-input decoder still rejects malformed finite buffers. GXCallDisplayList retains its existing full-input parsing/validation before enqueueing.

The FIFO's staging buffer is compacted only after `drain()` observes complete commands and returned callbacks through its requested target. Fetching an incomplete prefix does not permit compaction. Publication without a breakpoint can also pause inside a command and resume after a later publication. A synchronous drain requires its producer to finish the command: a permanently incomplete byte stream cannot complete, just as the hardware decoder cannot execute it. This change does not invent padding or round a breakpoint to a different address.

Saved FIFO rebinding compares the actual fetched read cursor and retains pending prefixes. Abort retirement advances fetched, decoded, and completed cursors past the abandoned command epoch.

No Game source changes were needed.

## Validation

Configured `build/aurora-fifo-tests-20260919` independently with Homebrew LLVM 23 and the existing locally cached dependency sources. The old cache under `decomp/build/aurora-upstream-merge-tests` still referenced obsolete pre-flattening source paths, so it was left intact. `configure-command.txt` and `configure.log` preserve the configuration.

Commands:

```
cmake --build build/aurora-fifo-tests-20260919 --target gx_fifo_tests -j 4
build/aurora-fifo-tests-20260919/tests/gx_fifo_tests --gtest_output=xml:notes/original-fifo-command-boundary-20260919/all-tests.xml
```

Final result: **305 tests passed in 11 suites**, exit 0 (`all-tests.log`, `all-tests.xml`, `build-final.log`). Nine new cases cover the exact 56/55 draw boundary, every byte of BP/XF/host-array-pointer commands, rearming inside one incomplete command, publication without a breakpoint, saved FIFO rebinding, abort retirement, and each byte of ordinary/sized/indexed draws with both three-byte and twelve-byte vertex layouts. Existing malformed finite draw/array/indexed-XF tests still pass.

These are CPU decoder/FIFO tests with renderer test support. They prove the reproduced command-boundary defect and lifecycle behavior, not completion of Gateway gameplay or real-GPU parity. The parent task owns the subsequent native production build/run.

Published Aurora checkpoint: `4e08fb21feabbda9427bf9a0c604461fcaec1c62` on `origin/codex/macos-compat`; `git ls-remote` returned that exact SHA. Parent gitlink and notes publication belong to the root task.

## Native original-process check

After the thirteenth native build, `../gateway-compat-20260919/fifo-clean-360.json`
records a no-debugger original-process run through360 frames: exit0, the actual
completed-frame marker, and the process gone after wait. The updated runner
marks verified_bounded_completion=true. The compressed log preserves startup
and teardown. This validates a clean original opening with the FIFO change;
it does not claim rabbit-chase or Rosalina completion.
