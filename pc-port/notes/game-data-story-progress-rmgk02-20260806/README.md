# RMGK02 story-progress comparison correction

Date: 2026-08-06

## Result

`GameDataHolder::isPassedStoryEvent` now implements the retail RMGK02
comparison:

```cpp
return mPlayerStatus->mStoryProgress >= progress;
```

The previous decomp source compared the operands in the opposite direction.
That made an early save appear to have passed later story events and made a
later save fail checks for events it had already passed.

## Target evidence

The retail function is at `0x803B35C8` in
`build/RMGK02/asm/Game/System/GameDataHolder.s`. Its final comparison loads:

- the required event `progress` from the local at `r1 + 0x8` into `r5`;
- the current player story-progress byte from `PlayerStatus + 0x0c` into `r0`.

It then executes:

```asm
subfc  r0, r5, r0
subfze r3, r3
```

The carry produced by `current - required` is converted to the returned
boolean, so the condition is `current >= required`.

This is directly relevant to the Gateway route: retail story-event checks gate
the Tico guide completion and the persistent spin entitlement.

## Verification

- `ninja build/RMGK02/src/Game/System/GameDataHolder.o`: pass
- object `.text`: 2912 bytes, 98.90797% similarity
- `isPassedStoryEvent`: 132 bytes, 99.393936% similarity
- object SHA-256:
  `a05007fe2d109832de0f1c00d5e07035b82942e69fd2911798134a60b2b13731`
- source SHA-256:
  `eb3e3f7fe89aaa9abd6d90690c2594b431b85837ebf63579836b373857efe1fc`

The remaining mismatch in this function is code-generation shape rather than
comparison semantics.
