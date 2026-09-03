# Original OnlyCamera import and complete retail audit

The PC `OnlyCamera.cpp` and `OnlyCamera.hpp` are exact copies of the root files. No root recovery, algorithm change, host adaptation, or additional provider was needed. The original source was last changed by `b3d02bedd657534f007633ade7a6d0c21852a092` (`Match OnlyCamera`). This checkpoint verifies the current source against the retail binary directly, rather than relying on that historical matching label.

## Original compiler verification

`verify-object.py --compile` compiles the real root translation unit using `build/tools/wibo`, `build/compilers/GC/3.0a3/mwcceppc.exe`, and `configure.py`'s complete `cflags_game` for RMGK01 / VERSION=0. It uses the configured real include hierarchy without a generated source/header overlay. Compilation completed without diagnostics.

The verifier resolves each external function and vtable relocation using `config/RMGK01/symbols.txt`. Local float/double constants are resolved to their actual retail addresses only after checking their bytes. It checks the actual retail SDA2 base initialization before applying SDA21 relocations. It then compares every byte of all six methods and the complete vtable against the verified DOL SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`.

| Retail method | Address | Size | Matching instructions | Resolved relocations |
| --- | --- | --- | --- | --- |
| Constructor | `0x800B70D8` | `0x98` | 38 | 6 |
| `calcPose` | `0x800B7170` | `0x74` | 29 | 2 |
| `calcStartPose` | `0x800B71E4` | `0x250` | 148 | 50 |
| `calcSafePose` | `0x800B7434` | `0x28C` | 163 | 49 |
| `moveToIdealPosition` | `0x800B76C0` | `0x18C` | 99 | 22 |
| Generated destructor | `0x800B784C` | `0x58` | 22 | 2 |

All **499 instructions** match byte for byte after **131 verified relocations**. The vtable at `0x80576BB8`, size `0x24`, also matches all nine words after its seven function-pointer relocations. `compiler-evidence.json` retains source/header/toolchain/object hashes and every relocation. The extra `OnlyCamera_FORCE_MATCH_SDATA2` source helper is not a retail class method and is not part of this claim.

Ignored build artifacts, including compiler command/log, original object, extracted retail disassembly and relocated function bytes, are in `build/original-only-camera-20260903/`. No native build or runtime test was run by this import task; integration and runtime validation are separate work by the root agent.

## Flag and lifecycle contracts

- The constructor initializes `mStartPose=true`, `mCalcIdeal=false`, `mSpeed=0`, and both reset/zero-move flags false. It deliberately does not initialize `_24`. Retail has no store to offset `0x24` in the constructor.
- `calcPose` consumes `mIsResetting`: clears `mCalcIdeal`, `_24` and the reset flag, sets `mStartPose=true`, then runs `calcStartPose` and clears `mStartPose`. It does **not** clear `mSpeed` or `mIsZeroFrameMoveOff`.
- Every explicit retail method was checked for `mIsZeroFrameMoveOff` access. The constructor initializes it; `calcSafePose` clears it at `0x800B7558`; no method reads it. The start path leaves an externally set value intact. `CameraDirector::calcPose` sets it when the selected manager requests zero-frame-move-off (`src/Game/Camera/CameraDirector.cpp:210`). Do not invent an effect for the flag.
- `mCalcIdeal` is read by `moveToIdealPosition` at `0x800B76E4`. Its constructor/reset/arrival stores only clear it. The available root camera source contains no assignment that enables it; the branch nevertheless remains unchanged and can be exercised through the actual public state. This does not claim to identify arbitrary external memory writes throughout the entire retail executable.
- When externally enabled, the ideal-position branch retains its original acceleration of 1, maximum speed of 100, and stopping test `distance < speed * 0.5f * (s32)speed`. It tracks the input ideal position separately from the output pose. Resetting uses the start path, which replaces `mPos` with the selected manager position while leaving speed unchanged.
- The generated destructor calls the `NameObj` destructor and conditionally deletes the `OnlyCamera` allocation; it does not delete the separately allocated `mPoseParam`. A native owner must retain ownership of that pose allocation without modifying the Game destructor.

## Pose contracts for integration and tests

The first/start calculation extends a nonzero watch distance below 300 to 300. If the displacement is near zero (default MR tolerance `0.001f`), it uses `watch = position + (0,0,-1)`, which has distance **1**, not 300. It normalizes front/up and repairs zero or nearly parallel up using the original quaternion from `(0,0,-1)` to front.

The later/safe calculation uses the previous output watch-minus-position when the requested distance is below 1; otherwise it extends distances below 300 to 300. It calls the original ideal-position branch, clears the zero-move flag, then repairs the up vector. Nearly collinear current/previous fronts reuse the previous up vector; other degenerate-up cases rotate it with the original quaternion. The dot threshold is exactly `0.98f`; it then calls the existing original `CameraLocalUtil::recalcUpVec`.

Both paths copy position, watch position, up, watch-up, global/local offset, and roll. They leave the output pose's FOV/front-offset/upper-offset unchanged. The original Director passes the current manager's FOV separately into `CameraViewInterpolator::updateCameraMtx` (`CameraDirector.cpp:316`); the PC owner should retain that distinction.

Existing providers cover the complete import: actual `NameObj` and `CameraPoseParam`, the original manager pose getters and `recalcUpVec` in `compat/CameraLocalUtilRuntime.cpp`, existing `Game/Util/MathUtil.cpp`, JGeometry vectors/quaternions, and the vector-magnitude provider. No target lookup, fake Director, stage-specific camera setting, or synthetic motion callback is needed. The exact relocated PPC result verifies the imported class algorithms; it does not claim bitwise parity for every pre-existing native math provider.

## Reproduce

```sh
python3 pc-port/notes/original-only-camera-20260903/verify-source.py
python3 pc-port/notes/original-only-camera-20260903/verify-object.py --compile
```

The source verifier checks both exact mirrors and all recorded retail range hashes when the ignored DOL is available. The compiler verifier requires the original compiler and verified DOL. Neither command invokes the native build system.
