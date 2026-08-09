# MarioFoo RMGK02 reconstruction evidence

Timestamp: 2026-08-09T04:18:56Z

## Scope

- Reconstructed `src/Game/Player/MarioFoo.cpp` from the RMGK02 target object.
- Corrected the dedicated ABI declaration in `include/Game/Player/MarioFoo.hpp`.
- No `Mario.hpp`, PC activation/factory/compatibility, SaveIcon, TriggerChecker, staging, or commit changes were made.
- The only owned source paths are the two paths above.

## ABI/layout proof

The retail constructor in `build/RMGK02/obj/Game/Player/MarioFoo.o` directly writes the reconstructed scalar fields at offsets `0x11` through `0xAE`, initializes matrices/vectors at `0x30`, `0x3C`, `0x64`, and `0x78`, and loops 64 times with a 12-byte stride over vectors rooted at `0xB0` and `0x3B0`. Its final three pointer/word stores are at `0x6B0`, `0x6B4`, and `0x6B8`, proving a 32-bit class extent of `0x6BC`. The retail MarioFoo vtable is 76 bytes (19 words), and the header now declares the corresponding overrides.

## Focused objdiff

Command: `build/tools/objdiff-cli diff -p . -u main/Game/Player/MarioFoo`

- `.text`: **96.66997%** (target 10,508 bytes; base 10,588 bytes)
- `.ctors`: 100%
- `.sdata`: 100%
- `.sdata2`: target bytes 100%
- Retail MarioFoo vtable: 100%
- Static initializer: 99.31373% (204/204-byte target/base size)

| Symbol | Match | Target/base bytes |
|---|---:|---:|
| `Mario::tryStartFoo` | 99.117645% | 136 / 136 |
| `MarioFoo::MarioFoo` | 99.788734% | 284 / 284 |
| `init` | 100% | 128 / 128 |
| `start` | 99.70238% | 336 / 336 |
| `update` | 96.729195% | 2,836 / 2,824 |
| `notice` | 100% | 80 / 80 |
| `close` | 95.99315% | 584 / 588 |
| `getGravityVec` | 100% | 4 / 4 |
| `jet` | 97.45283% | 848 / 848 |
| `updateTilt` | 96.29518% | 664 / 660 |
| `hitWall` | 100% | 240 / 240 |
| `getStickY` | 100% | 4 / 4 |
| `spin` | 95.90625% | 256 / 264 |
| `passRing` | 99.818184% | 220 / 220 |
| `calcRingAcc` | 99.427086% | 384 / 384 |
| `draw3D` | 94.82746% | 3,292 / 3,376 |
| `getBlurOffset` | 100% | 8 / 8 |

The remaining code differences are compiler register/stack allocation and auto-inlining differences, chiefly in `draw3D`; they do not indicate missing behavior. Retail `.data` also contains 63 bytes of unreferenced damage-name literals absent from the reconstructed source. The source-proven ten `INIT_NERVE` declarations reproduce the retail constructor order and weak vtables; the unsplit base object additionally owns their instance storage, whereas the split retail object marks those instance symbols undefined.

The complete raw diff is `focused-objdiff.json` in this ignored directory.

## Verification

- Focused unit compile: success.
- Full `build/RMGK02/main.dol` build: success (`ninja: no work to do`, already current).
- `build/tools/dtk shasum -c config/RMGK02/build.sha1`: **OK**.
- Built and retail DOL SHA-1: `54b71431af0d509097bfdef4ec28617afc487e89`.
- Built and retail DOL SHA-256: `8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf`.
- Byte comparison against `orig/RMGK02/sys/main.dol`: identical.
- Tracked header whitespace check: clean; the no-index new-source check emitted no whitespace errors (its exit 1 only records that the file is new).

## PC provider implications

No PC provider was added or activated. A later PC implementation must supply the `CelestrialSphere` area query/sphere helpers, DashRing sensor host data, Mario actor/animator/constant interfaces, left/right hand joint positions, `FooLine.bti` via `ResourceHolder`/`JUTTexture`, and the direct GX/TDDraw trail path or an equivalent renderer. Existing sound calls remain in the retail reconstruction, but audio provider work can be deferred.
