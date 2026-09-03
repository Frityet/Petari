# Native integration results

The full native import builds on macOS arm64. Source recovery root commit:
7b0874008. Actual Game Binder and collision-zone cpp files are byte-identical
to root; checked headers are detailed in source-parity.json.

The real-disc smg-pc-showcase smoke passes 28 rendered frames, ordinary
PlanetMap, animated Mario packets, visible actor center, GPU submission,
probe gravity acceleration and planet KCL contact. The frame20 screenshot
showcase.png was inspected; it shows Mario and the Gateway terrain. This
smoke does not certify stable player grounding or full demo completion.

The actual original-Binder smg-pc-mario-gateway-walk-tests run passes the
neutral setup, real Mario state callback tests and begins Run, then fails
at the terrain seam after frame157 with the existing strict grounding
assertion. That assertion is retained. The earlier frame203 release failure
in the state notes was from the intermediate native Binder, before the full
import. Neither failure is reported as a passing walk proof.

The next player work must restore the actual floor-query/state/writeback
path (plus its required real animation owner), because retail Mario can be
grounded without current Binder contact. See mario-grounding-audit.md.

Initial camera failures came from missing collision ownership in geometry-free
fixtures and a stale exact-unit expected value after restoring Wii vector
normalization. Fixture corrections and rerun results are recorded separately;
production camera code was not modified to make those fixtures pass.

integration-results.json records the initial outcomes and logs. Passed
standalone integration checks include original camera runtime, OnlyCamera,
view interpolation/service, collision registration and triangle filtering,
spin checkpoint, Binder/KCL and all 29 Aurora groups. The StageStart and
ActorEvent reruns are appended when the fixture corrections are complete.
Logs and the screenshot remain local rather than committed.

Both camera fixture reruns pass with the real disc: StageStart14 and
ActorEvent23, bringing OnlyCamera/view/stage/event coverage to59 groups, plus
the separate original-camera-runtime target. Explicit empty collision scenes
now own the no-geometry camera fixtures. The handoff test compares the prior
rendered OnlyCamera/view output (unchanged before the event phase), not an
un-normalized manager pose. No production camera modification was needed.

The final checkpoint retains one failing integration target: the full real
Gateway walk at the seam. This is documented progress toward restoring the
original player pipeline, not a completed Gateway movement/demo claim.
