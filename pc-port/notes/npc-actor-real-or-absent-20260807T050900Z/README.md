# NPCActor exact-source / real-or-absent migration

## Outcome

`pc-port/src/Game/NPC/NPCActor.cpp` is a byte-identical copy of
`src/Game/NPC/NPCActor.cpp`, and `pc-port/src/Game/NPC/NPCActor.hpp` remains a
byte-identical copy of `include/Game/NPC/NPCActor.hpp`. The port no longer links
the old 284-line replacement body from `NPCActorCompat.cpp`; that translation
unit now compiles the complete 859-line decompiled source through
`compat/NPCActorSource.inl`.

The direct Game source is excluded in `Game/xmake.lua` only to avoid compiling
the same definitions twice. The compile bridge adapts the host `LiveActor`
storage mismatch outside Game: model and StarPointer presence are queried from
their real runtime registries, while the retail-named `mSpine` is accessed by a
translation-unit-local private-member bridge. No accessor or NPC workaround was
added to `Game/LiveActor`.

The required Game dependency headers were copied byte-for-byte from the root
decompilation:

- `Game/Enemy/AnimScaleController.hpp`
- `Game/NPC/NPCActorItem.hpp`
- `Game/Util/JointController.hpp`
- `Game/Util/NPCUtil.hpp`

The generalized JGeometry layer gained the retail `TUtil::acos` and
`SMatrix34C::setInline` operations needed by the exact source.

## Real behavior supplied

- quaternion construction/turning and matrix translation/scaling use actual
  numeric state;
- base matrices are installed through the existing live-actor renderer state;
- gravity uses the active real gravity service;
- BCK existence checks query parsed model animation resources;
- ordinary and indirect NPC goods require a real object archive and, when
  requested, a real named joint matrix;
- placement transforms come from the real JMap iterator;
- model and StarPointer presence checks query their actual registries;
- the exact NPC reaction/cooldown, nerve, sensor-message, and lifecycle code is
  the linked Game implementation.

## Explicitly absent behavior

The following APIs throw `std::logic_error`; none returns a fabricated success,
default state, or no-op result:

- NPC item parameter-table lookup;
- NPCUtil reaction/talk/action orchestration;
- `GroupCheckManager` SearchTurtle attributes;
- real MarioActor swing state;
- J3D joint-controller callbacks and `AnimScaleController` construction;
- joint-bound StarPointer targets and second-player StarPointer input;
- random writable BCK frame selection;
- shadow CSV/controller ownership and queries.

`DemoRabbit` calls `NPCActorCaps::setDefault()` and therefore requires several
of those absent systems. Its creator and archive callback were removed from the
placement factory. The strict Gateway route now expects all three DemoRabbit
rows to be `blocked`, not created, and no longer expects a TrickRabbit render
packet. This prevents an actor that cannot initialize from being advertised or
partially constructed, and prevents the main link from extracting DemoRabbit's
NPCActor closure through the factory table.

## Validation

Completed before the final concurrent utility-header churn:

- `xmake build smg-pc`: pass (exact NPCActor linked)
- `smg-pc-npc-actor-real-or-absent-tests`: 4/4 pass
- `smg-pc-live-actor-util-real-or-absent-tests`: 3/3 pass
- `smg-pc-player-util-real-or-absent-tests`: 4/4 pass
- `smg-pc-actor-sensor-real-or-absent-tests`: 6/6 pass
- `cmp` for NPCActor source/header and all four copied dependency headers: pass
- `git diff --check`: pass

The final NPC rerun was prevented by a concurrent, unrelated compile error in
`PlanetGravityCompat.cpp`: the newly changing MathUtil header did not yet expose
`MR::separateScalarAndDirection`. The prior NPC binary itself passed 4/4; no NPC
failure was observed.
