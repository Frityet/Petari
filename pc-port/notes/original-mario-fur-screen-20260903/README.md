# Original Mario fur visibility and screen allocation recovery

Root-first recovery after checkpoint `0f94cf8df`. Eight functions, 924 retail instruction bytes, all instruction bytes identical after masking only verified relocation fields. Every relocation offset/kind/addend and resolved constant/string content or external call target is equal. Original GC 3.0a3 configured Game compiler and the verified RMGK01 DOL were used. `verify-original.py` reproduces this proof; no ROM bytes are included here.

| Function | Retail / compiled bytes | Raw objdiff |
|---|---:|---:|
| MarioActor::initScreenBox | 92 / 92 | 99.78% |
| MarioActor::hideBeeFur | 160 / 160 | 99.75% |
| MarioActor::showBeeFur | 180 / 180 | 99.56% |
| MarioActor::drawSphereMask | 188 / 188 | 99.26% |
| FurMulti constructor | 164 / 164 | 100% |
| FurMulti::offDraw | 68 / 68 | 100% |
| FurMulti::onDraw | 64 / 64 | 100% |
| MR::initFurPlayer | 8 / 8 | 100% |

`initScreenBox` allocates **0x20000 bytes at 32-byte alignment** (256×256 RGB565). The removed comment incorrectly suggested 0x80000; retail `lis r3,2` and the complete 92-byte match establish the actual allocation. This remains the original Game allocation. Native scene allocation scope and retirement must surround it externally; this checkpoint does not claim a new ownership scope for `_B44`.

Bee visibility calls the real FurMulti layer controls and the actual MarioParts virtual appear/kill methods, carried actor model visibility, and invincibility effect/joint methods. `_9EC`, `initFurPlayer`, and `initMultiFur` now have the actual FurMulti pointer type. The class is 0x24 bytes on Wii, has no vtable, and keeps its actor/model/control arrays as typed pointers; the retail allocation in initMultiFur is 36 bytes. The recovered constructor and off/on functions prove all fields they access. Unknown fields 0xC–0x14 remain u32 placeholders. No fabricated LiveActor cast or visibility no-op was introduced.

The sphere mask preserves the original conditional, TDDraw setup, depth/cull/blend state, Spine1 position, 180-unit sphere, color, subdivision count, and Z scale restoration.

`stage-native.py` creates the minimal eight-file native delta under `build/original-mario-fur-screen-20260903/staged`. It copies root-identical SpecialDraw and FurMulti code, root-identical FurMulti/FurCtrl declarations, changes only the FurMulti pointer/return types in native MarioActor and LiveActorUtil, and adds the missing GXTransform include wrapper for Aurora's actual API. The isolated native SpecialDraw, FurMulti, and existing MarioActorDraw consumer all compile with current native Game flags. No native production files, shared builds, source selection, or GPU runtime were changed by this work.

Remaining activation requirements: source selection of FurMulti.cpp; complete original MR::initMultiFur/setLayerDirect and downstream FurCtrl/FurDrawer construction if fur creation is live; actual MR::initFurPlayer provider selection (the root wrapper is restored but full native LiveActorUtil.cpp is not selected by this patch); general ownership for its arrays and `_B44`. `drawPreWipe` already exists in the root MarioActorWipe.cpp and needs source selection, not a replacement body. Remaining larger SpecialDraw methods are separate recoveries. This evidence proves recovered source and bounded compile behavior, not live Mario gameplay.

Files for the root checkpoint: src/Game/Player/MarioActorSpecialDraw.cpp; src/Game/Util/FurMulti.cpp; src/Game/Util/LiveActorUtil.cpp; include/Game/Player/MarioActor.hpp; include/Game/Util/FurMulti.hpp; include/Game/Util/LiveActorUtil.hpp; this notes directory. `root.patch` records only these changes against the frozen prior checkpoint. `native.patch` is reviewable and does not edit xmake.
