# Exact FileSelect foundation wave

Date: 2026-08-08 UTC

## Outcome

This wave advances the retail FileSelect path without installing a parallel
title host, a synthetic player, or a silent service fallback.

- `InvisiblePolygonObj` and its GCapture variant are exact Game mirrors backed
  by generalized RARC/ResourceHolder/CollisionParts ownership and real KCL
  queries.
- `SphereSelector` and `SphereSelectorHandle` are reconstructed and exact Game
  mirrors. Their SceneObj, pointer-mode, layout, camera, math, scheduler, and
  teardown foundations live outside Game.
- `PlacementStateChecker` is an exact Game mirror. Stage StartInfo now owns and
  passes the real transformed JMap row inside the retail current-zone scope,
  before ordinary placement phases.
- `FileSelectItem` is functionally reconstructed at 99.80353% `.text` and is
  mirrored exactly on PC for source parity.
- The Mii font path uses real MiiFont26 BRFNT bytes, glyph metrics/sheets, and
  recursive FileInfo layout binding; removal/destruction invalidates the
  binding rather than retaining a copied fallback.

Production remains deliberately absent where the required implementation is
not real:

- `SphereSelectorHandle`: real atmosphere-level sound playback is missing.
- `FileSelector`/`FileSelectItem`: real Mii/RFL model rendering,
  CenterScreenBlur/FullScreenBlur, the complete save/item child graph, and the
  Mario auto-rush producer remain incomplete.

The exact Sphere and FileSelectItem translation units may compile or be kept as
source mirrors, but neither is advertised through the production factory past
those boundaries.

## Strict reports

- FileSelect scenario 1: 4 total / 2 complete / 2 blocked / 0 ignored.
  `FileSelector` is first; `SphereSelectorHandle` reports
  `atmosphere_level_sound_playback_runtime_unavailable`.
- HeavensDoor scenario 1: 242 total / 47 complete / 187 blocked / 8 ignored.
  `RestartCube` remains first. Report SHA-256:
  `38584ed1e4484db7634795d33b5f4117ae555b7c4dc19d44b03eb6def03e9d30`.

Both probes exit 1 by design before fabricating unsupported roots.

## Verification

See `verification.log` for the integrated commands and results. Detailed
component evidence is retained in the adjacent InvisiblePolygon, Sphere,
FileSelectItem, Mii-font, and stage-start note directories.
