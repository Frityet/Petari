# Real LayoutActor retirement regression

This bounded follow-up adds a real-disc regression to `OriginalSceneWipeOwnerTests.cpp`. It creates actual LayoutActor, LayoutManager and original Spine instances, retaining SysInfoWindowMini BRLYT records plus root and SaveIconPosition pane controllers. Two generations each cover explicitly scheduled and never-scheduled actors. It checks one actual nerve step, removes the actor while the executor is alive, checks all layout/manager/pane identity counts plus NameObj and scheduler membership return to their baselines, and checks no subsequent movement invokes the retired nerve.

The Spine is created through unchanged LayoutActor::initNerve while the original CurrentHeapRestorer selects the actual reclaiming JKR root heap. The assertion that root free space returns immediately after actor destruction observes the individual Spine deletion before the scene solid heap is disposed. Each complete scene teardown must then restore total root free space and all original and native metadata counts. No synthetic layout resource or renderer is used.

Production additions in this follow-up are only debug actor/manager/pane metadata counts in LayoutHost.hpp and LayoutManagerCompat.cpp. The separate parent-owned LayoutActor destructor now releases its native manager record and original Spine. This test exercises that destructor and the parent-owned scheduler removal change which avoids calling layout release recursively.

The authored-placement fixture also lacked real execution ownership when constructing LensFlare children and SwitchWatcherHolder. Those three cases now use the existing SceneExecutionFixture with a real scheduler, original scene executor, requirement holder and JKR domain. One accessor exposes the fixture's real SceneObjHolder for the existing identity assertions. Other synthetic lifecycle cases retain their original owner-free setup. Existing initialization/postpass edits in AuthoredPlacementInstantiatorTests.cpp predate this follow-up and are preserved.

## Validation

All three changed translation units compiled directly to native ARM64 debug objects with the current production include closure: LayoutManagerCompat.cpp, OriginalSceneWipeOwnerTests.cpp and AuthoredPlacementInstantiatorTests.cpp. Commands, return codes and source hashes are in native-proof.json; compiler output is retained separately. Diff whitespace checks passed.

Runtime is pending the parent's shared build lane. No Xmake command was run by this follow-up and no runtime or gameplay success is claimed. Intended runtime fixtures are the original scene wipe owner and authored placement instantiator targets, with SMGPC_REAL_DISC selecting the local actual RVZ. Parent is concurrently rebuilding the camera/jump demo and repairing the CaptureScreenActor scene construction boundary.
