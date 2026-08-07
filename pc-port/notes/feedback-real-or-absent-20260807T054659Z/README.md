# Feedback services: real or absent

Timestamp: 2026-08-07T05:46:59Z

## Outcome

- Rumble is now a real host service. It consumes the untouched retail `RumblePattern` table and drives Aurora's existing SDL controller/device haptic path through `WPADControlMotor`.
- Unknown named patterns, unavailable controller motors, invalid channels, duplicate active source/pattern pairs, and exhausted retail channel capacity return `false`. None are converted to a different strength or recorded as successful.
- Camera shake now reproduces the seven retail amplitudes (`0.08, 0.2, 0.5, 1, 3, 6, 9`) and the 25-frame damped sine from `CameraShakePatternSingly`.
- The raw shake offset is converted to projection space with the RMGK02 `CameraShaker::adjustOffsetToScreen` scaling: X uses `30 / MR::getScreenWidth()` and Y uses `30 / renderMode.efbHeight`.
- Runtime width provenance is exact: a base camera aspect of `608 / 456` maps to width 608, `16 / 9` maps to width 832, and any other or absent projection size leaves shake explicitly unavailable.
- The projection offset reaches the GX projection matrix, CPU J3D projection, star-pointer projection, and camera project/unproject compatibility paths.
- Effect deletion now requires the actor's real registered host effect keeper. Missing keepers and missing required runtime state throw instead of recording a delete event or silently returning.

## Game-source boundary

- `Game/Util/ObjUtil.cpp` no longer contains the PC rumble or normal-camera-shake implementations.
- `Game/Util/SystemUtil.cpp` no longer contains the PC-only `NerveExecutor` rumble overload.
- `Game/Util/SystemUtil.hpp` is byte-identical to `include/Game/Util/SystemUtil.hpp`.
- `Game/System/WPadRumbleData.hpp` is byte-identical to `include/Game/System/WPadRumbleData.hpp`.
- The untouched root `src/Game/System/WPadRumbleData.cpp` is compiled by `compat/WPadRumbleDataSource.cpp`; host motor ownership remains in compat/Aurora.
- The retail-strength wrappers and required camera wrappers live in `compat/GameRuntimeCompat.cpp`.

## Aurora actuator

Aurora commit `afdf18076d3588a3a844bd5a1c77b02ddb22ce1a` (`Add real WPAD rumble bridge`) was pushed to `origin/main`.

The bridge adds a read-only capability query covering SDL gamepad rumble, GameCube-controller rumble, and Aurora's device-haptic route. `WPADControlMotor` delegates to the existing PAD motor implementation. Hardware-unavailable `try` requests return `false`; they do not complete successfully according to fabricated retail semantics.

## Verification

See `test-output.txt` for the captured concise results.

- `xmake build smg-pc-feedback-real-or-absent-tests`
- `xmake run smg-pc-feedback-real-or-absent-tests` — 4/4 passed
- `xmake build smg-pc-aurora-native-tests`
- `xmake run smg-pc-aurora-native-tests` — 26/26 passed
- `xmake build smg-pc` — passed
- `git diff --check` on the scoped top-level paths — passed
- `git -C pc-port/aurora diff --check` before commit — passed

No top-level files were staged or committed. Protected SaveIcon and TriggerChecker files were not touched, and no Demo or Layout Game source was edited.
