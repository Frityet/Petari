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

The reference-only `cylinderPushCheck` recovery also compiles at 98.813255% for its 664-byte retail body in `MarioActorOffensiveMsg.cpp`. Its original two-axis checks, asymmetric boundary comparisons and dummy-sensor position/radius writes are retained. Other missing offensive/take functions are still the next frontier; they are not replaced by simplified handlers.

`root-paths.json` identifies the native cohort; `decomp-paths.json` intentionally excludes OffensiveMsg: cylinder-push proof is retained as recovery evidence, while that reference TU contains a newer incomplete pickup candidate outside this checkpoint. The header is shared with the other Player source restoration and must be committed coherently by the parent. `native-syntax-results.json` records successful LLVM23 full-TU checks of native DefensiveMsg, duplicate retirement and the final LiveActor boundary. Full native link/runtime tests remain the parent's coordinated validation; this reference/source checkpoint does not establish playable jumping or complete message-owner closure.
