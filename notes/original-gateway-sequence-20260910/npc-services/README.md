# Original Gateway NPC services audit — 2026-09-10

This is a read-only source/retail audit, followed by a parent-authorized test update. No NPC recovery, production changes, root Xmake, commits or runtime validation were performed by this task. The parent is independently removing the NPC include wrapper and integrating the original JointController. Provider observations below describe the pre-removal snapshot; they must not be mistaken for the final parent change.

## Main finding

NPCActor was already executing its original source. `src/Game/xmake.lua` excluded the direct CPP because `NPCActorCompat.cpp` included `NPCActorSource.inl`, which included that same original CPP. The wrapper substituted actual model presence and actual Spine access, but replaced the joint-delegator factory with an unconditional thrower. Removing that wrapper is an understandable native compile-boundary cleanup; it does not implement the still-missing NPC reaction, message or demo systems.

At the audit snapshot the main shared blockers were:

| Surface | Current native behavior | Original dependency / effect |
| --- | --- | --- |
| NPC wait/turn/reaction | Five MR functions in `NPCActorRuntimeCompat.cpp` unconditionally throw | Original NPCActor/Tico wait and reaction nerves call these helpers |
| Programmable catch/comment demo | `requestStartDemoMarioPuppetable` has no active native definition; the original `DemoUtil.cpp` is excluded | RunawayRabbit catch and RunawayTico comment methods require nerve-delivering demo requests |
| Talk director identity | SceneObj_TalkDirector factory returns `TalkRuntime : NameObj` | Original owner is `TalkDirector : LayoutActor` with balloon/state holders and TalkPeekZ |
| Talk puppet control | `TalkRuntime::Impl::start` ignores its fourth bool | `startTalkForceWithoutDemoPuppetable` reaches the same host start path without that distinction |
| Normal talk demo | Host start rejects demo_type 1 | The original normal programmable-talk ownership path cannot run |
| Talk presentation | `TalkMessageCtrl::updateBalloonPos` is empty; camera dispatch throws | Original balloon positioning and camera remain unavailable; pressing A closes the host presentation without the original text/page state machine |
| Broadcast messages | Scheduler-entry iteration excludes suspended actors | Original AllLiveActorGroup iteration excludes only dead actors and the supplied sender |

There is no standalone `EventDemo` class in this audited path. The relevant distinction is the original programmable demo request/owner versus the currently retained DemoSheet/time-keep host runtime.

## Concrete Gateway route

`decomp/src/Game/NPC/RunawayRabbitCollect.cpp` creates actual RunawayRabbit and RunawayTico children from the authored child iterator, then matches rabbit group IDs to Tico demo cast IDs. `linkMsgCtrl` assigns the Tico's existing TalkMessageCtrl to the rabbit. This is a shared borrowed controller identity; a second controller per rabbit would change the original flow and ownership.

The collector sends ACTMES_HEAVENSDOOR_RUNAWAY_RABBIT_WAIT after placement, later START when the guide Tico reports the original runaway state. Completion counts groups, not the raw number of rabbit placements. This audit does not claim native collector initialization or gameplay completion.

`RunawayRabbit::receiveOtherMsg` requests the `捕まり` programmable demo with original success/failure nerves. Its caught-talk nerve calls `tryTalkForceWithoutDemoMarioPuppetableAtEnd` on that borrowed controller, then enters caught-end. `RunawayTico::appearBushComment`, `appearHoleComment`, `appearPipeComment` and `appearMamaComment` select original flow branches and request the `ぼやき` demo. These paths require actual demo request arbitration and correct talk lifecycle rather than directly changing the rabbit nerve in native code.

RunawayRabbit inherits LiveActor; RunawayTico inherits Tico, then NPCActor. RunawayTico installs its own guide/wait/appear/talk nerves, so the NPC base throwers are a shared class/link frontier, not proof that every first Gateway tick executes NPCActor::exeWait. Tico's normal wait/reaction paths do use them. Tico's base caps do not set `_70`, so the old joint-delegator rejection is not established as the immediate Gateway Tico initialization blocker.

## Bounded restoration candidate

The smallest coherent non-rail action/reaction candidate is five original MR functions:

1. `isActionLoopedOrStopped` — 0x803EEF5C, 0x4C bytes.
2. `tryStartTurnAction` — 0x803EF244, 0xE4 bytes.
3. `tryStartReaction` — 0x803EF394, 0x328 bytes.
4. `tryStartReactionAndPushNerve` — 0x803EF7F4, 0x58 bytes.
5. `tryStartReactionAndPopNerve` — 0x803EF84C, 0x8C bytes.

The reference `NPCUtil.cpp` currently contains TakeOutStar/FadeStarter/DemoStarter class implementations but none of these MR definitions. All five have saved retail assembly in `notes/gateway-audit-20260907/restoration/retail/asm/Game/Util/NPCUtil.s`. Their direct external requirements are existing original NPCActor turning/Spine operations and current BCK/action predicates/providers. A future task should recover them in decomp first, compile the whole Wii TU and record per-symbol objdiff, copy the exact recovered source to native, then remove only the three corresponding native throwers.

The two talk-action throwers require more than forwarding to TalkCompat: `tryStartTalkAction`, `tryStartMoveTalkAction` and `startMoveAction` add original rail movement and followRailPose/followRailPoseOnGround dependencies. These pose helpers are also absent. `npc-action-closure.json` records ten first-hop functions and exact retail calls; it deliberately does not claim that ten functions form a complete link closure. The five-function non-rail candidate is therefore a better bounded recovery than promising all talk/rail behavior in one small change.

## Sensor state: useful existing owners and a concrete remaining difference

`GameActorSensorCompat.cpp::add_joint_sensor` resolves `MR::getJointMtx` and calls the actual `HitSensorKeeper::addMtx`. `StarPointerServiceCompat.cpp::initStarPointerTargetAtJoint` similarly passes the real matrix to `initActorStarPointerTarget`. Rabbit Body/Catch sensors use the Spine joint. These surfaces should be reused and tested with the actual model; there is no need for a rabbit-specific sensor service.

Broadcasting is different. Original `ActorSensorUtil.cpp:698` and retail 0x803C47C8–0x803C4888 use `getAllLiveActorGroup`, check only `isDead` and sender equality, then call receiveMessage. The loop reloads the group's current count. Native `SceneScheduler::send_message_to_live_actors` uses a scheduler snapshot, deduplicates registrations and adds an execution-suspended exclusion. A suspended but living receiver therefore misses collector messages. This is an independently demonstrated service-semantic mismatch, not a runtime claim that a particular authored receiver is suspended. Restore original group ownership/traversal in a separate generalized change; do not patch collector messages.

## Full original talk activation remains a larger owner cohort

Fresh code confirms the original TalkDirector CPP is not in native src, while TalkRuntime supplies TalkMessageCtrl/TalkNodeCtrl/TalkFunction bodies and owns flow copies and callbacks. The earlier `notes/original-talk-director-owner-audit-20260907/README.md` has useful Wii/isolated-object evidence, but that is not current linked/runtime proof.

Current missing original owners include MessageData/MessageHolder, native TalkDirector/TalkBalloon/TalkState/TalkTextFormer, and actual DrawSyncManager implementation. TalkPeekZ's original constructor requires a real callback registered with DrawSyncManager, then FIFO draw-sync token/projection/depth handling. Retaining current host BMG or renderer metadata does not make it a MessageData or DrawSyncManager object. TextBox-derived tag/reveal support remains part of the talk activation scope. Save flag and camera services have changed substantially since the prior note, so its entire historical undefined-symbol list was not reused as a current claim.

Replacing only the SceneObj factory cast would be invalid: the controller and host tear-down code currently depend on TalkRuntime identity. A coherent future migration must remove duplicate providers and replace the entire owner graph with real resource and lifecycle boundaries.

## Focused test coverage and authorized changes

Existing target: `smg-pc-npc-actor-real-or-absent-tests` (`tests/NPCActorRealOrAbsentTests.cpp`). Prior cases covered caps, reaction edges, base transforms, GroupCheckManager names/ownership, missing item/swing access, AnimScale rest and float offset. No test exercised NPC push/pop/null nerve or joint-delegator callback substitution. TalkRealOrAbsentTests only uses its synthetic NPC subclass for multi-controller ownership, not model or nerve execution.

The parent authorized these fixture changes:

- Add a real `initNerve`/Spine test to NPCActorRealOrAbsentTests. It checks the exact executor and Spine pointer, executed base step, pending push, pending popAndPush, completed replacement, pop return identity, restored base, original null nerve execution, repeat-null rejection and restoration. Probe Nerve objects are real instances; no fabricated Game pointer is used.
- Replace removed actor_base_matrix access and invalid model-less transform calls with a real Tico ModelManager/J3DModel loaded from SMGPC_REAL_DISC under the actual SceneExecutionFixture. Float-base assertions inspect the original model matrix. Caps and reaction tests remain model-free where the original method does not require a model.
- Move all six old talk/player-up float assertions to `tests/OriginalPlayerUtilTests.cpp`, invoked by the existing Gateway `--player-util` route. The old plain LiveActor occupying the player slot was invalid after original PlayerUtil activation. New assertions use the already initialized actual MarioActor, retain the six old expected results, add a changed-up-vector check, and restore position/up with scope. NPC borrows a real scene-registered TalkMessageCtrl for the scalar predicate; this does not claim full TalkDirector or balloon behavior.

The standalone NPC executable still has six counted groups because its former isolated base-transform group is now combined with the real resource/float matrix group and the new Spine group is added. The actual-Mario executable prints a separate explicit NPC talk-height proof line. No test group is silently skipped; the real resource test requires SMGPC_REAL_DISC.

Validation is owned by the parent. First NPC target build passed, then its new model assertion exposed a fixture mistake: original ModelManager::initModelAndAnimation stores animated models in mXanimePlayer->mModel and leaves mModel null. The corrected fixture uses getJ3DModel and additionally proves that branch plus exact Tico ResourceHolder model identity; it does not relax the resource requirement. The next run reached the float-dispatch assertion and exposed a second fixture issue: real model initialization/setBaseMtx already execute the virtual override. The corrected test snapshots virtualCalls immediately before the qualified float operation and requires no increment from that operation. Final parent rerun passed all six NPC groups with the real Tico model, original Spine transitions and clean process exit. `notes/original-npc-direct-source-20260910/run-npc-final.log` contains the model proof and `NPCActor real-or-absent tests passed: 6/6`.

The parent reports Gateway --player-util passed with every relocated NPC float assertion. Its run-player.log was also inspected: it prints the relocated NPC proof and completes scene teardown with `[ok] real original PlayerUtil ownership proof`. It also reports its separate new original JointController tests passed, including actual NPC instances, shared Tico resources and repeated scene generations; that fixture is parent-owned and is not claimed as work from this task. Parent evidence is under notes/original-npc-direct-source-20260910/.
