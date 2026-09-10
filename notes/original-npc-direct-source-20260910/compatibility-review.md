# Compatibility reduction review

The user asked why so much Compat code exists, and whether expanding shared platform support should let us delete specific tweaks. Yes: the intended direction is original Game code running on a growing shared implementation of the Wii environment. A source file that remains byte-identical behind token substitutions is not sufficient evidence of unchanged behavior.

## Distinguish responsibilities

1. Platform implementation is necessary: host input into original WPAD/KPAD records, OS memory/time/thread behavior, disc/NAND access, GX rendering and SDK data layouts. Put reusable implementations in Aurora when their ownership belongs there.
2. Native resource and lifetime integration is necessary where the host ABI differs: decode big-endian resource bytes into correctly aligned native fields, retain the actual J3D/JPA/Game objects, release them at their real lifetime boundary. Share this at the owning system level; avoid actor-specific metadata mirrors.
3. Game replacements are migration debt: duplicated Game algorithms, substitute scene/managers, source wrappers and per-actor special behavior should shrink as their actual dependencies become available. The removal gate is original-code/runtime equivalence, not fewer files with a particular suffix.
4. Some `Original*.cpp` providers already contain extracted original Game routines. Do not mistake those for invented behavior merely because they are outside Game. Prefer the complete original translation unit when it can be activated coherently, deleting duplicate extracted definitions in the same change.

## Measured examples in this checkpoint

| Surface | Current evidence | Next disposition |
| --- | --- | --- |
| NPCActor wrapper | Original TU compiles with just one qualified member-function pointer; model/spine substitutions obsolete | Deleted; normal original Game TU enabled |
| NPC joint factory/callback rejection | Actual J3D models, matrix buffers and two-phase traversal exist; fresh original JointController Wii proof 10/10 functions at 100% | Deleted; original shared JointController enabled with runtime verification |
| LodCtrlSource.cpp | Direct original TU passes syntax; both original TU and wrapper were listed in the current compile database; derived ModelObj wrapper adds no behavior | Candidate for removal after checking actual LOD initialization/lifetimes; do not confuse syntax with that runtime proof |
| StarPieceCompat.cpp | Direct original TU passes syntax; the source no longer has the substituted allocateDelegator call, and literal gravity host 0 already matches its u32 declaration | Candidate for removing obsolete wrapper and enabling the direct original TU, with star-piece lifetime/sensor validation |
| FileSelectFuncSource.cpp | Direct TU compiles, but wchar_t width still matters: original code copies Wii 16-bit names while native wchar_t is 32-bit | Replace with a consistent fixed-width Game text boundary; deleting the macro solely because syntax passes would be incorrect |
| NPCActorRuntimeCompat.cpp action/reaction helpers | Five active rejection functions; initial ten-function expansion also exposes missing rail-pose helpers. A closed five-function reaction/turn subset can remove three rejections first | Recover in decomp first; activate original NPCUtil behavior, not new per-NPC action logic |
| TalkRuntime / DemoSceneRuntime | Parallel host talk/demo state; programmable demo requests still unavailable; actual TalkDirector/DemoDirector dependency chains incomplete | Restore original directors and helper owners against shared scene movement, camera, message and player-control facilities; do not expand a bunny-specific demo substitute |
| SaveDataHandleSequenceCompat | Null child constructor and rejected operations despite current original UserFile/GameDataHolder support | Restore actual save sequence, shared NAND queue and error/reset ownership together; do not fabricate a partly populated GameSystem |

The three remaining source-wrapper syntax probes are in `wrapper-audit/results.json` and their logs. They are an inventory, not a claim that those runtime paths have been restored. The broader NPC/stage dependency evidence is in `../original-gateway-sequence-20260910/`.

## Working rule

For each replacement: identify the original owner and calls, recover missing Game code in decomp, implement the missing platform/resource behavior at its common boundary, enable the original Game implementation, validate real object lifetime and observed behavior, and delete the superseded provider in that checkpoint. Keep unsupported paths explicit until their dependencies work. Small direct compiler/ABI edits are preferable to hidden macro substitution when a shared boundary cannot express the required adaptation.

This checkpoint does not claim an ordinary fully initialized Gateway scene or the complete rabbit/Rosalina sequence. The existing movement demo still uses a bounded development route.
