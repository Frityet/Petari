# Rush-end damage bitfield source review

2026-09-25. Reviewed only the current RushEndInfo.hpp / PlayerUtil.cpp change, actual consumer, donor declarations and Aurora macro. No production edits, compilation, tests or live input. **No actionable issue found.**

`decomp/include/Game/Player/RushEndInfo.hpp` declares a complete PowerPC u32 group: `_0:4`, `mDamageType:4`, `_8:24`. The new native `AURORA_PPC_BITFIELD_GROUP(u32, (_0,4), (mDamageType,4), (_8,24))` preserves its numeric layout. In `aurora/include/aurora/ppc_bitfield.hpp`, big-endian/Metrowerks keeps field order; supported little-endian hosts reverse it to `_8:24`, `mDamageType:4`, `_0:4`. Thus mDamageType occupies numeric bits24–27 in either supported ABI, mask `0x0F000000`. The width typedef verifies a complete32-bit group and adds no storage. This is source/ABI reasoning, not a newly compiled layout test.

The union still holds one32-bit word, matching the replaced raw `_20`. Native pointer widening moves later physical offsets compared with original Wii offset comments, but that was already true before this change: the union does not introduce an additional pointer or widened value. Observed call sites pass a typed RushEndInfo pointer; the consumer accesses `_20` as a member, not at a hardcoded Wii byte offset.

`RushEndInfo.cpp:3` value-initializes `_20` to zero. All six restored setters first OR `0xC0000000`, then assign the four-bit field, so the independent high flags remain intact:

| PlayerUtil setter | Donor/native damage value | Resulting numeric word |
| --- | ---: | --- |
| endBindAndPlayerDamage (`:540`) | 1 | `0xC1000000` |
| endBindAndPlayerFlip (`:548`) | 6 | `0xC6000000` |
| endBindAndPlayerAcidDamage (`:603`) | 4 | `0xC4000000` |
| endBindAndPlayerFreezeDamage (`:611`) | 3 | `0xC3000000` |
| endBindAndPlayerFireDamage (`:619`) | 2 | `0xC2000000` |
| endBindAndPlayerElectricDamage (`:627`) | 5 | `0xC5000000` |

Values and assignment order match `decomp/src/Game/Util/PlayerUtil.cpp` exactly. `MarioActorRush.cpp:247` switches on `(_20 >> 24) & 0xF`, dispatching damage, acid/fire, freeze, electric and flip as expected; its high-flag reads at236/244 and independent bit22 read at289 remain nonoverlapping. The old native setters only cleared the damage mask, leaving zero and suppressing those cases. Restoring the donor assignments fixes that concrete behavior without adding a new policy.

Ordinary jump/SpinDriver helpers retain their existing zero damage nibble and are unaffected by these six assignments. This source review does not claim that an actual damage or launch sequence was exercised.
