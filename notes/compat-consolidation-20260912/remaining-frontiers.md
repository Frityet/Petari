# Prioritized remaining compatibility removals

Read-only audit of NPC/talk/message utility ownership, after the string/message and duplicate Power Star state cleanup. This map does not claim those larger systems are already active.

## 1. Replace the TalkRuntime owner cohort, then remove OriginalDemoUtil

Largest coherent NPC removal: `compat/TalkRuntime.cpp` (1099 lines), `TalkCompat.cpp` (390), their runtime header, and `OriginalDemoUtil.cpp` (currently 585). OriginalDemoUtil now differs from original Game DemoUtil only by the missing TalkDirector lookup and three owner queries: isSystemTalking, isNormalTalking, getTalkingActor. Their counterparts in TalkRuntime read its current native presentation. The fourth former omission, isPowerStarGetDemoActive, was an unwritten StageSession flag and is fixed in this checkpoint by restoring the original GameScene query.

The original three talk queries guard whether the scene slot exists, then call the actual `TalkDirector`. `SceneObjHolderCompat.cpp` currently fills SceneObj_TalkDirector with a NameObj-derived TalkRuntime, not the original LayoutActor-derived TalkDirector. Simply selecting original DemoUtil or casting that slot would be invalid. Editing Game DemoUtil to call TalkRuntime would preserve the duplicate owner under a different filename rather than remove it.

Required actual source/owner cohort:

- Import original `Game/NPC/TalkDirector.cpp`, `TalkBalloon.cpp`, `TalkState.cpp`, `TalkTextFormer.cpp`, `TalkMessageCtrl.cpp`, `TalkNodeCtrl.cpp`, and `TalkMessageInfo.cpp`; activate original `Game/Util/TalkUtil.cpp`. The original Director/State/Balloon sources alone total 1516 lines. Retire overlapping Game-named class methods currently defined inside TalkRuntime only when their complete source owners are active.
- Publish the original TalkDirector instance in the actual SceneObj slot. Its init allocates balloon/state holders, a 128-entry controller container and TalkPeekZ. Retain raw helper allocations in the scene arena and capture LayoutActor children through the normal scene lifetime system. Native controller ownership must not leave dangling NPCActor::mMsgCtrl references when actors retire.
- Restore recursive text tag stepping used by TalkTextFormer: initTagProcessorRecursive, nextStepTagProcessorRecursive and isEndStepTagProcessorRecursive. The restored page scanner removes one prerequisite, but current LayoutRuntime still lacks the original reveal/finished processor state. Current TalkRuntime presents a complete formatted message and ends a non-short talk on a released-then-triggered A input, rather than running original balloon/state/page progression.
- Bind the actual TalkPeekZ to DrawSyncManager/GX frame lifetime. Original code registers callback4/count1, snapshots projection/viewport, pushes a draw-sync breakpoint, peeks depth and inverse-projects. Existing Aurora GX and DrawSyncManager capabilities are useful foundations; copying a StarPointer-specific depth-owner wrapper is not equivalent to this owner.
- Activate original prep/term transitions together: Mario talk state, clipping restoration, cameras, programmable demo start/end and timekeep pause/resume. Current TalkRuntime explicitly rejects ordinary programmable demo type1; keep that honest until actual owner transitions exist. Manual start/end calls alone do not implement original prep/term.
- Resolve real player and progression dependencies. TalkSupportPlayerWatcher consumes Mario floor/water/control/speed state; TalkDirector branch initialization consumes actual event data. Its reference getBranchResult still uses raw object offset0x70, which must be expressed in native member/array layout before a 64-bit import can read it correctly. This is a real architecture issue, not a reason to replace branch values with constants.

Acceptance: original request/prep/talk/selection/next/term transitions, multi-page text reveal, timekeep pause balance, programmable demo ownership, camera/clipping restoration and repeated actor/scene retirement. Then compile complete Game DemoUtil and delete its 585-line copy and the three TalkRuntime query counterparts in one change. Owner absence should retain original false/null guards; owner presence must expose original state.

## 2. Retire the mixed NPCActorRuntimeCompat provider through whole utility owners

`compat/NPCActorRuntimeCompat.cpp` mixes native archive/parts ownership with general quaternion/vector math, matrix extraction, gravity, animation, NPC pose/float helpers and shadow dispatch. Its 319 lines are not a single NPC platform boundary.

The large removable mathematical group already has original definitions in `decomp/src/Game/Util/MathUtil.cpp` and `MtxUtil.cpp`: makeQuatRotateRadian/Degree, turnQuatYDirRad, makeAxisFrontUp, isSameDirection/isOppositeDirection, clampVecAngleDeg and extractMtxTrans. Prefer complete MathUtil/MtxUtil activation coordinated with the other existing GameMath/Original math providers. Do not add another OriginalNpcMath fragment.

Concrete semantic differences deserving priority: native makeQuatRotateRadian constructs trigonometric products and normalizes afterward; original delegates to the JGeometry Euler operation. Native clampVecAngleDeg invents an orthogonal axis for anti-parallel input and rotates it, while the original returns when its cross-axis normalizeOrZero test reports a degenerate axis. Native turnQuatAxis likewise contains manual normalization/angle tolerances. These affect arbitrary actors, not only rabbits. Validate against original scalar/JGeometry edge cases rather than preserving the current approximations as expected results.

Other definitions have distinct natural owners: calcGravity and setBckFrameAtRandom belong complete LiveActorUtil; initShadowFromCSV belongs the existing shared archive/ShadowController CSV boundary; createNPCGoods and float/pose helpers belong NPCUtil once recovered in decomp. Decomp NPCUtil still lacks the latter bodies, so recover them from retail before migrating. The current parts creation path must preserve actual PartsModel joint/heap lifetime, not silently replace absent archive/joint records.

Acceptance: activate the source cohorts, remove overlapping definitions together, then delete NPCActorRuntimeCompat entirely. Keep only actual native resource/ownership boundary code in the existing respective owner modules. Source-file motion without retiring duplicate behavior is not completion.

## 3. Finish original text processing before broader StringUtil callers activate

Message/String utility source duplication is removed this checkpoint. Remaining real functionality: StringUtil's variadic number-font tag writer, ReplaceTagFunction::ReplaceArgs and original reveal/recursive layout tag processing. Complete these at original MessageEditor/NW4R processor boundaries. Date/time/race strings now have their original utility entry points, but uncalled dependencies remain explicit link frontiers, not implemented behavior.

`compat/MessageUtilCompat.cpp` now contains only the two explicit UTF-16 APIs required by fixed-width RFL/banner storage and retained layout-message pointer identity. This is an actual native representation boundary. Removing that filename by globally redefining wchar_t, copying arbitrary host wchar bytes into RFL buffers, or reintroducing duplicate message tables would regress this cleanup. A later API organization change may move the boundary into NativeBmgResource/MessageHolderOwnership together with its clients, but the retained UTF-16 view itself must remain.

## Owner-dependent validation that remains open

The new exact Power Star query reaches real GameSceneBinding and actual nerve checks. StageSession cannot answer it anymore. Positive GameScene tests need its honest original constructor/destructor graph; this checkpoint tests missing-owner behavior in the existing StageInitializationResourceTests fixture. Ordinary Gateway compatibility tests or a native archive build do not demonstrate the bunny chase, Rosalina spawn, complete original talk progression or these future owners.
