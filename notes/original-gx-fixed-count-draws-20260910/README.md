# Original fixed-count GX draw completion — 2026-09-10

The Wii SDK's `GXEnd()` is an empty inline function (`decomp/libs/RVL_SDK/include/revolution/gx/GXGeometry.h:29`). A retail draw is complete when the FIFO contains the number of vertices declared by `GXBegin`. The native compatibility API had incorrectly required a subsequent `GXEnd`, aborting when original Game code began its next already-complete primitive.

The frozen repair distinguishes ordinary fixed-count primitives from Aurora extensions. The next begin publishes the preceding fixed draw before writing another header, including raw FIFO vertex writers. Explicit `GXEnd` remains a publication hint. Aurora automatic-count and indexed extensions still require their explicit end, while the decoder continues to reject truncated fixed-count payloads. No Game source or retail source changed.

The initialization agent prepared the fix and five regression tests; the root audited the production behavior and continued real demo testing. This agent independently reviewed the saved patch and ran the existing separate CMake test lane, without using root Xmake.

Validation: build exit **0**, **274/274** tests passed, zero skips. New tests cover omitted ends across state changes/raw writes, display-list recording, threaded publication before the next header, malformed vertex counts, and retained extension end requirements. Existing automatic-size patch/publication and final-command drain tests also pass. This suite uses renderer test doubles and does not establish real GPU presentation.

Exact commands/results: `result.json`; logs: `build.log`, `tests.log`; test detail: `tests.xml`; source/reference hashes: `source-manifest.json`. The two Aurora source/test files were committed as codex; publication SHA and remote verification are recorded in `publication.json`.
