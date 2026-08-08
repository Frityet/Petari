# Retail stage-start lifecycle boundary

This wave retains the selected `StartInfo` as an owning `JMapInfo` row and passes that exact row to the normal `NameObj` lifecycle. It does not create a proxy player and does not advertise `Mario` while the real player closure is unavailable.

## Retail behavior restored

- `StageStartInfo` owns its source BCSV after the resolved stage tables are destroyed.
- The owned row is transformed through its child-zone matrix before exact Game code reads `pos_*` and `dir_*`.
- Creator lookup follows `MR::getObjectName`: `type` takes precedence over `name`.
- The exact `PlacementStateChecker` is a required scene object.
- Creator construction and `init(iter)` execute inside an exception-safe current-placement-zone scope.
- The selected player row is initialized before the retail high-priority/common/scenario and ordinary/common/scenario placement phases.
- Source-registered `CollisionParts` are built before `initAfterPlacement` queries and rebuilt after all post-placement callbacks.
- Missing StartInfo names are rejected only after the ordinary strict placement report has had priority.

The actor name passed for the player row remains the retail literal `マリオアクター`. No title, player, collision, or placement state is synthesized.

## Source boundary

`pc-port/src/Game/Scene/PlacementStateChecker.{hpp,cpp}` are byte-identical to the root decomp. All ownership, ordering, exception safety, and host service integration lives under `pc-port/src/scene` and `pc-port/src/compat`.

## Verification

See `verification.log`. The exact source boundary, production translation units,
real-disc StartInfo selection, integrated debug application, and strict
FileSelect preflight are green. FileSelect remains honestly unavailable at its
two real runtime blockers; no StartInfo or placement root is fabricated to get
past preflight.
