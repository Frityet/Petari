# WPAD SDK callback and request regressions, 2026-09-10

Added 13 regression groups to the existing `aurora/tests/wpad_test.cpp`; the previous 18 groups remain. This subtask changed only that test file. Parent owns the new Aurora SDK implementation and declarations. The test fixture now initializes a complete fresh WpadService and restores the previous full service afterward, including client callbacks/configuration; clearing only physical channels would leave callback state shared between tests.

The tests exercise the actual native SDK service through its public WPAD/KPAD APIs and owner scope:

- Exact 24-byte WPADInfo scalar layout and field offsets are asserted at compile time.
- Connection callbacks run after published buttons/stick samples, can register the extension callback, and precede the extension event. Repeated dispatch and callback registration alone produce no duplicate transitions. Device changes and disconnection/reconnection are checked separately. A connection callback can disconnect before extension publication.
- WPADGetInfoAsync accepts one deferred destination per connected channel. Busy, absent, invalid-channel, and null-destination errors complete synchronously; accepted requests write only on dispatch. Guard words and exact byte snapshots prove bounded writes and no write on disconnection/error. Core/Freestyle attachment, battery, speaker, LED, and other virtual-device fields are checked.
- Completion can enqueue another info request, which must remain pending until the next dispatch. Independent channel requests remain independent.
- WpadClientScope suspends an outer request, lets the inner client observe current physical connection, cancels inner pending writes on destruction, and restores the outer request and callbacks. The abandoned inner destination is actually freed before later dispatch.
- Retirement inside each connection, extension, and info callback frees a pending destination. ASan verifies the dispatcher does not retain/use the invalid pointer, and callback counts verify no old-client continuation. A temporary nested scope inside a connection callback leaves the restored outer request for the next dispatch, preserving it while dropping the inner stack destination.
- Speaker enable/streaming commands fail synchronously because this device has no speaker; disable succeeds for a connected device. Unsupported channels return their connection error.
- KPADReset clears sample history and button timing while preserving connection, live inputs, configuration, pending requests, and callback registration. KPADInit additionally restores sampling parameters without disconnecting the client. Sensor-bar and autosleep configuration remain explicit across sampling resets.

## Validation

The isolated LLVM 23 `-O2` ASan/UBSan build and runtime both exit 0: **31/31 tests pass**, zero skipped. The executable links actual `aurora/lib/wpad.cpp`, existing `wpad_test_stubs.cpp` (the physical PAD motor boundary), and GoogleTest. It does not provide a Game owner or replace Game input processing.

`results.json` contains commands, exact source hashes, and binary identity; `tests.xml` and `run.log` contain all test outcomes. The initial compile-only attempt omitted the required existing `AURORA` / `TARGET_PC` preprocessor definitions; its invocation error is retained in `first-build-results.json` and `first-build.log`. Repeating the compile with the same definitions as the existing target succeeded without production source changes.

No root Xmake, new target, Game edit, commit, or push was performed. These are native SDK behavior/lifetime proofs; parent owns full original WPad integration and gameplay validation.
