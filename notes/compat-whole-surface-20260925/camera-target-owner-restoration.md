# CameraTargetObj owner restoration

Restored `src/Game/Camera/CameraTargetObj.cpp` byte-for-byte from current decomp `1a126cb5da311fedff662f53fb31c5aeaf851408`. No native adaptation was necessary: previously split actor/player providers already used the same types and calls. Complete donor also supplies CameraTargetDemoActor constructor/init/setTargetMtx, which previously had declarations but no provider.

Deleted `src/compat/OriginalCameraTargetActor.cpp` and `src/compat/OriginalCameraTargetPlayer.cpp`; removed only CameraTargetObj constructor/comment fragment from `src/compat/CameraLocalUtilRuntime.cpp`. Left all other local utility behavior for its coherent owner batch. No header or xmake change. Existing Game `**.cpp` wildcard selects the canonical owner; no exclusions need adding/removing for this batch.

Read-only verification: canonical source bytes equal donor; all39 CameraTargetObj/Actor/Player/DemoActor out-of-line definitions have one provider in src; no explicit old-source entries were found in tests/xmake.lua. Build/link and focused camera tests are delegated to root, not executed by this agent.

Recommended validation: native game link followed by OriginalCameraContextTests, OriginalCameraHolderTests, and ActorEventCameraTests original object/actor/player target phases. Check target position/orientation/gravity/grounding, player movement timer and demo suppression; no whole Gateway gameplay claim follows from this source restoration.
