# Geometry merge repairs

The first full MWCC build exposed six geometry translation units with textual merge overlaps. The upstream merge had no Git conflicts, but it combined local recovered function bodies with upstream implementations and identifiers. Repairs follow the user's explicit preference for upstream when implementations overlap.

- `decomp/src/Game/AreaObj/CollisionArea.cpp`: restored upstream `hitCheck` and the rest of the upstream file, resolving the mixed `pCube/form`, `pPoint/pContact`, `localPoint/localContact`, and related names. Preserved the separate local `DynamicCollisionObj` destructor, which is still declared locally and has no other source provider.
- `decomp/src/Game/LiveActor/Binder.cpp`: restored upstream collision binding methods, resolving mixed `pPlanes/pPlane`, count, and hit-control names. Preserved the separate local `HitInfo::operator=` provider because the retained local header declares it. Kept its explicit `HitInfo.hpp` include.
- `decomp/src/Game/LiveActor/ShadowVolumeCylinder.cpp`: restored the complete upstream matrix method, resolving the mixed `controller/pController` names. Retained explicit ShadowController and JMath includes.
- `decomp/src/Game/LiveActor/ShadowVolumeOvalPole.cpp`: restored the complete upstream matrix method, resolving mixed `size/scale` and `mtx/drawMtx` names and missing matrix declarations. Retained explicit ShadowController and JMath includes.
- `decomp/src/Game/Map/KCollision.cpp`: restored the upstream file. Several sphere traversal and sphere hit methods contained incompatible local/upstream variable sets and label/control-flow fragments. Upstream already contains all the corresponding methods.
- `decomp/src/Game/MapObj/MapPartsRailGuideDrawer.cpp`: restored the upstream file, removing the appended duplicate local `exeDrawForward` body.

Validation: all six repaired translation units compile successfully using the configured Wii MWCC commands. Commands were obtained with read-only `ninja -t commands`, stripped of dependency-file generation, and run with isolated outputs under `geometry-compile-first/`. `geometry-compile-first/results.json` records exact commands, exit codes, source hashes, and timings; individual compiler logs are beside the isolated objects. This is translation-unit compilation evidence only; the primary agent owns full build/link validation.

No Git staging or commits were performed by this repair lane.
