# Mario walking-core recovery (RMGK02)

This tranche reconstructs the six central `Mario.cpp` routines needed by the normal walking/update path:

- `Mario::inputStick()`
- `Mario::update()`
- `Mario::actionMain()`
- `Mario::updateGroundInfo()`
- `Mario::doExtraServices()`
- `Mario::checkForceGrounding()`

The implementations were recovered against `build/RMGK02/asm/Game/Player/Mario.s` and compared with `objdiff-cli`. No inline assembly is used. The older `actionMain` and `updateGroundInfo` pseudocode trapped inside the unfinished `writeBackPhyisicalVector` comment was removed so every recovered routine has one discoverable definition.

## Assembly-proven corrections

- `checkForceGrounding` is a `void` operation: retail branches every guard directly to its epilogue and `update` ignores its result.
- The retail symbol is intentionally misspelled `writeBackPhyisicalVector__5MarioFv`; the shared declaration and all three `update` calls now preserve that ABI.
- `updateGroundInfo` assigns the return value of `checkGround()` to `mMovementStates._1`; the prior pseudocode discarded it and forcibly cleared the bit.
- The landing animation repair sets `_B`, not `_14`.
- The long-fall safety service tests `MarioActor::_3C0`, not Mario's unrelated `_3C0` timer, and its second mode exclusion is Foo rather than Bee.
- Forced grounding tests the second-word `_37` movement flag, not `_17`.
- The grounded damage gate is `grounded && (!jumping || _B)`.

## Focused result

| Function | Target bytes | Source bytes | Fuzzy match |
|---|---:|---:|---:|
| `doExtraServices` | 524 | 508 | 92.061066% |
| `checkForceGrounding` | 712 | 716 | 97.174160% |
| `inputStick` | 792 | 792 | 97.070710% |
| `update` | 1,412 | 1,404 | 96.566574% |
| `actionMain` | 480 | 480 | 100.000000% |
| `updateGroundInfo` | 588 | 588 | 99.965990% |

Across the six functions, the target contains 4,508 bytes, the arithmetic mean is 97.13975%, and the target-byte-weighted match is 97.03638%. Five of six functions exceed 95%; `doExtraServices` is complete and its remaining delta is boolean materialization/register allocation.

Source object SHA-256: `da1921dbff14c6805feb3ee6a2358c28c5a635dbd752bf0a02e3e99b53d209a9`.

The raw focused JSON has SHA-256
`c663325a54c807d951e8f8be9b1fcbf7e795e5ba1ef5392c6c946b69f0d6429c`;
it remains a local ignored work product because its summarized form above is
the useful repository evidence.

## Activation note

`configure.py` was intentionally not changed. The byte-identical full DOL therefore verifies that this source/header work did not disturb the currently activated RMGK02 link; it does not claim that `Mario.cpp` was switched from its target object to the new source object.
