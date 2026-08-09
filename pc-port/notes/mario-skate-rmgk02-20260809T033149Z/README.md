# MarioSkate RMGK02 reconstruction

## Scope

Reconstructed the previously absent `Game/Player/MarioSkate.cpp` from the
RMGK02 retail assembly and object metadata. The implementation covers all ten
retail functions:

- `Mario::isSkatableFloor`
- `Mario::doSkate`
- `MarioSkate` constructor
- `MarioSkate::postureCtrl`
- `MarioSkate::exitJump`
- `MarioSkate::start`
- `MarioSkate::update`
- `MarioSkate::close`
- the translation-unit static nerve initializer
- `MarioSkate::notice`

No PC-port source, compatibility fallback, build configuration, or protected
file was changed.

## ABI and layout evidence

The retail object proves a `0x28`-byte `MarioSkate` layout after the `0x14`-byte
`MarioState` base:

- `0x14`: 32-bit step timer
- `0x18..0x1D`: jump/drive/animation/foot-step state bytes
- `0x20`: skate yaw cycle
- `0x24`: posture lean

It also proves that `Mario::doSkate()` returns `true` after calling
`changeStatus`; the shared declaration was narrowly corrected from `void` to
`bool`. Both existing call sites discard the return value, and the mangled name
is unchanged.

## Recovered behavior

The implementation restores ice-floor eligibility, skate state entry and exit,
jump handoff, landing and twist animations, spin acceleration, alternating
left/right skate steps, effect and sound events, speed maintenance, yaw/lean
posture control, and preservation of the gravity component when handing a jump
back to normal Mario movement.

## Result

The focused object has the exact retail `.text` size (3,184 bytes), exact
`.ctors` and `.sdata2`, and all ten retail functions are present at their exact
retail byte sizes. The raw focused objdiff score is 99.19598%. The normalized
project report score is 99.72990%; seven functions are exact and the remaining
three are 99.54% or better.

The full RMGK02 DOL remains byte-identical to the retail input.
