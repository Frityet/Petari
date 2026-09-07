# Original Mario state reference recovery, 2026-09-07

Restored seven missing reference translation units and five dedicated typed
headers from `e1985ac3a9736a9c432c1d7af6ce8bf96e3abed4`, following
`decomp/AGENT_DECOMP_GUIDE.md`. Faint and Paralyze headers already matched that
source. The reference `MarioActor.hpp` additionally regains the original
`updateTeresaAnimation()` declaration. No native file, activation list, shared
Mario.cpp body, or Git index was changed by this recovery.

Every complete restored translation unit compiles with the configured Wii
compiler. Retail objects were freshly split from the locally verified RMGK01 DOL
(SHA-1 `25c5959534b3c21246c6c7e42021b916b41fb578`) without modifying the production
decomp configuration. The following are fresh object comparison scores:

| Unit | Whole text | Constructor | Vtable |
| --- | ---: | ---: | ---: |
| MarioFaint | 98.97% | 100% | 100% |
| MarioDamageParalyze | 99.88% | 100% | 100% |
| MarioRecovery | 99.01% | 100% | 100% |
| MarioTeresa | 76.54% | 99.83% | 100% |
| MarioFoo | 96.67% | 99.79% | 100% |
| MarioHang | 85.85% | 100% | 100% |
| MarioWait | 98.38% | 99.85% | 100% |

Every constructor also retains the retail direct-call sequence. Separate compiled
PPC assertions establish state sizes `0x28`, `0x1C`, `0x8C`, `0x5C`, `0x6BC`,
`0x44`, and `0x18`, respectively, plus selected timer, vector and final-member
offsets. This validates the typed layouts without constructing invented native
objects.

Two source corrections accompany recovery:

- Foo's three DashRing accesses use the current member names `mMaxDuration`,
  `mSpeedScale`, and `mBoostTime`, preserving offsets `0xAC`, `0xB0`, and `0xA8`.
  Its full object score remains the documented 96.67%.
- The historical Teresa constructor initialized extra fields and padding before
  its vector reset. Direct retail inspection showed a 116-byte constructor that
  first zeroes `_14`, initializes `_40`, `_20`, `_24`, `_4C`, `_54`, `_58`, and
  `_59`, then invokes `resetTeresaMode`. The recovered constructor now preserves
  that sequence; its score improves from 25.21% to 99.83% at the same retail size.
  Its body assigns scalars after the vector operation to preserve that observed
  order, consistent with neighboring original state constructors.

**The lower-score Hang and Teresa bodies are recovered candidates, not verified
behavioral equivalence.** Hang's `update` remains at 69.73%; Teresa's aggregate
contains other lower-score methods despite its corrected constructor and 96.26%
state update. Those methods require a separate instruction-level review before
native readiness can be claimed. No gameplay or camera runtime claim follows
from this reference checkpoint.

`function-proof.json` contains every covered function score, constructor call
sequence and retail byte hash. `compile-results.json`, `objdiff-results.json`,
`retail-split.json`, and `layout-proof.json` preserve commands/results. Initial
compile failures from the two stale declarations/names are retained separately.
`source-manifest.json` records provenance/hashes, and
`decomp-checkpoint-paths.json` is the explicit 13-path parent commit list.

Prior recovery notes used to locate source and compare scope:
`mario-constructor-states-decomp-20260809T021050Z`,
`mario-damage-crush-paralyze-rmgk02-20260809T024404Z`,
`mario-recovery-rmgk02-20260809T034644Z`,
`mario-teresa-rmgk02-20260809T050110Z`,
`mario-foo-rmgk02-20260809T041856Z`,
`mario-hang-rmgk02-20260809T042238Z`, and
`mario-wait-rmgk02-20260809T022906Z`. Their historical full-DOL build claims are
not reused as native runtime evidence here.
