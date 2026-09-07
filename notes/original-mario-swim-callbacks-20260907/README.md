# Original MarioSwim inherited callbacks — 2026-09-07

The unresolved `MarioSwim::proc`, `MarioSwim::keep` and `MarioSwim::postureCtrl` were caused by incorrect override declarations in the existing reference header. These derived methods do not exist in the retail object. The original 76-byte `MarioSwim` vtable points to the actual `MarioState` implementations in these three slots.

Removed those three declarations first from `decomp/include/Game/Player/MarioSwim.hpp`, then from the native `src/Game/Player/MarioSwim.hpp`. The headers remain byte-identical; neither Swim source TU changed. No forwarding methods or substitute callback behavior were introduced.

| Callback | Retail target | Retail vtable offset | Native vtable offset |
| --- | --- | ---: | ---: |
| `proc(u32)` | `MarioState::proc` | 0x18 | 0x30 |
| `keep()` | `MarioState::keep` | 0x2C | 0x58 |
| `postureCtrl(MtxPtr)` | `MarioState::postureCtrl` | 0x30 | 0x60 |

Whole-TU compilation passes with the original Wii compiler and Homebrew LLVM 23 for native ARM64. The complete corrected Wii vtable matches retail at **100%**, with the exact **76-byte** size. All **17 method slots** in the native vtable have the corresponding retail method and owning class; native pointer width and RTTI metadata follow the native ABI. The two `addVelocity` overloads preserve their separate signatures and order in the recorded relocation targets. A fresh baseline using the old header through an isolated include overlay shows **61 existing function match scores unchanged**.

`vtable-proof.json` records every slot and unchanged-source checks. `MarioSwim-vtable.retail.s` comes from `decomp/build/original-player-state-recovery-20260907/retail/asm/Game/Player/MarioSwim.s`, using the object in the matching `obj/Game/Player/` directory. `reference-compile-command.json`, `native-compile-command.json`, both compile logs, the baseline commands and `reference-objdiff-command.json` reproduce validation. `native-relocations.txt` is LLVM's full symbol/relocation evidence. The original DOL SHA-1 is `25c5959534b3c21246c6c7e42021b916b41fb578`.

The existing shared `MarioState` provider supplies message dispatch, the original inherited keep result, and posture delegation to the real Mario posture method. This tranche makes Swim use that real inheritance. It does not establish complete Swim gameplay or run a root application/GPU test; parent integration owns the complete executable build.
