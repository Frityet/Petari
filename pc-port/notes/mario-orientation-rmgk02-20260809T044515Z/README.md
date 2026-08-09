# Mario physical/orientation recovery (RMGK02)

Baseline: `d672a0f443a500bdbdff2c43f2090ba811cb7c7b`

Recovered from `build/RMGK02/asm/Game/Player/Mario.s`:

- `Mario::createDirectionMtx` (`0x802AAFDC`, `0x184`): 99.793816% fuzzy
- `Mario::createCorrectionMtx` (`0x802AB160`, `0x29C`): 93.862274% fuzzy
- `Mario::writeBackPhyisicalVector` (`0x802ACC08`, `0x790`): 96.66942% fuzzy
- `Mario::postureCtrl` (`0x802ADD48`, `0x404`): 96.04669% fuzzy
- `Mario::createAngleMtx` (`0x802AE14C`, `0x340`): 90.70192% fuzzy

The recovered bodies include the collision projection and final actor vector writeback, Bee/Fur wall-walk probe, swimming knockback projection, posture blending, direction/correction matrices, and sensor-driven angle matrices. Assembly also proves that `createCorrectionMtx` returns `bool` (`false` only on the non-fix-head early path) and that the final head-gravity guard in `createAngleMtx` reads movement-state bit 22 (`debugMode`).

Verification:

- `ninja -C . build/RMGK02/src/Game/Player/Mario.o`: pass
- `ninja -C . build/RMGK02/src/Game/Player/MarioActor.o`: pass (validates the shared declaration consumer)
- `ninja -C . build/RMGK02/main.dol`: pass / no work required
- `sha1sum -c config/RMGK02/build.sha1`: pass
- DOL SHA-1: `54b71431af0d509097bfdef4ec28617afc487e89`
- `git diff --check -- include/Game/Player/Mario.hpp src/Game/Player/Mario.cpp`: pass

Owned production paths:

- `include/Game/Player/Mario.hpp` (only `createCorrectionMtx` return type)
- `src/Game/Player/Mario.cpp`

Provider implication: these five central providers now have active retail-faithful C++ bodies, so the recovered `Mario::update` loop can propagate physical vectors to `MarioActor` and build orientation matrices instead of depending on absent/commented implementations. No PC-port activation, factory, compatibility, configure, protected-file, staging, or commit action was performed.
