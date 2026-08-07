# Factory-only stage placement

Date: 2026-08-07 UTC

## Invariant

An ordinary stage placement is constructible only when the compiled original
`NameObjFactory` table has a real creator for its object name. Finding an
`ObjectData/<name>.arc` or an archive alias from the original factory source is
resource metadata, not an actor implementation.

Non-renderable stage/area/camera/planet/demo helper tables remain explicitly
ignored. Every other row without a creator is reported as `Unsupported` with
`no_original_factory`.

## Changes

- Removed `GenericModel` and `GenericAliasModel` support kinds and their
  placement fields.
- Removed both host-side `ModelObj` construction branches.
- Limited archive descriptions and preloads to objects with real creators;
  their original mount-list and archive-table behavior is preserved.
- Limited stage root resolution, construction, debug `created` counts, and
  explicit-placement matching to real factory rows.
- Removed the synthetic `stage_name` actor created when a stage had zero real
  roots. Zero constructible placements now means zero actor roots.
- Prevented unsupported placements from loading KCL. Collision placement counts
  now describe only factory-eligible rows, the trace reports total placement
  rows separately, and archive/KCL lookup uses the direct object-archive path
  stem plus object name.

This change did not edit `pc-port/src/Game`; all policy removal is in the host
scene/name-object compatibility boundary.

## Regression coverage

`NameObjFactoryPlacementTests.cpp` covers:

- real creator vs synthetic unsupported classification;
- intentionally ignored helper-table classification;
- archive requests retained for a real factory and suppressed for unsupported
  names;
- unsupported construction throwing instead of synthesizing `ModelObj`;
- automatic real-disc discovery by walking workspace ancestors for RMGK01;
- dynamic direct-archive-only rejection (currently `AirBubble.arc`);
- dynamic original alias-only rejection (currently `Bomb -> BombHei.arc`);
- Gateway root filtering to real factories only;
- zero collision eligibility/archive loads for an unsupported archive-backed
  placement, while a real `Coin` factory placement remains eligible.

The candidates are selected dynamically or conditioned on the absence of a real
creator, so later actor imports turn into success/skip rather than stale test
failures.

See `verification.log` for the commands and results.

## Gateway route evidence

The full scripted route still reaches `HeavensDoorGalaxy` after title, the
six-slot file select, and all five picturebook pages. The honest post-removal
frame is intentionally sparse: unsupported actors no longer paint fake stage
geometry. Its gate therefore requires the real scene transition, real Mario
creation/render packets, and real factory actor packets in addition to the
placement report.

The 2026-08-07 capture reports:

- 242 placement rows;
- 35 real factory-created actors;
- 135 ordinary actor rows explicitly blocked as `no_original_factory`;
- 72 non-actor helper rows intentionally ignored;
- zero generic model or archive-alias construction statuses;
- 604 render packets, including Mario, Coin, TrickRabbit, and StarPiece;
- a 0.006986 non-black ratio, above the sparse-frame floor of 0.005.

This is a truthful incomplete Gateway, not a visual substitute for the 135
actors that still need their original implementations and factory entries.
