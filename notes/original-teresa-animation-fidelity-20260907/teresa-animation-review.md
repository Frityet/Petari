# Independent Teresa animation retail review

Reviewed only `MarioActor::initTeresaMarioAnimation`, `runTeresaBaseAnimation`, `changeTeresaAnimation`, and `updateTeresaAnimation`. No production or decomp source was changed by this review, and no root build or GPU process was launched. The proposed method replacements are in `teresa-animation-candidate.cpp`; they require fresh Metrowerks comparison before adoption. Existing disassembly scores below describe the supplied baseline, not a newly compiled candidate.

No omitted animation-selection branch or wrong gameplay constant was found. There are real differences in call sequencing and stored-state reads in `updateTeresaAnimation`; the proposed body preserves the retail calls even though the current concrete getter implementations are pure. These are fidelity corrections, not independently demonstrated ordinary-gameplay failures.

| Method | Retail offset / bytes | Supplied baseline score | Review |
| --- | --- | --- | --- |
| `initTeresaMarioAnimation` | `.text+0x1370`, 240 | 73.33% | Both allocations, resource/model arguments, default/basic animation calls, and ModelManager installation are present. Retail reloads the stored table/player between calls; candidate uses existing typed access helpers at each use. |
| `runTeresaBaseAnimation` | `.text+0x1cf8`, 136 | 52.38% | Wait-state and already-basic guards, animation switch, random blink timer 60–180, and BTP start all match. Candidate removes the player-pointer load before the state guard and reloads it after the animation query. |
| `changeTeresaAnimation` | `.text+0x1d80`, 264 | 76.59% | Sleep-effect deletion, `-1` interpolation distinction, wait/run random timer and BTP stop, and existing-name/fallback-blink BTP branch all match. Candidate spells the last two call branches separately. |
| `updateTeresaAnimation` | `.text+0x1e88`, 1112 | 58.49% | All wake, one-shot, hit/spin gating, fly/fall/base, track weighting, fade/countdown/sound, and blink-timer branches are present. Candidate restores query order, branch-local/repeated virtual calls, intermediate weight storage, and owner-field reloads. |

## Concrete sequencing differences

1. Retail `.text+0x1ec0..0x1f10` evaluates acceleration, stick magnitude, and jump-vector magnitude independently while asleep; the current `||` expression short-circuits later probes. Candidate uses three separate `if` statements before the wake action. `MarioTeresa::isTeresaAccel` reads `_44`; `MarioModule::getStickP` reads `mStickPos.z`; the current vector-length helper has no Game callback. No changed wake decision is claimed for these concrete implementations.
2. Retail `.text+0x1fe0..0x2004` queries fly/base eligibility before fetching gravity or invoking virtual `getLastMove`. The current body hoists one dot-product evaluation above the animation checks, even for unrelated animations. Candidate calls the getters only inside the fly/base and fall branches.
3. Retail `.text+0x2054..0x2098` first tests `mDrawStates._1C`, then fetches gravity and invokes virtual `getLastMove` a second time before the absolute-speed check. The current body reuses the first dot result. Candidate preserves the second call and explicitly sequences `getGravityVec` before the virtual getter, including on modern C++ compilers. The fall branch separately computes its own dot only after confirming the fall animation.
4. Retail `.text+0x2128..0x2158` stores the unbounded speed ratio to `_9B0` before `MR::clamp`, stores the result, and reloads the actor's player pointer between track updates. The current source stores only the clamped value and caches the player across the whole method. Candidate restores both stores and uses the existing actor-field access helper at each operation.
5. Retail reloads the Teresa pointer and selected MarioConst table in the fade/countdown branches rather than caching them before intervening callbacks. Candidate likewise accesses the owner at the actual use. This preserves the original access points without adding a new policy or state.

`MarioActor::getLastMove()` is virtual and currently returns `mLastMove` (`MarioActorCamera.cpp:100`); `getGravityVec` currently returns the Mario gravity vector (`MarioActorGravity.cpp:11`). Therefore the virtual-call difference is meaningful to the recovered API contract but is not evidence that current Mario movement already exhibits an incorrect animation transition.

## Verified identities and data

The retail DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`, the same hash recorded by the supplied split. `teresa-animation-retail-data.json` records strings read directly through the DOL section table and the original 152-byte group table / 40-byte two-track table bytes.

- `MarioStatus_Wait` is `0x1c`, matching the immediate in `runTeresaBaseAnimation`.
- `mDrawStates._1C` is the 29th declared bit in the Wii word at offset `0x18`; retail `extrwi ...,1,28` selects that bit (numeric bit 3 on Wii). It is not an incorrect neighboring flag.
- `_9B4` and `_418` are `u16`; retail countdown truncation and zero checks match. Do not change the fade decrement to clamp at zero: retail can cross zero in either decrement, and it tests the updated value before applying a second decrement when `_46` is nonzero.
- MarioConst offsets `0x6bc`, `0x6c0`, `0x6c4`, and `0x6c8` are alpha max, increment, decrement, and `u16` wall-through time respectively. The spin trigger is strictly `remaining > duration - 3`; the movement threshold is exactly `1.0f`, and track ratio divides by `10.0f`.
- DOL text is basic `基本`, `blink`, `sleep`, effect `Sleep`, `wait`, `spin`, `fall`, `hit`, `run`, `fly`, and sound `テレサ現れる`. The two basic strings and two run strings occur at different original addresses but have identical contents. Current data names and track defaults match: base group rate `1`, `_8 = 0x10`, wait weight `1`, run weight `0`, and empty-name sentinels. The gap between the two tables contains the separate `wait` string and alignment; it is not a missing group member.

This review does not cover the movement methods being independently corrected by the Player owner, Xanime runtime ownership, or complete Teresa gameplay. Candidate initialization retains the reference `u32` pointer storage cast; the native mirror must retain its existing architecture-width conversion when the owner applies the method spans.
