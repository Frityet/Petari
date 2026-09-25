# Scene integration and retired contracts

OriginalSceneSupport now borrows the Scene's actual NameObjListExecutor and invokes its native initialization/retirement methods. No execution-binding object or global current-binding stack remains. Constructor failure retires original registrations/resources and detaches the actual executor. SceneFunction::allocateDrawBufferActorList and CategoryList::execute(DrawType) use their exact donor bodies; original category ownership retains callback storage through execution.

Deleted SceneDrawBufferService.cpp/.hpp, SceneExecutionBinding.cpp/.hpp and their two duplicated initial-table includes. Original tables remain in SceneNameObjListExecutor. SceneExecutionFixture uses the same actual executor; the existing scene-owner and callback-heap tests reference that owner directly.

Retired OriginalPreDrawSchedulerTests.cpp and its Xmake target. Its free-standing scheduler fixture did not create the original scene executor and explicitly required historical callback rollback. That obsolete sidecar contract is removed; the existing SceneSchedulerHeapTests still cover actual pre-draw callback dispatch. No replacement fixture framework was introduced and no standalone tests were run.
