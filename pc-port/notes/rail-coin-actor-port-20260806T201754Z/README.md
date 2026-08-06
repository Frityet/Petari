# Coin / RailCoin actor port

## Scope

- Imported the regular `Coin`, `CoinGroup`, `CoinHolder`, `CoinRotater`, and `RailCoin` headers and implementations into `pc-port/src/Game/MapObj`.
- Corrected only the case-sensitive `Game/Mapobj/RailCoin.hpp` include to `Game/MapObj/RailCoin.hpp` for the Linux host build.
- Imported `PurpleCoinHolder` and `PurpleCoinStarter` from the regular decomp and connected their shared scene services.
- Reconstructed four generic functions that were absent or commented out in the regular decomp before mirroring them into the PC copy:
  - `CoinHolder::appearCoinPopToDirection`
  - `CoinHolder::appearCoinCircle`
  - `CoinHolder::appearCoin`
  - `CoinRotater::movement`

No stage names, route coordinates, HeavensDoor placement IDs, or other route-specific behavior were added.

## Root reconstruction evidence

The missing behavior was reconstructed from the RMGK02 target objects in `build/RMGK02/obj/Game/MapObj` and compiled with the regular Metrowerks configuration.

| Function | Target size | Fuzzy match |
| --- | ---: | ---: |
| `CoinHolder::appearCoinPopToDirection` | 168 bytes | 99.88095% |
| `CoinHolder::appearCoinCircle` | 372 bytes | 97.41936% |
| `CoinHolder::appearCoin` | 332 bytes | 99.93976% |
| `CoinRotater::movement` | 248 bytes | 94.758064% |

The reconstructed behavior includes the original pooled-host accounting, random velocity spread, coin-appearance sound/ME selection, gravity-relative circular scatter, and the normal/water/high-speed rotation matrices (+8/+4/+16 degrees per frame, wrapped to 360 degrees).

Focused build:

```text
ninja build/RMGK02/src/Game/MapObj/CoinHolder.o build/RMGK02/src/Game/MapObj/CoinRotater.o
[1/2] MWCC build/RMGK02/src/Game/MapObj/CoinHolder.o
[2/2] MWCC build/RMGK02/src/Game/MapObj/CoinRotater.o
```

The default RMGK02 build remains exact:

```text
54b71431af0d509097bfdef4ec28617afc487e89  build/RMGK02/main.dol
54b71431af0d509097bfdef4ec28617afc487e89  orig/RMGK02/sys/main.dol
```

## PC source fidelity

The ten primary PC actor files are byte-equivalent to their regular root counterparts except for:

- the required `Mapobj` -> `MapObj` include-case correction in `RailCoin.cpp`;
- a final newline in `CoinRotater.hpp`.

The two HeavensDoor `RailCoin` placements use the generic normal-rail branch. The source still retains the original Mercator branch and purple-coin branch; those branches were not removed or bypassed.

## Shared provider inventory at import time

These were the compatibility/runtime responsibilities identified when the actors were imported. They should not be implemented as actor- or
stage-specific workarounds. Parallel compatibility work has since supplied many of them; the current math/gravity evidence is recorded below.

### `Coin.cpp`

- Scene/lifecycle: `connectToSceneItemStrongLight`, mirror actor creation.
- Sensors/messages: generic `addHitSensor`, `ATYPE_COIN`, sensor-radius mutation, item get/pull/show/hide/start-move/end-move and black-hole message predicates.
- Binder/collision: `LiveActor::initBinder`, binder exclusion, bind on/off, no-bind/roof/wall/ground/damage-fire/pressed checks, contact normals, rebound, velocity zero/attenuation, and death-volume query.
- Clipping/shadow: far-100m clipping, clipping validation, shadow-inclusive clipping range, all surface/volume shadow initialization and drop configuration, validation/invalidation, one-time/continuous calculation modes, and private-gravity drop modes.
- Gravity/math/water: actor Y axis, gravity enable/calculate, vector near-zero/normalize helpers, water query.
- Model/parts: `createPartsModelNoSilhouettedMapObj`, `PartsModel::initFixedPosition`, show/hide model.
- Event/player/result: dark-comet query, coin/purple-coin increments, player oxygen increment.
- Focused utility exposure/providers: demo simple-cast registration, system/actor sound, existing Nerve helpers, and original `Game/Util.hpp` umbrella coverage.

### `CoinGroup.cpp`

- Demo registration/action/request/end functions.
- Actor camera initialization/existence/start/end functions.
- System sound defaults and `isGreaterStep` for `LiveActor`/`NerveExecutor`.

### `CoinHolder.cpp`

- A future PlanetGravityManager-equivalent positional gravity resolver for pop/circle behavior in non-flat fields.
- Dark-comet and sound/ME queries/submissions.
- `SceneObj_CoinHolder` / `SceneObj_CoinRotater` IDs, storage, creation, and lifetime.

### `CoinRotater.cpp`

- `SceneObj_CoinRotater` service.
- The general Y-rotation matrix provider is declared in the imported `MtxUtil.hpp`; its host definition is now linked by the shared compatibility work.

### `RailCoin.cpp`

- `AreaObjUtil::isInAreaObj` declaration/provider.
- Mercator division is preserved and routed through a generic evenly sampled rail fallback until the full area/transform services are hosted.
- RailRider and the normal RailUtil query/movement surface are owned by the parallel rail-core integration.

### Purple coin branch

- `PurpleCoinHolder`, `PurpleCoinStarter`, their factory entries, and the related indexed scene-object service are now integrated. Their PC Game implementations retain the regular source shape.

## Validation status

- Regular RMGK02 focused objects: compile successfully.
- Regular RMGK02 default DOL: exact SHA-1 match.
- PC actor sources: imported and syntax enumeration performed.
- PC combined native-test binary: builds and all 17 tests pass.
- Full title -> file select -> five-stop picturebook -> HeavensDoor route: exits 0, creates both path-backed `RailCoin` groups, and renders 24 member-coin packets at frame 10350. See `../rail-coin-runtime-20260806T203306Z/`.

## Math and gravity compatibility follow-up

The focused PC compatibility layer now supplies the original source-shaped behavior required by `Coin.cpp` and `CoinHolder.cpp`:

- random vector generation/addition;
- scalar/vector near-zero checks and normalize/normalize-or-zero overloads;
- gravity-relative `makeAxisVerticalZX` construction;
- degree rotation about a normalized arbitrary axis;
- both `calcReboundVelocity` overloads;
- canonical `TVec3f::setLength` behavior;
- `calcGravityVector`, `calcGravityVectorOrZero`, and `calcGravityOrZero` host entry points.

The rebound formulas were recovered from the RMGK02 `MathUtil.o` calls and dataflow. The four-argument form splits the incoming velocity into tangent
and normal components, scales them independently, and reverses the normal component. The three-argument form applies the equivalent normal impulse.

Gravity intentionally has a strict fallback while there is no host PlanetGravityManager: a `LiveActor` query normalizes its existing authoritative
`mGravity`; a positional query on an arbitrary `NameObj` returns `false` and a zero vector. `calcGravityOrZero` retains the original grounded-normal
fallback. This avoids silently claiming world-down at arbitrary stage positions. Static `RailCoin` actors remain correct because `LiveActor` starts
with the original `(0, -1, 0)` gravity. `CoinHolder` pop/circle emission awaits a real stage gravity-field host for nonzero positional queries.

The deterministic native test covers zero/nonzero `setLength`, normalization, both axis-selection branches, right-handed axis rotation, both rebound
forms, random-vector bounds, actor gravity normalization, strict positional zero gravity, and the grounded-normal fallback. The complete run is saved
in `native-test-20260806.log`.
