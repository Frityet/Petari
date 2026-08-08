# AreaObj runtime boundary

## Outcome

The PC stage host now has a scene-owned `AreaObjContainer` compatibility boundary instead of the former unconditional-unavailable stub. The original decompiled `Game/AreaObj/AreaObjContainer.cpp` is present byte-for-byte and excluded from the host build while its complete 67-manager specialized closure is unavailable.

The host implementation constructs only manager types represented by a complete descriptor. A descriptor carries the retail actor creator and manager name, original manager-table order, capacity, and manager creator in one record. The descriptor table is intentionally empty in this foundation commit; adding a manager alone cannot advertise actor placement support.

## Behavior retained

- Manager lookup uses retail prefix matching and first-match ordering.
- Missing manager and missing scene queries throw instead of returning a fabricated value.
- Managers are owned for the scene lifetime.
- Manager `initAfterPlacement` runs once, in construction order.
- Duplicate placement descriptors share one manager and must agree on order, capacity, and creator.
- Descriptor order is validated before any manager is constructed.
- Stage preflight treats an `AreaObjInfo` row as complete only when both the actor factory route and its descriptor exist. This is table-driven and contains no stage-name policy.
- Exact death/dark-matter helpers query their real AreaObj managers; absent specialized water and Mercator behavior remains explicitly unavailable.

## Source parity evidence

At capture time:

```text
cmp src/Game/AreaObj/AreaObjContainer.cpp pc-port/src/Game/AreaObj/AreaObjContainer.cpp -> identical
cmp include/Game/AreaObj/AreaObjContainer.hpp pc-port/src/Game/AreaObj/AreaObjContainer.hpp -> identical
sha256 AreaObjContainer.cpp -> 2a7d373345ad24e56c5d1d925590cb301a3e438f2a7464e943ca13fa0bd41347
```

## Validation

- Focused compilation passed for `AreaObjRuntime.cpp`, `AreaObjRuntimeCompat.cpp`, `SceneObjHolderCompat.cpp`, and `StageHostScene.cpp` through the normal `smg-pc-game` xmake target.
- `AreaObjRealOrAbsentTests.cpp` covers real SceneObj construction, missing-query rejection, scene ownership, one-shot manager lifecycle, prefix collision/first-match behavior, descriptor retail order, and the shared strict preflight predicate.
- The final focused target links cleanly and all six runtime tests pass; see `verification.log`.

## Integration contract

Future supported areas must be added only to `cCompleteAreaObjPlacementDescriptors` using a complete `AreaObjPlacementDescriptor`. The host NameObjFactory should obtain its AreaObj creator from `find_complete_area_obj_placement_descriptor`; it must not duplicate an independent AreaObj support list.

The five Gateway generic-manager order values in the retail manager table are:

```text
17 PullBackCylinder
18 RestartCube
32 ViewGroupCtrlCube
33 LensFlareArea
40 BlueStarGuidanceCube
```

## Final integration outcome

The integration following this foundation enables four complete, passive
Gateway descriptors through the same registry:

```text
17 PullBackCylinder       Cylinder  capacity 0x40
32 ViewGroupCtrlCube      Cube2     capacity 0x40
33 LensFlareArea          Cube2     capacity 0x40
40 BlueStarGuidanceCube   Cube2     capacity 0x10
```

`RestartCube` remains deliberately absent from the descriptor and factory
tables. Its exact source and generalized stage-session/audio foundations are
present, but production does not yet own a real Mario actor, call the retail
restart-area dispatcher, or provide audible stage-BGM playback. Advertising
the actor before that closure exists would violate the real-or-absent rule.

An RMGK01 HeavensDoor scenario-1 strict probe now reports 205 blocked rows
(down from 216), with `RestartCube` as the first blocker. The preflight rejects
the stage before placement construction, stage-audio start, or gravity
registration. See the separate `areaobj-stage-integration` evidence bundle for
the final commands and output summary.
