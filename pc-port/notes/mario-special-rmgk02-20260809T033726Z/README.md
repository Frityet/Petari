# MarioSpecial RMGK02 reconstruction

Date: 2026-08-09 UTC

## Scope

Reconstructed `src/Game/Player/MarioSpecial.cpp` from the RMGK02 target
object and disassembly.  The recovered gameplay methods are:

- `Mario::checkOnimasu`
- `Mario::isDossun`
- `Mario::isStageCameraRotate2D`
- `Mario::isNoWalkFallOnDossun`
- `Mario::isNotReflectGlassGround`
- `Mario::isUseAnotherMovingPolygon`
- `Mario::isUseFooSpecialGravity`
- `Mario::updateOnimasu`
- `Mario::isHeadPushEnableArea`
- `Mario::isOnimasuBinderPressSkip`

`updateOnimasu` restores the complete moving-collision behavior: local-space
tracking against the Onimasu collision matrix, entry/exit clamping, velocity
and translation correction, dominant-axis press detection, cube-edge
snapping, and wall-contact reaction.

## Assembly-proven declarations

Three narrow `Mario.hpp` corrections were required and coordinated with the
other Player work:

- `checkOnimasu` returns `void`; its retail body constructs no return value,
  and the sole retail caller ignores `r3` immediately after the call.
- `isUseFoolSpecialGravity` was corrected to the retail symbol spelling
  `isUseFooSpecialGravity`.
- `_5FC` is a `HitSensor*`, as shown by the direct `HitSensor` position and
  host dereferences in both Onimasu methods.

The concurrent `doSkate` and `doRecovery` declaration changes are not part of
this tranche and were preserved unchanged.

## Focused comparison

The source was compiled with the repository's GC/3.0a3 Metrowerks command and
compared against `build/RMGK02/obj/Game/Player/MarioSpecial.o`.

- `.text`: **99.72414%** over 3,712 retail bytes
- `.ctors`: **100%** over 4 bytes
- `.sdata2`: **100%** over 88 bytes
- `isDossun`: **100%**, including the retail zero-test branch layout
- `updateOnimasu`: **99.947914%**, with the exact retail size of 2,304 bytes
- all ten gameplay methods are implemented

The integration pass also restored this translation unit's retail 108-byte
MarioActor nerve singleton initializer. It is 100% matching and emits the
target `.ctors` entry, consistent with the other recovered Player units.

The final metrics above were regenerated after restoring the target
initializer; the exact command and object hash are recorded in
`verification.log`.

## Verification

The focused object builds successfully.  The only output is the two existing
non-trivial-union warnings from `MarioActor.hpp`.

The complete RMGK02 retail build remains checksum-identical:

```text
build/RMGK02/main.dol: OK
SHA-256 build/RMGK02/main.dol:
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf
SHA-256 orig/RMGK02/sys/main.dol:
8b7f28d193170f998f92e02ea638107822fb72073691d0893eb18857be0c6fcf
```

`git diff --check` passes for the owned source and shared-header hunks.
