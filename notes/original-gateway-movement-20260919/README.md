# Original Gateway movement observations, 2026-09-19

The preceding 2,600-frame original replay completed cleanly, but that does not prove movement or the rabbit/Rosalina story. This investigation adds a read-only, opt-in debug trace after the original GameSystem frame loop while the actual guest execution owner is held. It observes only the initialized executable scene and reads actor positions, original nerves and controller samples; it never sets actor state or story flags.

`SMGPC_DEBUG_ACTOR_TRACE_PATH` creates a new JSONL file (exclusive creation). `SMGPC_DEBUG_ACTOR_TRACE_INTERVAL` defaults to 60 frames. `SMGPC_DEBUG_ACTOR_TRACE_TYPES` filters C++ RTTI type names by comma-separated substrings; an empty filter records all LiveActors. Mario's original status, internal vectors and raw movement/draw state words are included. Actor names are decoded from CP932, with conversion failures recorded rather than silently replacing text.

The same frame observer can call the layout diagnostic at `SMGPC_DEBUG_LAYOUT_DUMP_FRAME` into `SMGPC_DEBUG_LAYOUT_DUMP_PATH`. The real runner records all of these environment values, clearing inherited values when disabled. Diagnostics are excluded from NDEBUG builds. The general synchronous observer exposed by OriginalGameApplication also allows a focused test to exercise actual scene-owned collision resources and validate their normal retirement.

## Validation

`../gateway-compat-20260919/original-trace-build1.log` records the complete main executable build passing in 23.16 seconds with the new trace/layout observers. An earlier focused target compile caught an incorrect KPAD include; it was corrected to the real `revolution/kpad.h` boundary. No Game source changes are part of this diagnostic.

The forthcoming run uses recorded controller input rather than physical keyboard automation. It is intended to distinguish controller publication, movement and original actor/demo progress. No story completion is claimed from compilation or frame count alone.

`movement-trace-baseline-3200.json` records 3,200 completed original GameSystem frames, exit 0, no timeout and no remaining PID (295.03 seconds; binary SHA256 `0389ccbf28d2e8873d2045facfa7a44fdc80eb473d87cda739081a62cf26d068`). The main binary contains the first coherent out-of-line display-list implementation, before later focused regression improvements, and the pre-fix layout graph. It ran without a debugger from fresh native save storage. This is a recorded debug controller replay, not a physical keyboard test.

All four controller stick directions reached original WPad and Mario input. Sampled movement during the four spans was approximately 847, 524, 1,538 and 1,004 world units net respectively. These are distances between samples within each span, not a claim about full paths or collision parity. Mario's sampled positions remain finite. The frame-3000 screenshot shows the camera following him around the planet near rocks. Original RunawayTico advances from Guide0 to Guide1 and retires; the chase collector and hidden rabbits remain waiting, and Rosalina remains dead. No rabbit catch or Rosalina appearance is established. The first filter omitted DemoRabbit, which is the guide rabbit and must be included in the next trace.

The frame-2300 layout dump shows three visible text/shadow sibling pairs (base, KrKo and CnSi) containing the same tutorial text. This confirms the native graph skipped original locale selection, and provides a comparison point for the restoration. Raw trace/layout/log bytes are preserved as gzip; the screenshot and launch/completion JSON are retained separately. `analyze_actor_trace.py` produces `baseline-summary.json` from the raw trace.
