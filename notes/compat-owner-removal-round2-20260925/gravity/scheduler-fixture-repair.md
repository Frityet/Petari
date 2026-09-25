# Scene scheduler category fixture repair

Read-only MemoryUtil/SystemUtil peer review found one concrete fixture dependency: `verify_category_execution` requested movement-off and invoked SceneNameObjMovementController::movement without a GameSystem singleton. Restored SystemUtil correctly synchronizes the actual GameSystemSceneController::mObjHolder, so the removed registry fallback could no longer hide the missing owner.

Updated only tests/SceneSchedulerHeapTests.cpp: include OriginalSceneControllerFixture, construct it inside the category test's existing heap-lifetime block, and give SceneExecutionFixture the actual original scene and NameObjHolder. The execution binding and callback objects retire before the original process fixture; its domain then retires before the existing root free-capacity assertion. This preserves the assertion and exercises the existing movement-off behavior through the original process holder. Other scheduler cases remain unchanged.

The test file was clean before this edit; its exact before bytes and status/hash metadata are saved alongside the gravity batch. Diff whitespace check passes. No builds, tests, config or Git index edits were performed.

The reviewed MemoryUtil methods otherwise preserve deleted provider behavior, including native byte/checksum handling, matching array deletion and heap/mutex selection. SystemUtil process calls retain original owner identity. Other omitted donor methods still have existing active providers; no missing current-call dependency was found.

## Stale clipping fixture compile repair

Root's rebuild then stopped on a pre-existing include of deleted ActorPhysicsRuntime.hpp. The same file still referenced its removed configure/update clipping functions. Removed the unavailable header and included the exact original clipping types. Actor-bearing scheduler fixtures now create the required real scene ClippingDirector before LiveActor construction. Its aggregate movement registration is explicitly disconnected because these tests control individual callback phases and do not construct a camera.

The clipping allocation-scope case uses the actual actor's ClippingActorInfo, actual scene ClippingJudge, and a local original ViewGroupCtrlDataEntry with test planes. It calls ClippingActorInfo::judgeClipping for the two transitions, exercising the original startClipped/endClipped dispatch. No retired compat helper was recreated. A second exact before snapshot is retained. Diff check passes; no compile or runtime command was run by this agent.

## Complete process ownership for all four scene bindings

Root's next rebuild passed compilation and then aborted at the main SceneExecutionFixture construction, before reaching category verification. Completed the same owner migration for every existing scene binding in this one test file: both explicit-callback scopes, category verification, and main now pass a real OriginalSceneControllerFixture scene and NameObjHolder. No bare SceneExecutionFixture remains.

Main constructs its process fixture after the deliberate first-ever NameObj registry-allocation check. At the end it retires its callback objects, execution binding, execution arena, and process fixture in that order before entering the three independent verification functions. Each scoped verification destroys its process fixture before the existing root heap-capacity assertion. A third exact before snapshot preserves this boundary.

This follow-up made no production or config changes. Diff whitespace check passes. Root owns the subsequent rebuild/run; no new runtime result is claimed here.
