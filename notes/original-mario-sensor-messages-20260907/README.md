# Original Mario sensor-message source closure

Recovered the missing original `MarioActorDefensiveMsg.cpp` bodies in `decomp/`, following `AGENT_DECOMP_GUIDE.md`, then copied the complete TU unchanged to native Game. No message or state branches are skipped. Native `MarioStateCompat.cpp` retires only its duplicated `hitWall` and `passRing` definitions; the complete original TU now owns those retail defaults.

Fresh RMGK01 Wii compiler and objdiff proof:

| Newly recovered function | Retail bytes | Fuzzy match |
| --- | ---: | ---: |
| receiveMsgEnemyAttack | 944 | 99.86017% |
| receiveMsgPush | 908 | 98.977974% |
| receiveOtherMsg | 2104 | 99.49239% |
| cylinderHitCheck | 960 | 99.34583% |

The full reference TU compiles successfully and retains the existing `tryAttackMsg`, `tryVectorAttackMsg`, `receiveMsgTaken`, and state defaults. `defensive-side-effect-calls.json` records original/recovered direct and virtual call sequences for inspection. All four new functions preserve their ordered external calls. Differences are compiler register/temporary choices and equivalent direct member access; the saved full objdiff is the detailed evidence.

`cylinderHitCheck` was incorrectly declared void in both headers. Retail callers branch on its returned r3, and the body returns true only when its sphere/cylinder overlap tests succeed. Both declarations now return bool. Its optional response vector preserves the original least-penetration branches and is only written on a hit. `receiveOtherMsg` retains the original jump, reflected jump, carry/rush cancellation, black-hole, fountain, warp, and animation behavior, including cases that perform work and still return false.

The reference-only `cylinderPushCheck` recovery also compiles at 98.813255% for its 664-byte retail body in `MarioActorOffensiveMsg.cpp`. Its original two-axis checks, asymmetric boundary comparisons and dummy-sensor position/radius writes are retained. That was the first defensive-message checkpoint; the subsequent original offensive/take closure below now completes those missing bodies.

`root-paths.json` and `decomp-paths.json` identify the first defensive-message checkpoint. That snapshot excluded OffensiveMsg while pickup recovery was in progress. The separate offensive/take/morph manifests below identify the final subsequent source cohort. The header is shared with the other Player source restoration and must be committed coherently by the parent. `native-syntax-results.json` records successful LLVM23 full-TU checks of native DefensiveMsg, duplicate retirement and the final LiveActor boundary. Full native link/runtime tests remain the parent's coordinated validation; this reference/source checkpoint does not establish playable jumping or complete message-owner closure.


## Subsequent offensive, pickup and morph closure

All missing bodies in the complete `MarioActorOffensiveMsg.cpp` and `MarioActorTakeMsg.cpp` TUs are now recovered in decomp and copied byte-for-byte into native Game. The existing `MarioActorMorph.cpp` gains its missing `changeMorphString` lookup. The new source retains original behavior, including the retail beam-shell name branch, message acceptance gates, stomp/rush ordering, morph pickup ownership and signed pickup jump timers. No host actor branch or new gameplay tuning is introduced.

| Newly recovered function | Retail bytes | Fuzzy match |
| --- | ---: | ---: |
| attackOrPushSensor | 1760 | 99.65000% |
| checkAndTryTrampleAttack | 364 | 98.07692% |
| tryGetItem | 1140 | 98.71930% |
| cylinderPushCheck | 664 | 99.17470% |
| tryPullTrans | 848 | 99.12736% |
| tryTornadoPull | 760 | 99.73684% |
| changeMorphString | 172 | 99.88372% |

All seven functions preserve the retail ordered direct and virtual call sequences (177 calls), recorded by `offensive-take-morph-side-effects.json`. Each full TU passes the Wii compiler; the final compile-command manifests include source hashes and the full objdiff reports remain alongside them. All three full native TUs pass isolated LLVM23 compilation, recorded in `offensive-take-morph-native-syntax.json`.

The homing-target field `_4A8` is now a truthful `HitSensor*` in both headers, based on its exact target assignment in `attackOrPushSensor`. The existing `tryCoinPullOne` declaration receives `NO_INLINE`, preserving the retail out-of-line calls from `tryTornadoPull`; this avoids the compiler inlining an existing function into the newly recovered body, without changing its algorithm. Existing native architecture declarations are preserved. The native source mirrors have no algorithm differences from the recovered reference.

The manifests `offensive-take-morph-root-paths.json` and `offensive-take-morph-decomp-paths.json` identify the new coherent source cohort. Native Game already selects these three TUs, so no build-list addition is required. Their integration may expose new original providers: `MR::getFootPoint` exists in the reference MathUtil TU but that native TU is excluded, so its exact reusable provider must be supplied. Full link and runtime validation remain the parent’s coordinated lane. This proof does not establish playable jumping or complete sensor broadphase accuracy.
