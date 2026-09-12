# Original Rosalina tower demo owner

Recovered and mirrored the complete RosettaDemoHeavensDoor translation unit. Both native and original-compiler probes pass. Native source and both imported declarations are byte-identical to reference. No source list, factory, process owner, or compatibility behavior changed in this pass; the PowerStar checkpoint is untouched.

The reference previously omitted all five first-scenario callback registrations and the second scenario's callback. These now use the original MR functors and actor-owned nerve transitions:

| Owner / part | Original callback |
| --- | --- |
| HeavensDoor1 / 高楼出現[デモ] | preDemo |
| HeavensDoor1 / 高楼出現[デモ後] | pstDemo |
| HeavensDoor1 / 高楼出現[フェードアウト] | fadeOut |
| HeavensDoor1 / 高楼出現[フェードイン] | fadeIn |
| HeavensDoor1 / スピンゲット[デモ1] | changeNerve to its Demo nerve |
| HeavensDoor2 / 郷愁[開始] | changeNerve to its Demo nerve |

Both missing changeNerve template bodies use the original NerveExecutor::setNerve with the corresponding nerve singleton. The first constructor uses the out-of-line MR::Functor helper, while the second uses the existing inline helper, matching the original call sites. The existing preDemo/pstDemo/fadeOut/fadeIn implementations remain original.

The same constructor audit recovered omitted behavior before those registrations: after the LightDome and LightHalo models are created, both original virtual calls are makeActorAppeared (Wii vtable offset 0x28), not makeActorDead. LightHalo then disables its fixed-position matrix updates and starts at the original world position `(15064.593, -7917.67, 7541.112)`. Its fadeIn callback reenables those updates. These values come directly from the retail constant pool at 806BEE54/58/5C and the stores at 8028403C-64. They are original authored behavior, not a new host positioning rule. The second scenario's completed-event branch also uses makeActorDead (offset 0x30), correcting its previous kill call.

Original compiler evidence: full .text improves from 72.34433% to 98.879944%. Constructor 1 matches 99.748856%; constructor 2 matches 99.86667%. The remaining constructor differences are string-pool relocation/addend representation. Both new changeNerve functions match 100%; existing tower callbacks, first-scenario Wait/Fade handlers, and both empty second-scenario routines match 100%. The pre-existing first-scenario exeDemo body remains 92.81309%; this pass does not claim a perfect whole-TU match. Full before/after objdiff files, exact native/reference commands, successful logs, and hashes are in `source-proof.json`.

`native-provider-audit.json` records every direct undefined symbol against the current shared Game archive and its SHA. Three direct Game calls remain absent: MR::createPartsModelNpc, MR::invalidateShadowAll, and Rosetta::startDemo. The first two belong to the existing general LiveActorUtil and ActorShadowUtil boundaries. Rosetta itself currently has only makeArchiveList recovered in reference; its real constructor/init/control/demo lifecycle is a separate necessary host-actor recovery. Importing Rosetta.hpp provides the actual declared object layout for this complete demo owner; it does not claim that the Rosetta actor can yet be constructed or activated. No fake Rosetta or standalone RunawayTico creator is introduced.

The first constructor's five registrations can now be consumed by the original DemoActionKeeper when a real Rosetta host is available. This is source recovery and compilation evidence, not a visible tower/Rosalina runtime result. Root owns the ordinary Gateway executable and initialization validation.

Exact reference and native path manifests are beside this file. Configure status can become Equivalent for the recovered reference TU at publication; no shared configure.py edit was made here.
