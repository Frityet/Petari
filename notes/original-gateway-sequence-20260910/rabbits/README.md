# Original Gateway rabbits — 2026-09-10

Initial closure audit, before native activation. Parent owns native shared services/factory/StageHost; this subtask owns Rabbit*/RunawayRabbit* sources and headers. The four existing user changes (`DISCREPENCY_REPORT.md`, `MACOS.md`, `notes/original-sequence-galaxy-move-20260910/README.md`, `script/package_walking_demo.py`) were inspected and preserved. No commits, Xmake, or native Game imports have been performed at this checkpoint.

## Actual required actor graph

Gateway's actual collector creates RunawayRabbit and RunawayTico child placements, links rabbit message controllers by matching group ID, activates rabbits on a child's original runaway-start state, records catches, and selects the remaining hint/mama dialogue. RunawayRabbit implements its own appear, runaway, catch, puppet-Mario talk, toss and stop nerves. It **does not use** RabbitStateCaught or RabbitStateWaitStart. Those two reusable states are present in reference and separately audited, but need not expand this Gateway import.

Native currently lacks the two primary actor TUs/headers and the following direct prerequisites:

- RunawayTico, whose original base is Tico.
- WalkerStateRunaway (+ parameter class) and WalkerStateBlowDamage.
- SpotMarkLight (original PartsModel subclass).
- TrickRabbitUtil::createRabbitFootPrint.
- Original ActorStateUtil provider/header, used to advance the Walker states.

FootPrint source/header already exist natively, as do PartsModel, ActorStateBase, BaseMatrixFollowTargetHolder and TalkMessageCtrl. This does not claim that every shared service used by these actors is behaviorally complete; parent owns that integration review.

## Fresh source and retail proof

All four reference TUs are already recovered and compile with the Wii toolchain. Their current text-section objdiff matches are:

| Source | Text bytes | Match |
| --- | ---: | ---: |
| RunawayRabbitCollect | 2468 | 97.40519% |
| RunawayRabbit | 5720 | 95.95944% |
| RabbitStateCaught | 1604 | 99.86285% |
| RabbitStateWaitStart | 2604 | 99.93088% |

The lowest per-function fuzzy scores are RunawayRabbit init/updateBindActorMatrix and several nerve predicates (~85–90%), plus collector exeActive (~90%). These scores are recorded without claiming a new semantic recovery or perfect match. No source changes have been made to improve compiler scores.

`wii-results.json` records exact compiler commands and individual scores. Each `*-objdiff.json` is a fresh complete object comparison. `retail-byte-proof.json` independently compares all 129 assembly functions against actual `main.dol` bytes (SHA1 `25c5959534b3c21246c6c7e42021b916b41fb578`). `reference-source-manifest.json` records the exact reference source/header hashes.

One suspicious-looking collector expression was checked directly: its final `MR::isValidSwitchA(this)` return really is discarded in retail at **0x80286EE4**, followed by BGM state change and virtual kill. It must not be replaced with an invented switch write. The NoActive and TryCaughtDemo nerve bodies also correspond to original empty nerves, rather than unimplemented native behavior.

## Native compile frontier

An isolated header overlay (`headers/`, `overlay-header-manifest.json`) supplies exact missing reference declarations without publishing them into the native tree. Fresh native results are in `native-overlay-compile.json` and per-TU logs:

- RunawayRabbitCollect: compile0.
- RabbitStateWaitStart: compile0.
- RunawayRabbit: fails because native Game/Util.hpp omits original ActorStateUtil/BaseMatrixFollowTargetHolder/TalkUtil declarations, and native NERVE_DECL_NULL automatically defines an inline singleton even though this original source explicitly defines the two singletons.
- RabbitStateCaught: fails only on missing createPowerStarDemoModel declaration; deferred from the actual RunawayRabbit graph.

The required general Nerve/umbrella boundary is reported to the parent. Deleting original singleton definitions only in RunawayRabbit would hide that broader macro-contract mismatch. Exact actor imports should follow coherent helper/header ownership, not leave a partially compilable native glob during a shared build.

## Pause at user steering

The parent requested audit-only completion while the user's current priority is removing NPCActorSource.inl and retaining direct minimal edits in NPCActor.cpp. Further rabbit/helper imports are paused. No reference/native production sources were edited by this rabbit task, and there is no active build or debugger. The complete candidate sources remain in decomp; the exact overlay/proofs above are saved for a later coherent activation. Initialization's independent audit additionally identifies shared NPCUtil action/reaction throwers and missing or incomplete Mario-puppet demo ownership as dependencies of the Tico/catch route; those findings belong to its separate audit, not a rabbit-specific workaround.

## Resumed import preparation

The next exact import is 11 existing source/header pairs: NPC/{RunawayRabbit, RunawayRabbitCollect, RunawayTico, Tico, TicoDemoGetPower, TrickRabbitUtil}, Enemy/{WalkerStateRunaway, WalkerStateBlowDamage}, LiveActor/SpotMarkLight, Util/ActorStateUtil, and Demo/AstroDemoFunction. The latter two Tico dependencies are retained by its actual original base init/vtable. `next-import-manifest.json` records each reference hash and destination; all 22 native destinations were absent at preparation. RabbitStateCaught/WaitStart remain outside this graph.

The four primary rabbit source/header hashes still match the earlier proof above. No repeat rabbit builds or new gameplay tests were run. The native umbrella still needs the original ActorStateUtil/BaseMatrixFollowTargetHolder/TalkUtil declarations, and native NERVE_DECL_NULL's automatic inline instance conflicts with the original rabbit's explicit instance definitions. These shared compiler contracts belong to the parent integration.

The missing original `MR::trySetMoveLimitCollision` and its two small keeper helpers have now been recovered in reference, including the retail Binder enable flag that the old inline omitted. See [movement-limit evidence](move-limit/README.md). Native imports remain held for the coherent demo/talk checkpoint. NPC action/reaction throwers from the initial audit have since been retired through the original NPCUtil and RailUtil work; this note does not claim the full rabbit chase is active.
