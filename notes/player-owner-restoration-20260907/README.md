# Original Player owner restoration and native closure, 2026-09-07

Eight previously recovered Player source units had been lost from the reference during the root flatten. They are restored byte-identically from `e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4` (`1347a481f^`); the current port already had these bodies. The source and per-file hashes are recorded in provenance.json. No new Player algorithm is invented by this restoration.

The original GC/3.0a3 compiler with current Game flags compiles every restored unit. Fresh RMGK01 targets are split from the verified original DOL with current symbol/split configuration. The table records complete text fuzzy scores, not a runtime or whole-game fidelity claim:

| Unit | Text fuzzy match |
| --- | ---: |
| MarioAnimationEfx | 98.912% |
| MarioBee | 99.70468% |
| MarioSideStep | 99.691925% |
| MarioWall | 93.25643% |
| MarioDamage | 98.96457% |
| MarioSpecial | 99.26617% |
| MarioSlope | 99.3767% |
| MarioCollision | 73.60395% |

MarioCollision remains a substantially lower-match inherited reconstruction. Source restoration preserves previous work; its whole gameplay behavior still needs retail comparison before claiming accurate collision. Objdiff pairs 40 inherited code symbols in that unit; the low aggregate must not be described as a high-match closure. No instruction-byte identity is claimed for the whole unit.

Missing owner declarations and fields in supporting Wall, SideStep and damage-state headers are restored from the same source checkpoint. MarioAnimator callback fields preserve `_10F`, signed `_11C`, and `HashSortTable* _120`; its four callback declarations are restored, and the sole Luigi swap-table definition returns to MarioAnimationEfx.cpp. The existing MarioAnimator unit still compiles. Its unrelated walk-weight header names are preserved.

Mario.hpp now gives the actual recovered return types to damagePolygonCheck, doAbyssDamage, doFireDanceWithInitialDamage, doFireObjHitWithInitialDamage, both doNeedleWithInitialDamage overloads, doNeedle, doFireDance, doDarkDamage, doSideStep, and doRecovery (bool); checkOnimasu is void. `_5FC` is HitSensor*, as established by the recovered Special source's assignments and loads of the sensor's position. These declaration corrections remove previously inconsistent source/header errors and preserve the retail PPC representation. The reconstructed MarioSpecial text scores99.26617% with this real pointer field. The existing getHitWallNorm bool correction is preserved.

Native activation and remaining dependencies are recorded in the parent port's notes/mario-link-closure-20260907. Shared Git staging and pushes are coordinated by the parent task.
