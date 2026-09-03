# Original scene connection helpers

The native port now uses all 65 original named scene connection helpers from Game/Util/ObjUtil.cpp. Each complete body is copied unchanged; 19 scattered native implementations were retired. Their declarations retain original NameObj/LiveActor parameter types. This covers collision actors, enemies, NPCs, map objects, planets, lights, layouts, cameras and special draw categories through the existing general MR::connectToScene service.

The general service already identifies real LiveActor and LayoutActor instances and routes their registrations to the scheduler. Thus the original layout helpers preserve layout ownership without separate per-helper native code. No stage or actor-name special case was introduced. Source-evidence.json records all body hashes and retired implementations.

The integrated real-disc File Select test passes after this replacement: original camera step60, six original planet models, six number layouts, authored lights, teardown and recreation. This does not claim the full game scheduler or Gateway jump loop is complete.
