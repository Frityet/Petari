# Original animation-driven pad and camera controller

Recovered the complete previously absent root `src/Game/LiveActor/ActorPadAndCameraCtrl.cpp`: five class methods, two anonymous helpers and the original `sFileName` data. This closes an original-source dependency of `LiveActor::initModelManagerWithAnm`; it does not import or activate the class in the PC build.

The existing header's `_C` field is corrected from `u32` to `const char*`. Retail `update` compares and stores the actual return of `ModelManager::getPlayingBckName`; `updateInfoBck` passes that value to `MR::isEqualStringCase`. Its Wii offset and size remain unchanged. This is an authored BCK-name pointer, not an integer ID or truncated native pointer.

## Original compiler and retail proof

Run from the repository root:

```sh
python3 pc-port/notes/original-actor-pad-camera-ctrl-20260903/verify-original.py
```

The script compiles the complete real root TU with GC3.0a3 and the configured Game flags, verifies root structure layouts, compares the retail split object with objdiff, and resolves every generated relocation against the supplied RMGK01 DOL. Its SHA1 is `25c5959534b3c21246c6c7e42021b916b41fb578`. Commands, objects and detailed comparison output remain in ignored `build/original-actor-pad-camera-ctrl-20260903/`; `source-evidence.json` records the durable source hashes and checks.

| Method | Retail address / bytes | Objdiff |
| --- | --- | ---: |
| Constructor | `0x8015C8C8 / 0x21C` | 95.888885% |
| `tryCreate` | `0x8015CAE4 / 0x94` | 100% |
| `update` | `0x8015CB78 / 0x114` | 98.695656% |
| `updateInfoBck` | `0x8015CC8C / 0x94` | 98.91892% |
| `tryUpdateCameraShake` | `0x8015CD20 / 0xFC` | 99.68254% |
| `isDistanceExistAndFar` | `0x8015CE1C / 0x24` | 99.44444% |
| `updatePadAndCamera` | `0x8015CE40 / 0xF0` | 100% |

All **410 retail instruction words** agree after the verifier's listed transformations. Four methods agree after ordinary relocation alone. Constructor/update/updateInfoBck additionally differ in physical working-register assignment; the constructor moves two loop-local zero initializations across the same positive-count guard and copies another flag zero from a different already-zero local. The verifier explicitly checks that seven-instruction region, its branch target, and the complete remaining instruction streams. It does not discard helper calls, field reads/writes, comparisons, branches or floating operations. This is functional source correspondence, not an unnormalized byte-exact claim for the whole TU.

The entire 222-byte authored string pool at `0x805877A0` matches, including file/key spelling and Shift-JIS shake names. The writable `sFileName` pointer at `0x806B1B20` targets its original `PadAndCameraCtrl` string. The three short shake strings and both float constants are separately checked in retail small data. Layout checks cover the 24-byte controller, 52-byte row, pointer field and both row flags.

## Preserved behavior

`tryCreate` asks the actual ModelManager for its resource holder and creates the controller only if `PadAndCameraCtrl.bcsv` exists and the original motion table has at least one entry. It does not substitute a null controller for a supported authored file. A present file with zero rows still creates the ordinary empty controller. Construction initializes the controller fields, loads the actual CSV, allocates its full row array, and reads the original fields in their retail order.

The row keys are `BckName`, `StartFrame`, `EndFrame`, `PadRumbleName`, `CameraShakeName`, `DistanceNear`, `DistanceFar`, `DistanceInvalid`, `PadRumbleNameMiddle`, `PadRumbleNameFar`, `CameraShakeNameMiddle`, and `CameraShakeNameFar`. The original string getters and their null conversion are retained. Only `DistanceInvalid` receives an explicit default, **3000.0f**, before the optional direct JMap lookup. There is no invented default initialization for other mandatory data, replacement CSV interpretation, or stage-specific table.

`update` detects BCK changes by pointer identity and calls `getPlayingBckName` twice on a change, as retail does. `updateInfoBck` case-insensitively matches each non-null row name. It does nothing when the current name is null, leaves a null row name's prior match flag alone, and does not clear the row's active interval flag on an animation change.

The actual ModelManager BCK `J3DFrameCtrl` owns time. Passing the start frame sets the active flag; every active matching update requests its authored pad/camera event. The request occurs **before** the active flag is cleared for a negative end frame, a non-positive frame rate, or a passed end frame. Original `checkPass` supplies wrap/reverse/frame semantics. This controller never advances animation time itself and introduces no native frame counter.

Distance is the original `getPlayerPos()->distance(*actorPosition)` call, which compiles to `PSVECDistance`. A negative threshold disables that threshold; otherwise the helper uses strict `distance > threshold`. Far is tested before near, so equality remains in the closer band. Each chosen pad pattern is requested on channel 0 with the controller pointer as the request source. The seven authored camera strings map directly to original VeryStrong/Strong/NormalStrong/Normal/NormalWeak/Weak/VeryWeak shake helpers; null or unrecognized names do nothing, exactly as retail.

**Retail reads and compares `DistanceInvalid`, but discards the boolean result.** At `0x8015CE7C` it calls the threshold helper, then loads the far threshold and calls the same helper at `0x8015CE88` without testing the first result. The recovered source preserves that otherwise ineffective comparison. It does not add a distance cutoff.

## Actual owner and native integration boundary

Root `LiveActor.cpp:330–338` constructs and initializes the real ModelManager, applies base scale/TR, calculates the actual model, then creates ActorAnimKeeper and this controller. The borrowed ModelManager, actor position, CSV-backed names and actual Xanime frame controller must outlive it. The original Game heap owns the row/parser allocations; this class adds no destructor or standalone replacement allocation policy.

Root LiveActor movement advances ModelManager/ActorAnimKeeper at its existing initial animation phase, executes actor control and Binder, then updates effects and this controller before light/sensor updates. Death checks can leave the movement method before the event phase. The animation-stop flag gates the earlier ModelManager update, not a new independent controller tick. Native integration must preserve this sequence and consume the same actual lower-player frame controller used by the model/animator.

Required call boundaries are original ModelManager `getResourceHolder`, `getPlayingBckName`, `getBckCtrl`; actual ResourceHolder motion table and retained authored CSV/JMap; original CSV/file/string helpers; `J3DFrameCtrl::checkPass`; player position/PS distance; rumble and seven camera-shake helpers. These declarations and the model methods exist in root. The concurrent parent task has restored the previously missing `ObjUtil.cpp` scalar CSV getters using the actual JMap templates and owns their native provider slice. No substitute method is supplied by this controller tranche.

The current native LiveActor still initializes `mCameraCtrl` to null and does not execute this original owner lifecycle. Its existing general rumble/shake service providers are not evidence that the authored controller is active. No native header/source import, shared build, GPU run or live animation/camera-event claim is made here.
