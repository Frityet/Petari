# Exact CubeCamera and MessageArea Gateway closure

## Outcome

The PC port now compiles the exact decompiled `CubeCamera` and `MessageArea`
Game sources unchanged. Four retail CubeCamera forms share the specialized
order-4, capacity-0xA0 `CubeCameraMgr`; its exact `initAfterLoad` priority sort
is dispatched once by a generalized compatibility-layer manager finalizer.
The two MessageArea forms share the ordinary order-42, capacity-0x10 manager.

No CameraManGame behavior, player state, route-specific actor, or fallback
model is claimed. Strict construction remains intentionally absent at the
first incomplete retail actor, `RestartCube`.

## Exactness

- `CubeCamera.cpp`: `59d9377bcb4ad661bc1b6265bc4344dcbff4e4da9551b491b57de1adae8e7bfc`
- `CubeCamera.hpp`: `53e198e5ef30ddd98a86deccb5db99ebdc9d8aee0b63cb83ea983f91181fb782`
- `MessageArea.cpp`: `5142caa6aea3822e20b6217f055b0786433605d0657c32f631661a2a9633d958`
- `MessageArea.hpp`: `cca505e2a1c3f742dfe39e421749c5465c1bdc7c64802c1643cea5bf0dd09421`

Each digest is shared by the root decomp source and its `pc-port/src/Game`
mirror. RMGK02 objdiff reports CubeCamera at 1088/1088 code, 112/112 data,
12/12 functions and MessageArea at 228/228 code, 48/48 data, 3/3 functions.

## Real-disc verification

The focused AreaObj runtime suite passed 10/10 against `RMGK01.iso`, including
construction of all 16 CubeCamera rows and both MessageArea rows. The complete
Aurora suite passed 35/35 targets with the real-disc fixtures enabled.

A fresh strict Xvfb probe wrote [placement-report.md](placement-report.md)
before rejecting construction. Its schema explicitly says `phase: preflight`:
`complete` means the exact creator/manager runtime route is present, not that
the actor was constructed.

- 242 total rows
- 47 complete retail routes
- 8 proven non-actor helpers
- 187 blocked actor-bearing rows
- first blocker: `RestartCube`

This is an exact improvement from 205 blocked: 16 CubeCamera rows and two
MessageArea rows. The full report SHA-256 is
`38584ed1e4484db7634795d33b5f4117ae555b7c4dc19d44b03eb6def03e9d30`.
[verification.log](verification.log) records the commands and outcomes.
