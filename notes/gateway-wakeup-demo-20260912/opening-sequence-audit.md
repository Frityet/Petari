# Original wake-up through Rosalina: source audit

The first fresh showcase build failed at link time (`first-build.log`). None of the runtime gaps below is a newly observed live failure. The authored data cited here is the retained retail extraction in `notes/gateway-demo-sheet-oracle-20260806T234325Z/`; it has not been re-extracted in this pass.

## Earliest construction gap

`Showcase.cpp:728-759` creates RuntimeContext and a genuine selected-file GameDataSession, selects the post-castle story event, then constructs GatewayDemoScene. It does not create the original GameSystem/scene controller or GameScene. GatewayDemoScene subsequently constructs SceneObjHolderBinding (`GatewayDemoScene.cpp:198`). Its SceneInitializationBinding now requires the real GameSystem scene controller (`SceneInitializationState.cpp:14-19`). StageSessionState is not that owner and cannot replace it.

This is a concrete source-level dependency in the current setup, before successful authored actor initialization can be assumed. Original SceneUtil also resolves child placement from the actual StageDataHolder (`SceneUtil.cpp:203-215`). The current showcase's retained StageAuthoredData tables are not evidence that this original owner is installed. The intended chain is original GameSystem scene controller -> real scene lifecycle/StageDataHolder -> original placement initialization -> execution in the real GameScene, with existing general compatibility services supporting those owners.

GatewayDemoScene currently sets its host execution phase to Gameplay after placement. That is not original GameScene::start/startStagePlayFirst or proof of its opening-camera/ScenarioStarter/Action nerves. `MarioActor::initAfterOpeningDemo` starts the walk-in animation and is not the Gateway wake-up trigger.

## The opening trigger is already part of the rabbit owner graph

The placed `RunawayRabbitCollect` is explicitly unavailable in the native creator table (`src/scene/nameobj/NameObjFactory.cpp:327-328`). This also removes the opening trigger, not only the later chase.

Its original `init` (`RunawayRabbitCollect.cpp:36-69`) enumerates child placement rows and creates RunawayRabbit and RunawayTico children. RunawayTico does not need a separate placed-object factory entry for this path. Its cast-0 child initializes to Guide0 (`RunawayTico.cpp:55-80`). If the guide-complete save flag is already set it dies in initAfterPlacement; the ordinary Gateway showcase selects the earlier post-castle checkpoint, while only the separate spin route later advances the guide-complete event.

On its original Guide0 first step (`RunawayTico.cpp:177-184`), the Luma:

1. Starts `チコガイドデモ` as a Mario-puppetable time-keep demo, with no starting-part override.
2. Sets the original guide-complete game-event flag.
3. Ends the start-position camera and starts its `DemoMeetTico` animation camera targeting Mario.
4. Forces the cinema frame.

The retained Time table begins with `チコとの出会い[開始]`, 930 frames. The matching Player row sets Mario to `MarioDemoPos` and starts `demomeettico`. The original DemoPlayerKeeper reads `PartName`, `PosName`, and `BckName`, then calls the real player-position and animation methods on that part's first step. The matching Action row invokes RunawayTico::setDemoTrans, which starts the Luma's `DemoMeetTico` animation and copies Mario's base matrix. This is the authored wake-up animation/camera route; adding a showcase timer or manually starting a replacement sequence would bypass it.

The following parts transform the Luma, appear DemoRabbit cast 0, play Mario's `demomeetticowatch`, invoke the original rabbit talk nerve, then guide the player into the chase. Native original DemoDirector/Executor/Player/Action/Camera/Time keepers exist, but their presence is not evidence that this whole owner chain currently runs. Original TalkDirector, the message data, player demo control, NamePosHolder ground placement, animation camera, and real scene stop/resume must all remain operative.

## Chase through Rosalina's appearance

The authored `ウサギ追いかけ[開始]` Action row calls RunawayTico::startRunaway. The collector observes it, activates its original rabbit children, and broadcasts the original rabbit-start message (`RunawayRabbitCollect.cpp:185-196`). The original rabbit sensors/capture state and shared talk controllers must drive completion.

After the last catch, the corresponding original Luma comment sets mIsAllCaught. Finishing that conversation selects WhiteOut; its WhiteIn later starts the same `チコガイドデモ` at `高楼出現[デモ]` (`RunawayTico.cpp:228-257,285-303`). The Time table then runs the 300-frame tower part, 120-frame post-demo part, fade-out, and fade-in. The Action table invokes Rosetta's demo callbacks and switches during the tower part and appears Rosetta in `高楼出現[デモ後]` (Action row 35).

Native Rosetta/RosettaDemoHeavensDoor sources are absent. The current reference RosettaDemoHeavensDoor1 constructor still has five commented action registrations, including preDemo/pstDemo/fadeOut/fadeIn. Those must be recovered reference-first before importing/advertising the endpoint. The placed collector's group-1 `SpinGetDemo` marker does not replace the actual executable group-0 TicoGuideDemo; the original caller selects TicoGuideDemo by name.

The immediate useful ordering is: close the observed main link, run ordinary Gateway to identify its first actual owner failure, establish real process/scene and child placement ownership, then activate the original collector/children. This advances the wake-up itself. Rosetta's reference registration recovery is a later concrete endpoint dependency, not a reason to claim the intro already works.
