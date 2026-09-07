# Original Mario actor and jump activation, 2026-09-07

The native actor previously replaced `init2`, `movement`/`control`, `mainMove`, and walking with PC-only code. That `mainMove` branch did not run the original `isRequestJump` / `tryJump` decisions. This checkpoint restores the complete original Actor, Move, Walk, and Pad translation units. The original `initActionMatrix()` call in `initMember` is restored too. The Triangle assignment now has its original Actor TU owner; the duplicate compatibility definition is removed.

The original actor's jump-button selector is 3. Original `isRequestJump()` checks its existing input-disable conditions, then selector 3 tests the A trigger on player WPad 0. Original `mainMove` and Mario's already restored frame/state dispatch now retain the real jump decision and MarioJump state behavior. There is no new trajectory or movement tuning.

Additional original owner recovery closes the newly reachable rush and held-object graph. Eleven missing reference MarioMove methods and `tryStandardRush`/`updateSpecialModeAnimation` were restored from verified preflatten source `e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4`, preserving the newer upstream `mainMove`. Four Parts methods and three Rush/RushMsg methods were recovered directly from the retail object instructions and relocated constants. Whole original BlackHole and chip counter/holder sources are imported for actual referenced owner methods. BlackHole construction remains unregistered: its incomplete reference constructor is not replaced with a synthetic instance.

`checkpoint-proof.json` records a fresh compile of **15 original Wii TUs and 15 native TUs, all exit 0**, taken after the final source saves. The Wii compiler is GC 3.0a3 through wibo/sjiswrap; native is the configured Homebrew LLVM 23. Retail split objects derive from the locally verified RMGK01 DOL, SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`. `verify-checkpoint.py` reproduces the complete compile/objdiff run. Raw generated objects/full diffs remain in the ignored notes output directory.

| Newly restored or newly active entry | Fresh Wii object score |
| --- | ---: |
| MarioActor constructor / init2 | 99.97758% / 99.66824% |
| initAfterPlacement / movement / control | 99.9315% / 99.88076% / 99.52702% |
| control2 / controlMain | 100% / 100% |
| MarioActor::initMember | 99.88868% |
| Mario::mainMove / tryJump / procJump | 99.29468% / 99.5881% / 96.40148% |
| updateTakingPosition / shootFireBall | 91.18239% / 97.04402% |
| updateFairyStar / updateThrowVector | 91% / 90.912% |
| tryStandardRush / isFixJumpRushSensor | 100% / 100% |
| getNearestJumpTarget / tryStartRush | 99.375% / 95.94827% |
| endRush | 98.669815% |

Instruction review is separate from the fuzzy score. The new rush methods retain retail direct-call order and relocated constants, including the jump-target search ceiling **100000.0**, rebind interval 120, launch flags, source-sensor identity checks, original gravity alignment, animation changes, and inherited velocity handoff. The first draft's search constant was corrected from 10000.0 before this checkpoint. `isFixJumpRushSensor`'s preexisting missing C++ `return` was corrected; the full 80-byte method now matches retail exactly. `rush-relocation-proof.json` records actual call/constant comparisons. The four Parts methods were reviewed against the instruction dumps in `actor-owners/`; no actor-specific absent-owner fallbacks were added.

Native changes beyond mirroring are compiler/representation fixes: the Actor's existing raster-buffer array names replace `_1D8`/`_1DC`; RushEndInfo's existing native semantic member names replace reference offset names in `endRush`; member-function pointers in MarioMove are qualified; original `_A38` now uses the existing typed HitSensor pointer in MarioJump; original reference include capitalization is made consistent. `MarioActor.hpp` also contains coordinated typed sensor declarations from the sensor lane. Its shared hash is recorded, but that lane owns those separate recoveries.

This is **source recovery and compile validation, not a playable jump/camera result**. The last coordinated showcase archive compiled, while the final executable still had 43 explicit unresolved providers before the final Rush activation. The existing reference `MarioActor::calcAndSetBaseMtx` scores 58.933773% and `Mario::doAirWalk` 87.73969%; they are not claimed as instruction-validated equivalence by this checkpoint. The complete original TUs preserve those existing bodies so they can be audited independently.

A concrete architecture issue also remains: native Clang bitfields place `MovementStates::jumping` at `0x00000001`, while retail/raw Game masks use `0x80000000`. `DrawStates::_5` similarly maps to native `0x20` versus retail `0x04000000`. The notes-only native executable in `checkpoint/bitfield-layout-probe.cpp` builds/runs 0 and demonstrates this using the actual production header. Mixed named/raw uses include `MarioMove.cpp`'s `_10_LOW_WORD & 0x1000`, `mMovementStates_LOW_WORD &= ~0x00200000`, and `mDrawStates_WORD |= 0x04000000`; Mario.cpp and MarioJump.cpp also use raw DrawStates masks. This requires a shared architecture boundary, not individual Mario behavior patches. No flag representation change is included here.
