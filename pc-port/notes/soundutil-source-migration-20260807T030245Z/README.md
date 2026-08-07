# SoundUtil source migration

## Result

`pc-port/src/Game/Util/SoundUtil.hpp` and `SoundUtil.cpp` are now exact copies of the regular decompilation sources in `include/Game/Util` and `src/Game/Util`. The PC target does not compile the retail `SoundUtil.cpp` yet because the retail audio dependency closure is not present in the PC Game tree (the first unavailable include is `Game/AudioLib/AudAnmSoundObject.hpp`).

The host-facing implementation now lives in `pc-port/src/compat/SoundUtilCompat.cpp`. It forwards the sound operations currently used by the PC route to the generalized `RuntimeContext` audio-event service. Its declarations and definitions use the retail API types, including `JAISoundHandle*` returns, `u32` fade durations, and the single retail `startCSSound(const char*, const char*, s32)` overload. Since the host event service does not manufacture JAudio handles, handle-returning calls dispatch the real host event and return `nullptr` rather than inventing a fake handle.

The three sound functions that had accumulated in `GameRuntimeCompat.cpp` were moved into the dedicated compatibility unit. There are no stage names, route names, actor-name aliases, or other content-specific branches in the new bridge.

## Source identity

SHA-256 pairs:

- Header, root and PC: `8bf88b3c35899a555574a05d7da59a49b3b14e55b2bd312b4879aefce3eba344`
- Source, root and PC: `76789837626ea4470158fb7586dbb5e18d20a88376a8707cd5c03bbd38383a5b`

Both `cmp` checks passed. `compile_commands.json` contains `src/compat/SoundUtilCompat.cpp` and no `Game/Util/SoundUtil.cpp` entry.

## Route evidence

The `gateway_handoff` Aurora-native route passed after the migration. This scenario drives the main title, File Select, the five-page picturebook advance, and the explicit transition into `HeavensDoorGalaxy` scenario 1. The manifest reports:

- status: passed
- capture frame: 10350
- semantic events: 1150
- render packets: 431
- nonblack ratio: 0.006986
- real rendered models asserted: Mario, Coin, TrickRabbit, StarPiece
- placement summary: 242 total, 35 created by original factories, 72 intentionally ignored helpers, 135 explicitly blocked
- fabricated placement statuses: zero

The checked-in evidence under `route/gateway_handoff/` contains the manifest,
validation inputs/log, placement report, and captured PNG.  The generated
SQLite trace database, application log, Xvfb scratch log, and emulated save
directory remain local run artifacts rather than repository inputs.

## Verification

See `verification.log`. The aggregate `xmake test` attempt was interrupted by an unrelated concurrent collision-layer edit: `NameObjFactoryPlacementTests.cpp` still called a removed `StageCollisionService::load` method. The focused suites linked against this SoundUtil migration and passed.
