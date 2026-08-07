# FootPrint and textured direct draw decompilation

Date: 2026-08-07 UTC

## Why

The real rabbit/Tico Gateway actor closure uses `FootPrint`. This replaces the
remaining assembly-only blocker with regular root decompilation source; it does
not add a PC-side stand-in and does not edit `pc-port/src/Game`.

## Source added

- `FootPrint` constructors, initialization, lifetime, print insertion and ring
  state, movement expiry, textured draw, clear/invalidate/query methods;
- `FootPrintInfo` layout and construction;
- `TDDraw::drawTexture3D`, including retail texture flip and quad ordering;
- the correct RMGK01 `.data` ownership boundary between `FootPrint` and
  `FurCtrl`.

The old split assigned `FurCtrl` Japanese layer/manager strings at
`0x805E1058..0x805E1208` to `FootPrint`. Moving that range to `FurCtrl` is
required when linking source-built `FootPrint`.

## Match evidence

Source-link comparison measured `FootPrint` `.text` at 99.93814% and
`TDDraw::drawTexture3D` at 99.896904%. Remaining deltas are compiler
constant-pool relocation-label identities; instruction behavior and the
`FootPrint` `.sdata2` bytes match. The standard RMGK02 build still reproduces
the exact expected DOL SHA1.

See `verification.log`.
