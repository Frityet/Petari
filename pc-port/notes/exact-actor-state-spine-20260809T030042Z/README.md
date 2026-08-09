# Exact actor-state and Spine synchronization

This lane restores the retail actor-state transition substrate on PC without activating Mario or adding a compatibility fallback.

## Exact source boundary

The following PC files are byte-identical to their root RMGK02 counterparts:

- `Game/LiveActor/ActorStateBase.hpp` and `.cpp`
- `Game/LiveActor/ActorStateKeeper.hpp` and `.cpp`
- `Game/LiveActor/Spine.hpp` and `.cpp`

Canonical RMGK02 metrics are complete for all three translation units: ActorStateBase is 88/88 code bytes and 1/1 function, ActorStateKeeper is 448/448 and 6/6, and Spine is 428/428 and 6/6.

Root SHA-256 values:

- ActorStateBase.hpp: `d665b9207c039363a2b1dc7d1ab423157ee6a4662c918e56667bd2c53a3cb960`
- ActorStateBase.cpp: `84d63e563c4053d4315d907d3e8fcc5f20abab13d5cda57d02217e9176d304bc`
- ActorStateKeeper.hpp: `e2e1ba7e169e5c84b40568ed7e0c8dd7710278b059c76387d70d0d244f150cf8`
- ActorStateKeeper.cpp: `c1d1ce4ccb60961cbe41fa2d8cf87de9c5b42c70e69acceca5084adac75c5415`
- Spine.hpp: `f4bdbb7f0d143fdd7b7310f6751e00542cef99b925696f477643e2a891fd4c38`
- Spine.cpp: `d18c12b878b5821cb8d79f1c052db451679e90e3053d7fb2b78b6b75d0fcd449`

## Semantic correction

The previous PC Spine omitted the retail end-of-update nerve transition, hid a pending nerve from `getCurrentNerve()`, and constructed a no-op state keeper. The exact implementation now commits a nerve queued during execution at the end of that update, reports a queued nerve immediately, and runs the real state appear/control/kill lifecycle.

The SphereSelector real-disc regression was updated to assert that exact pending-nerve visibility. This is a generalized behavior correction, not a route-specific workaround.

## Result

- all 75 configured Game source-mirror pairs pass, including the six new pairs
- Aurora-native substrate tests pass 26/26
- SphereSelector real-or-absent tests pass 4/4 using the real RMGK01 FileSelect row
- layout tests pass 7/7, demo-scene runtime tests pass 19/19, and file-select-name tests pass 5/5
- the full `smg-pc` debug target links successfully

Mario and MarioActor factory records remain absent. This lane only makes the exact state machine available for the later atomic player closure.
