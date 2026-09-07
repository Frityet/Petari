# Original MarioActor::beginRush, 2026-09-07

Recovered the missing 436-byte method at retail RMGK01 `0x802BDFA0` into the reference Rush translation unit, then mirrored it into native. It selects the actual first rush sensor, clears the animator's joint transforms, invalidates and selectively revalidates original sensors, stops the two original effects, and follows the original fixed-jump/spin-catch versus normal binding branches. The Power Star sensor branch retires effects and the actual CollectCounter, restores the player mode/fog, and preserves the original blend-timer exception. Bee meter changes use the actual GameSceneLayoutHolder owner.

Fresh Wii compile passes; `beginRush` scores **99.770645%** and the direct call sequence matches retail. The typed CollectCounter and MarioConst includes permit the exact original virtual kill and `mRushInBlendTimer` access. Native whole-TU object compilation also passes. The preexisting endRush mirror still maps only native RushEndInfo member names; no additional PC behavior is introduced.

`proof.json` records the exact source hashes, symbol address/size, method scores, calls, and verified string-pool offsets. `beginRush.asm` is the retail instruction review. This closes a source/link dependency; runtime rush or jump success is not claimed here.
