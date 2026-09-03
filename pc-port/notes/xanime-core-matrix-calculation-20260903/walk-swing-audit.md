# Intermittent real-window swing assertion

The first full walk regression in `mario-gateway-walk.log` failed the compound
locked-swing assertion. That run did not record which component failed. The
parent added failure-only diagnostics and reran the same production code;
`walk-diagnostic.log` ends with `[ok] real Gateway Mario stand/walk/release proof`.
It records walk distance 325.684, 14,521 KCL triangles, release frame 297, 12 Run
packets, and 115.644 units of camera watch movement. No failure diagnostic was
emitted on that rerun. The original failure remains unisolated.

A bounded source audit found a concrete input-interference boundary, not a
confirmed cause:

- `MarioGatewayWalkTests.cpp::set_host_key` pushes one event into the shared SDL
  queue and immediately calls `window.poll_events()`. That poll drains both the
  injected event and real pending desktop input. The test's `run_frame` does not
  poll SDL, so native input may accumulate between injected transitions.
- `AuroraWindow::Impl::process_sdl_event` retains the last processed state for
  each mapped key/button. A real X key-up processed after the injected X key-down
  can clear its level. Physical keyboard or mouse events may also set A/B.
- Original `MarioActor::updateControllerSwing` first tests the A/B triggers. A
  trigger resets `_F1C` to 10, which suppresses `_F00` on that frame even if WPAD
  correctly reports a fresh swing. The preceding scripted C input does not use
  this right-controller A/B suppression path.
- Focus loss alone neither clears the native `pressed` array nor gates its
  getter. Current Aurora forwards SDL events after ImGui processing without
  consuming keyboard capture here. Those mechanisms are therefore not proven
  explanations for the failed assertion.

WPAD publication order is correct: `begin_frame` captures the previous swing,
then RuntimeContext publishes the current window level before camera and actor
movement. Trigger calculation is current level and not previous level. The new
core/quaternion calculation code does not write the WPAD or Mario swing fields.
No source-backed direct path from its matrix arithmetic to the failed input
predicate was found.

The retained failure diagnostics are useful. If the assertion recurs, `_F1C`
and WPAD A/B trigger bits distinguish the original suppression path from a
missing X level/edge. This audit changed no production input code, relaxed no
assertion, and launched no additional test. The passing rerun should be reported
together with the single earlier unisolated failure.
