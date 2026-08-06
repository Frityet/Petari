# General CommonPath placement metadata

Updated: 2026-08-06T20:05:14Z

## Outcome

The PC stage resolver now preserves the original association between a placement row and its `CommonPathInfo`/`CommonPathPointInfo.N` data. `MR::getRailInfo` can therefore expose the same header-row iterator and point table expected by the original `RailRider` constructor, without any HeavensDoor, RailCoin, or DemoRabbit special case.

This also fixes a broader placement lifetime defect. `StageHostScene` previously built actors from a local placement vector that was destroyed as soon as initialization returned, even though original Game objects are allowed to retain raw `JMapInfoIter` pointers. The scene now owns that vector until after its roots are destroyed, matching the lifetime supplied by the original `StageDataHolder`.

No root `Game/` source changed in this PC substrate slice.

## General compatibility behavior

- Rail metadata is stored per JMap entry, so two rows in the same placement or child-object table can refer to different rails.
- The resolver finds `CommonPathInfo` within the same placed zone, matches `CommonPath_ID` against `l_id`, and selects `CommonPathPointInfo.N` using the matched header row index. That mirrors `StageDataHolder::getCommonPathPointInfo` rather than assuming route-specific numbering.
- `pnt0`, `pnt1`, and `pnt2` are transformed through the zone placement transform once when the association is built. The original game performs the corresponding matrix application while reading each rail point.
- Attached header/point tables use shared ownership across `JMapInfo` copies; the scene-owned placement vector keeps the final attachment alive for actors that retain raw iterators.
- Child-object tables receive the same per-row association logic.
- Debug placement reports now include the common-path ID, attachment state, header row, point count, and transformed first point.

The compatibility storage lives in the PC `JMapInfo` implementation, while table resolution and ownership stay in the port-native scene layer. Original actor and rail code can consume the normal `MR::getRailInfo` interface.

## Native coverage

`rail info ownership and per-entry lookup` builds two independent rail associations, destroys the temporary source tables, copies the placement info, and verifies that both header iterators and point tables remain valid and distinct.

```text
xmake build smg-pc
[100%]: build ok

xmake test
[ok] rail info ownership and per-entry lookup
14 Aurora-native test(s) passed
100% tests passed
```

## Real-disc evidence

Raw transient run: `/tmp/smgpc-rail-metadata.gfd6ll/`

The normal FileSelect boot was used, then the existing F10 debug request loaded HeavensDoor scenario 1. Rendering was skipped for this metadata-only pass; the separately preserved full-sequence regression remains the visual proof for title, file select, and picturebook.

```text
stage=HeavensDoorGalaxy; scenario=1
objects=242; created=162; ignored=72; blocked=8
application result=0
fatal/segmentation/abort/crash matches=0
```

All route-relevant placed rails resolve through the same code:

- DemoRabbit cast 0: path 0, header row 0, 5 points;
- Middle-zone RailCoin: path 0, header row 0, 4 points;
- Inside-zone RailCoin: path 1, header row 1, 4 points.

The two RailCoin first points also match an independent transform of the raw disc coordinates:

| Placement | Raw first point | Zone translation / X rotation | Reported and independently calculated world point |
| --- | --- | --- | --- |
| HeavensDoorMiddleZone row 10 | `(10, -4.924067, -1910)` | `(2856.892334, 6490.821289, -7048.847168)`, `-12.300564°` | `(2866.89, 6079.10, -8913.95)` |
| HeavensDoorInsideZone row 19 | `(-15.637055, 0, -1151.307251)` | `(42840, -14537.606445, -2530)`, `-12.300564°` | `(42824.36, -14782.88, -3654.88)` |

Compact machine-readable evidence is under `artifacts/`. This slice intentionally leaves the placement frontier unchanged: importing the source-close rail/coin actors is the next layer.
