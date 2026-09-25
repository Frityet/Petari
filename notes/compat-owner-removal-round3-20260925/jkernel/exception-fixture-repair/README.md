# Exception ownership fixture correction

The test still expected `Camera state is unavailable.`, a message from the deleted CameraUtilCompat producer. Commit `60df3871d` switched MR::getCamPos to the original CameraUtil body with a debug-only OriginalGameDiagnostics guard, whose actual message names the missing SceneObj_CameraContext. This source was unchanged by the heap consolidation. Root's first run passed all nine standard exception types, nested allocation routing, message-view copying, copied exceptions, and retained rethrow before failing only that stale exact-message assertion.

The test now uses OriginalSceneControllerFixture and SceneExecutionFixture, verifies that MR and the original GameSystemSceneController return the same actual holder with no CameraContext, and checks the current exact debug error after both original owners and their heap retire. The retained payload host-ownership, exception type, weak-domain expiry and root free-size assertions remain. NDEBUG builds skip this invalid-precondition probe because original CameraUtil only emits the diagnostic in debug builds; the general Aurora exception cases still run.

Only tests/JkrExceptionOwnershipTests.cpp changed; it was clean at capture. No production edits, builds, staging, or commits. Before snapshot and exact patch are adjacent. Root runs the target.
