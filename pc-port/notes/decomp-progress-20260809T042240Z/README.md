# Decompilation and walking-demo frontier

Snapshot: 2026-08-09T04:22:40Z, RMGK02.

## Current decompilation report

`ninja build/RMGK02/report.json` regenerated the canonical report after the
MarioDamage and MarioFoo reconstructions landed.

| Scope | Fuzzy code similarity | Exact matched code | Exact matched functions |
|---|---:|---:|---:|
| Whole project | 79.87446% | 65.53285% (3,547,524 / 5,413,352) | 80.68085% (33,915 / 42,036) |
| `Game/` | 76.92337% | 62.87926% (2,608,008 / 4,147,644) | 79.84602% (28,521 / 35,720) |
| `Game/Player/` | 78.31112% | 33.87041% (159,512 / 470,948) | 64.30333% (1,043 / 1,622) |

The report has 2,219 total units and 734 complete units. Player has 107
configured units; 10 are currently complete by the report's strict unit
criterion.

“Fuzzy” is the best measure of how much source behavior/control flow has been
reconstructed. “Exact matched code/functions” is deliberately stricter: a
complete function can remain non-exact because the compiler selected different
registers, stack slots, literal pools, or relocations. Neither percentage by
itself claims that the code is already portable to or active in the PC build.

## Exact Mario source closure

The constructor-seeded retail Mario closure contains 96 Player translation
units. At this snapshot:

| Source gate | Units | Closure percent |
|---|---:|---:|
| Source present | 93 | 96.875% |
| Source absent | 3 | 3.125% |

The three absent units are `MarioHang`, `MarioShadow`, and `MarioTeresa`.
MarioHang reconstruction is already in progress. Source presence is necessary
but not sufficient: central `Mario.cpp` walking/update functions and several
large collision/model units remain incomplete even though their files exist.

The walking-critical `Mario.cpp` tranche now in progress covers `inputStick`,
`update`, `actionMain`, `updateGroundInfo`, `doExtraServices`, and
`checkForceGrounding`.

## Native PC frontier

The reproducible audit in
`../pc-player-compile-probe-20260809T035937Z/README.md` measures all 96 required
units against the current PC-first headers and Aurora surface:

| PC native gate | Units |
|---|---:|
| Parses natively now | 52 |
| Source present, syntax/provider surface blocked | 41 |
| Source absent | 3 |

The measured exact NameObj/LiveActor header tranche unlocks four more units;
the retail GX declaration tranche unlocks five; together they move the
compile frontier to 61/96 without regressing a baseline-pass unit. The actor
ABI tranche is now being implemented with host-only state moved to a
generalized external registry.

The remaining first-error distribution is led by J3D/model/animation (13
units), RVL/GX/GD (10), other exact Game APIs (9), and Binder/KCL (4). The full
retail closure has 830 external symbols across 146 non-Player provider objects.

## How close to a walking demo?

Source recovery is close; runtime integration is not finished. The bounded
milestones are:

1. finish the three constructor-required source units and central walking loop;
2. restore the exact NameObj/LiveActor ownership surface and retail GX/J3D
   declarations;
3. provide real J3D model/animation, Binder/KCL, HitSensor, resource, and effect
   behavior through generalized PC/Aurora services;
4. construct exact MarioActor through the existing StartInfo and MarioHolder
   lifecycle, remove host double-integration, and prove grounded stick motion;
5. use the permitted debug-only placement gate to show Gateway walking before
   the remaining production placements are all supported.

Already landed prerequisites include exact StartInfo ownership/order,
MarioHolder, keyboard-to-retail-stick input, the stationed Mario archive set,
and substantial walking/wall/slope/damage state reconstruction. Audio is
explicitly off this milestone's critical path.

## Reproduction

```text
ninja build/RMGK02/report.json
jq '.measures' build/RMGK02/report.json
python3 pc-port/notes/pc-player-compile-probe-20260809T035937Z/compile_probe.py
```

The exact full RMGK02 DOL remains byte-identical to the retail baseline after
the landed decompilation tranches.
