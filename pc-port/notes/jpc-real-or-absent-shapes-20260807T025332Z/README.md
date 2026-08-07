# JPC real-or-absent shape dispatch

Date: 2026-08-07 UTC

## Outcome

The PC particle renderer now submits geometry only for the JPA shape it actually
implements: retail shape type `2`, the ordinary/rotated billboard family.
Every other encoded shape type, and a particle whose required BSP1/SSP1 shape
metadata is absent, produces no geometry and no draw-packet trace.

This removes two substitutions:

- world-camera draws no longer coerce every non-billboard shape into a
  camera-facing `JpcWorldShapeFallback3D` quad;
- screen-space draws no longer coerce every non-billboard parent into a plain
  2D quad, nor route child types `4` and `8` through the synthetic segmented
  triangle strip.

The existing type-2 implementation remains unchanged. World-camera draw groups
use the camera-facing 3D GX material path; real 2D draw groups use the existing
screen-space billboard path.

## Source oracle

Root decompiled `src/JSystem/JParticle/JPAResource.cpp` dispatches each shape
family to distinct retail routines:

- type `0`: point;
- type `1`: line;
- type `2`: billboard or rotated billboard;
- types `3` and `4`: direction or rotated direction;
- types `5` and `6`: stripe and stripe-X emitter geometry;
- types `7` and `8`: rotation;
- type `9`: directional billboard;
- type `10`: Y billboard or rotated Y billboard.

The compatibility renderer currently has source-backed geometry only for type
`2`. Reusing that basis for another type changes retail behavior, so those
families remain absent until their own routines are implemented.

The earlier source/Dolphin evidence is preserved in
`pc-port/notes/dolphin-gateway-jpa-oracle-20260806T231821Z/`. In particular,
Gateway `Steam00` is type `4` direction geometry while `Steam01` is type `2`
billboard geometry. This cleanup therefore preserves the real `Steam01` path
and stops inventing `Steam00` geometry.

## Implementation

- `jpc_particle_packet_path` now returns an optional path. It returns a 2D or
  world billboard path only for shape type `2`, and no path otherwise.
- `EffectService::draw` resolves shape metadata before texture or vertex work
  and skips a particle when its metadata/path is absent.
- Missing base or child shape metadata is no longer silently treated as type
  `2`.
- The fallback enum/name and synthetic child display-list shape builder were
  removed.
- The focused test exhaustively checks all 16 values representable by the
  parsed four-bit JPA shape field in both screen and world modes.

No file under `pc-port/src/Game` was changed.

## Verification

See [`data/verification.txt`](data/verification.txt) for the command output.

- focused target build: passed;
- 5/5 focused JPC billboard tests: passed;
- full `smg-pc` build: passed;
- `git diff --check`: clean.

## Deliberate remaining gap

Types `0`, `1`, and `3` through `10` remain simulated as particles but are not
rendered. Implementing one later requires its corresponding retail basis or
primitive construction and focused oracle evidence; it should then be added as
an explicit supported path rather than a generic substitute.
