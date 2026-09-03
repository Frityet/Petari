# Complete-model sanitizer gate

The frozen parent `OriginalJ3DModelResourceTests.cpp` passes all four groups
under the isolated ASan/UBSan harness. The actual retained Mario BDL was loaded
with original binary flags `0x01200000`, `0x01201000`, and `0x01202000`. The
fixture also exercises original BMD3/BDL3 dispatch, actual recursive joint
calculation, original allocation-domain and alias retention, OS interrupt/GD
state restoration, texture retirement, and ordered aliased display-list
validation. No alternate model classes or fake virtual objects were added.

`verify-native.py` compiled 58 source objects using each source's frozen
configured build flags plus `-fsanitize=address,undefined` and
`-fno-omit-frame-pointer`. The instrumented closure includes every complete
model component, native metadata decoder, original material/shape factory,
hierarchy/finalization bridge, material blocks, matrix traversal, packet
providers, JKR heap/domain/provenance providers, and OS allocation/lifecycle
providers listed in `native-asan-evidence.json`.

The remaining SDK, render/runtime, DVD/archive/disc decoder, GX/FIFO, shared math,
standard/package libraries and Aurora `compat.cpp` object were linked from
frozen uninstrumented outputs. This is a broad system-boundary sanitizer run,
not whole-program instrumentation. The harness does not run xmake or GPU
frames and does not replace production code.

`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` were set. The process exited
zero with no sanitizer diagnostics. `native-asan.log` includes all three
required real-disc markers and the four-group success marker; the script
rejects a skipped real-disc fixture.

The run verified 246 included workspace file hashes,
17 linked archive hashes and 59 build-flag cache hashes before and after
execution, plus every instrumented source and the retained Aurora compat
object. The binary SHA256 is `5b9778645269466202ec59c3a6b55aee9070f4f9b2adf42cb1df2c22d16ea99b`. Commands, objects and
individual compile/link logs remain under ignored
`build/original-j3d-model-resource-20260903/sanitizer/`; the evidence JSON retains
the exact source/header/archive identities. `SMGPC_REAL_DISC` may select an
explicit existing disc, otherwise the verifier uses the retained Korean RVZ
in the project root.

Only this verification script, log, evidence and note are owned by the
sanitizer worker. The parent owns production integration, test source, shared
builds and commits.
