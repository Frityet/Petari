# Demo cast registration: optional `CastId` sentinel

Date: 2026-08-06

## Result

The host demo registry now accepts actors with a valid `DemoGroupId` when
`CastId` is `-1`. This matches the original data model and allows the Gateway
Rosetta placement (`DemoGroupId=0`, `CastId=-1`) to join its time-keep demo
group.

## Original-code evidence

- `src/Game/Demo/DemoDirector.cpp`, `DemoDirector::registerDemoCast`: validates
  the placement and `DemoGroupId`, constructs `JMapIdInfo` from that group, and
  attempts group registration. It does not reject `CastId=-1`.
- `src/Game/Demo/DemoCastGroup.cpp`: group membership is matched through the
  group/link identity; `CastId` is not a registration precondition.
- RMGK02 `HeavensDoorMysteriousZone` placement data gives Rosetta
  `DemoGroupId=0` and `CastId=-1`.

The generalized compatibility change is confined to
`pc-port/src/compat/DemoCompat.cpp`; no `Game/` actor source was changed.

## Verification

```text
xmake build smg-pc-aurora-native-tests
SMGPC_REAL_DISC=../RMGK01.iso xmake run smg-pc-aurora-native-tests
24 Aurora-native test(s) passed
```

The new native test also verifies the inverse case: a placement with
`DemoGroupId=-1` remains unregistered even if it has a nonnegative `CastId`.
