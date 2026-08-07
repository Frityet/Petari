# Forced scene-transition removal

## Policy

Scene changes must be requested by the real game sequence. A host environment variable, a debug key, or a host-side object-lifecycle trigger is not retail sequence behavior, so those mechanisms are absent rather than retained as a fallback.

`SceneTransitionRequestService` now accepts stage changes only from the source-backed `StorySequenceExecutor` flow:

- initial move type 7 selects the retail initial stage;
- the after-loading game-data request executes retail move type 6;
- `notify_scene_started` advances the executor's real scene-start nerve.

No `Game/` source was changed for this removal.

## Removed

- all `SMGPC_SCENE_TRANSITION_*` environment parsing;
- configured transition/trigger structures and the live-to-dead `NameObj` trigger tracker;
- the configured target and debug-key request members/branches;
- the F10 renderer mapping, debug input enum, RuntimeContext edge/pending state, and consume API;
- the application log advertising an F10 HeavensDoor teleport;
- the Aurora-native test for the external configured transition;
- the route-smoke environment block that forced the Gateway handoff (removed concurrently by the StagePlayer real-or-absent migration).

## Changed paths

- `src/scene/SceneTransitionRequestService.cpp`
- `src/scene/SceneTransitionRequestService.hpp`
- `src/runtime/RuntimeContext.cpp`
- `src/runtime/RuntimeContext.hpp`
- `src/render/RendererService.cpp`
- `src/render/core/RenderTypes.hpp`
- `src/app/Application.cpp`
- `tests/AuroraNativeTests.cpp`
- `scripts/aurora_route_smoke.lua` (concurrent StagePlayer task)

## Evidence

A source/test/script scan returns no matches for the removed transition variables, trigger/config types, pending debug-transition API/state, F10 teleport text, or debug transition input enum.

Validation results are recorded below after the shared exact `LodCtrl` migration becomes link-stable.

## Validation

- `git diff --check` for the changed paths: pass
- source/test/script removed-surface scan: pass (zero matches)
- focused and aggregate build/tests: pending shared `LodCtrl` completion
