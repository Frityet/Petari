# Original shared Demo prerequisites, 2026-09-10

Recovered complete `DemoTimeKeeper`, `DemoWipeKeeper`, and `DemoSoundKeeper` translation units from retail assembly, and restored the five omitted talk-animation loops in `DemoExecutor`. The reference was edited first under `decomp/AGENT_DECOMP_GUIDE.md`; the final four native CPPs are byte-identical copies. Exact native dependency headers were copied too. `source-manifest.json` records every owned reference/native path and hash.

These are ordinary Game archive sources (`src/Game/xmake.lua` already uses `**.cpp`). The real DemoDirector/Executor graph is **not instantiated by the current native runtime**. This checkpoint does not replace the active host DemoSheetRuntime/DemoSceneRuntime clocks, prove a running authored demo, or complete the Gateway bunny sequence. No fake Director/Executor, copied clock wrapper, JMap special case, root Xmake changes, or commits were made by this subtask.

## Wii and native evidence

The original GC3.0a3 full-TU compiler commands and bounded objdiff commands are recorded in each `*-proof.json`. Retail objects and assembly are under `notes/gateway-audit-20260907/restoration/retail/{obj,asm}/Game/Demo/`.

| Source | Checked recovery | Wii match |
|---|---|---|
| DemoTimeKeeper | 11 complete methods | 93.625–100%; six at 100% |
| DemoWipeKeeper | ctor, info ctor, start/update/execute, virtual methods, template and thunk | ctor 99.74026%; other nine symbols 100%; both vtables 100% |
| DemoSoundKeeper | complete raw retail-write baseline | ctor 97.19512%; other nine symbols 100% |
| DemoSoundKeeper | **final portable leading-byte recovery** | ctor **87.63415%**; other nine symbols and all three vtables 100% |
| DemoExecutor | movement | 65.08108 → 100% |
| DemoExecutor | start | 70.126434 → 97.70115% |
| DemoExecutor | startPart | 31.772728 → 100% |
| DemoExecutor | startDemoSystemPart | 42.192307 → 100% |
| DemoExecutor | tryStartDemoSystemPart | 38.96552 → 92.93104% |

Do not describe the whole Executor TU as 93–100%: its unchanged preexisting `end()` is 76.89744%. The recovered talk loops are the five methods above. Retail `end()` confirms no omitted talk-controller end loop and calls the same `MR::endDemo` for request types 1 and 2.

Final proof files: `recovered-final-proof.json` (Time), `wipe-refined-proof.json`, `sound-raw-reference-proof.json`, `sound-portable-final-proof.json`, `executor-baseline-proof.json`, and `executor-talk-loops-proof.json`. Native `final-native-proof.json` contains four successful full-TU LLVM23 compilations with the actual final CP932 wrapper, original editable native sources, and native headers only; no scratch header overlay was needed. Per-TU `*.charset.json` records preprocessing. `verify-recovered-native.py` reproduces these isolated compiles. This is compilation/import proof, not linked/runtime owner proof.

## Original semantics retained

TimeKeeper owns actual `DemoTimePartInfo[]` parsed through `DemoFunction::createSheetParser`. The field called `mSubPartInfos` is the current **main** part pointer. `start()` chooses part zero without resetting the counters; `end()` resets them. A paused update still advances counters through zero. Suspend behavior, the unusual final boundary comparison, and caller validation of named parts are preserved. The source does not invent a missing-part guard or alter original invalid-input behavior.

Wipe and Sound use the actual two-base layout: DemoSheetKeeperBase holds the Executor, and DemoSheetKeeperInfoHolder holds the array and its own virtual execute dispatch. Corrected the reference const virtual declarations, supplied the original base constructor and info-holder update/execute bodies, and restored Sound's byte-width ReturnBgm and inherited base start slot. Wii vtable matches verify this layout. Explicit `NO_INLINE` template instantiation preserves the retail out-of-line push_back without changing shared Array code.

Executor calls talk update after the unpaused keeper block, so talk updates continue while the demo clock is paused. Start initializes talk animations after sheet start and before actor broadcast. All three part-entry methods configure talk-part state before requesting/starting the demo, including a failed try-start. The recovered bind2nd/member-function iteration uses the existing MSL conventions and active counts.

## Sound ReturnBgm endian recovery

This is a retained retail width mismatch, not a normal boolean conversion. `DemoSoundInfo` initializes one byte at offset 0x0C (`800BD950`, stb). The constructor passes that same address to `JMapInfo::getValue<s32>` (`800BDA54–58`). The actual template at `800B8BE8` in DemoActionKeeper assembly calls the s32 getter; `JMapInfo::getValueFast(s32*)` writes all four bytes. The copy at `800BDC7C` and execute at `800BDBC0` read the **leading byte** with lbz. On big-endian Wii that is bits31–24.

A raw cast into the byte member matches codegen well, but would read the low byte on the little-endian host and also relies on an invalid typed store across a byte member/padding. The final original recovery reads into an initialized s32 local and assigns `static_cast<u32>(returnBgm) >> 24`. Missing/rejected fields remain zero. ReturnBgm=1 remains false; -1 becomes 255; 0x01000000 becomes 1. No guessed `!=0` conversion is used. The isolated raw baseline is preserved as `DemoSoundKeeper.raw-reference.cpp`; it is not copied into native Game.

`sound-byte-semantics.json` records the actual assembly locations, representative values, all signed 8/16-bit inputs, and deterministic 32-bit samples compared with a big-endian store/leading-byte model. This is scalar semantic evidence, not an authored-disc or running sound-demo claim. The additional temporary accounts for the explicitly reported lower final ctor codegen match. A less clear retained-byte variant reached 88.86585% and was discarded; clarity and functional equivalence take priority.

## Whole-owner integration path and remaining closure

The real constructor path must be DemoDirector::init → original DemoFunction::loadDemoArchive (`DemoSheet.arc`) → original Executor::init → each actual keeper. `DemoFunction::createSheetParser` already exists in the reference and resolves `Demo%s%s.bcsv` through the real Director's ResourceHolder. Feeding TimeKeeper with an invented partial Executor/Director would defeat that ownership, so the existing host clock is unchanged until that complete owner graph is ready. Once active, clock queries should delegate to actual TimeKeeper and the duplicated host clock fields can be removed.

`existing-native-probe.json` is a bounded unchanged-source import survey of twelve neighboring classes. CastGroup, Executor, SubPartKeeper, PositionController, PlayerKeeper, CameraKeeper and TalkAnimCtrl compile against a scratch copy of exact reference Demo headers. The initial failures are declaration/include prerequisites: CastGroupHolder lacks the MR::isName declaration; CastSubGroup references absent native MessageHolder.hpp; SimpleCastHolder needs a complete LayoutActor for conversion; ActionKeeper lacks MR::isSame. DemoCameraFunction additionally has a reference compile defect (`return nullptr` in a bool function); it needs retail-backed correction before adoption. This survey is not proof that those complete classes are runnable or fully recovered.

`recovered-cohort-link-frontier.json` records the current four native archive identities by size/mtime and the recovered objects' missing imports. Beyond original cast/keeper classes and DemoFunction registration/resource methods, current missing exact native symbols include `MR::startLastStageBGM`, `MR::isInvalidClipping(const LiveActor*)`, and the NameObj-based three-argument time-keep request overloads. Root owns that integration and the Director/StartRequest/DemoFunction/ExecutorFunction cohort. Archive import absence is a snapshot, not an instruction to add stubs.
