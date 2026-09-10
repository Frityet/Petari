# Original Demo cast and keeper integration

This cohort imports the complete original DemoCastGroup, DemoCastGroupHolder, DemoCastSubGroup, DemoSimpleCastHolder, DemoSubPartKeeper, DemoActionKeeper, DemoPlayerKeeper, DemoCameraKeeper, DemoCameraFunction, DemoTalkAnimCtrl, and DemoPositionController sources and headers. Director, DemoFunction, DemoExecutorFunction, StartRequest, and native owner/routing changes belong to the parent integration task.

## Reference-first corrections

Three reference CPP changes are required before native adoption:

- DemoCameraKeeper::update: return when `_4 <= _C`, preserving the original valid interval `0 <= current < count`. Retail 800B978C compares count with index, 800B9790 branches **into** the loop when count is greater, otherwise 800B9794 exits. The previous source returned on count>index and therefore skipped every nonempty camera table immediately after start. Full Wii compile succeeds; baseline fuzzy96.6129%, corrected96.451614%. The slightly lower score is not a semantic regression: the comparison/control-flow evidence is decisive. Other methods remain unchanged, including actual camera dispatch and continuous-camera behavior.
- DemoCameraFunction::isCameraTargetMario: replace the bool-returning `return nullptr` with `return false`. Retail800B9624 returns zero. Full Wii compile/objdiff remains100% for all three methods; Clang can compile the original method without a custom Game workaround.
- DemoSimpleCastHolder: include the real LayoutActor declaration for the original LayoutActor*→NameObj* conversion. All checked full-TU Wii symbols remain100%. No behavioral change.

The guide used is `decomp/AGENT_DECOMP_GUIDE.md`. Baseline and recovered full-TU Wii commands, logs, source hashes and per-symbol matches are in the corresponding `*-proof.json` files. Retail references are `notes/gateway-audit-20260907/restoration/retail/{asm,obj}/Game/Demo/`.

## Unchanged-source proof

All eleven original TUs compile with the Wii compiler. Baseline functions are95–100% except the JMapIdInfo-based DemoCastGroup::tryRegisterDemoActor (18.7%): its equality operator is inlined by the current shared header but was out-of-line in retail. The original operator at800B9ED8 checks both the ID and zone, exactly as the current header does; no functional correction or global header change is justified. Do not describe this whole cohort as uniformly95–100%.

All fourteen checked DemoTalkAnimCtrl symbols are100%, including camera pause/resume, animation control publication, first-part setup and the unusual repeated frame assignment. No speculative changes were made to those bodies. The apparent ActionType9/10 duplication and ActionType11 no-op in DemoActionKeeper also match retail and remain original.

## Native prerequisites and evidence

`probe-native.py` first compiles unchanged/reference-corrected sources through the actual CP932 wrapper in a scratch native header overlay. `native-probe.json` records eleven successful full native compilations. It includes the current native MessageHolder.hpp, preserving its BMG ownership extensions; the prior audit's missing MessageHolder header is stale. Added only the two original MR declarations absent from native ObjUtil.hpp (`isName`, `isSame`). The final copied CPP/header pairs are all exactly equal to reference. `final-native-proof.json` and `verify-native.py` record/reproduce eleven successful CP932-wrapper full-TU compiles using only the real native source/header paths, without the scratch overlay. `source-manifest.json` records all45 reference/native paths.

`final-link-frontier.json` compares the eleven objects, the previously recovered Time/Wipe/Sound/Executor objects, and the four current smg-pc archives. It records archive size/mtime and exact missing Game symbols. The remaining non-Demo providers are MR::isName/isSame (original ObjUtil.cpp862/866) and MR::reflectBckCtrlData (original LiveActorUtil.cpp1769, which publishes to the actual Xanime player and optionally the original sound object's loop bounds). Parent owns restoring those providers. libc strcmp/strlen/bzero imports are normal runtime dependencies. This snapshot is not a request to add success stubs.

The whole original DemoDirector/Executor graph still needs parent integration and linked/runtime validation. Compilation of these prerequisite archive sources alone does not demonstrate a running authored camera, actor action, or Gateway bunny sequence.

## Frozen handoff

The native source/header cohort and three reference corrections are frozen. No root Xmake invocation, index mutation or commit was performed. Parent owns shared utility definitions, real Director ownership, linked tests and runtime evidence. Compiler success and imported archive members are not presented as active scene behavior.
