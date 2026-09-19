# Actor and collision merge decisions — 2026-09-19

Read `decomp/AGENT_DECOMP_GUIDE.md`. This is source reconciliation of the current local canonical branch with the fetched upstream merge, not a new retail-matching claim. Native Game sources were not copied from this merge and remain their separately validated snapshot. No build, GPU execution, staging or commit was performed by this lane.

The lane resolves 51 assigned conflicts plus `include/Game/LiveActor/MaterialCtrl.hpp`, where Git automatically retained a second full MirrorReflectionMtxSetter definition. The upstream class already defines addUpdatingTexMtxFromName inline with NO_INLINE; consequently both its duplicated old class and the old out-of-line method must not survive. All new upstream projection/material controllers remain.

Verified local recoveries are retained for Binder sweep, slide, margins, contact storage and moving reaction; CollisionParts point/sphere/line/area queries, equal-scale arithmetic and moving transform; CollisionArea hitCheck and its negative-shift sentinel guard; ShadowController projection/direction routines; shadow line/oval matrix arithmetic; and SensorHitChecker distance summation order. The upstream CollisionCategorizedKeeper implementation has the full corresponding query/membership routines and changes storage to original fixed arrays with descriptive fields. Its traversal/capacity/filter order remains the same at source review. CollisionParts' upstream `u32 createAreaPolygonList` declaration fixes the old local header's `void` declaration, while both source bodies already return u32.

WarpPod uses upstream descriptive field names and one manager class declaration in the same header. Retained local initPair/initDraw/drawCylinder bodies are token-renamed to those fields. Upstream initDraw/drawCylinder still contain uninitialized intermediate vectors; those incomplete bodies are not accepted as replacement for the locally recovered complete geometry. No new gameplay logic or path-specific workaround was introduced during merging.

Upstream includes complete previously absent DisplayListMaker, DynamicJointCtrl, view/projection material controllers, rail helpers and ChipCounter exe handlers. These are preserved. Upstream PlantGroup/StageEffectDataTable retain the same resource, sound and effect strings (string multiset differs only by includes). Their declarations, field naming and nerve/enum constants are reconciled together. Trivial sorting, clipping, view flags, J3D getters and shadow color spelling are accepted after checking the affected branches.

Local explicit HitInfo assignment, DynamicCollisionObj destructor, ShadowVolumeOval destructor and ChipCounter destructor remain where upstream has no owning definition. Upstream relocated SensorGroup methods and inline ShadowSurfaceDrawer/ShadowVolumeModel destructors are retained only once. Quaternion setEuler now has an upstream shared TVec template implementation, so AnmPlayer's old duplicate specialization is removed.

Validation is bounded source inspection: all 52 owned files have no conflict markers; duplicate class scan found only the Nerve macro's intentional placeholder class names after fixing MaterialCtrl; literal function-signature scan found no duplicates. The attached JSON records local/upstream/final hashes per file. The resolution script reads merge stages and never changes the Git index. Root owns canonical compile/integration and final merge. Existing published retail evidence applies to retained local bodies; newly adopted upstream methods require their own build/runtime checks before any PC parity claim.

## Per-file decisions

- `include/Game/LiveActor/MaterialCtrl.hpp`: Repair auto-merged duplicate MirrorReflectionMtxSetter class; upstream inline NO_INLINE method already owns addUpdatingTexMtxFromName. Keep the one complete upstream class and all newly added material controllers.
- `include/Game/LiveActor/DynamicJointCtrl.hpp`: Upstream field offset annotations, identical member types.
- `include/Game/LiveActor/ShadowSurfaceCircle.hpp`: Upstream radius offset annotation.
- `include/Game/LiveActor/ShadowSurfaceDrawer.hpp`: Use upstream inline empty destructor, remove old out-of-line duplicate with its cpp.
- `include/Game/LiveActor/ShadowVolumeDrawer.hpp`: Equivalent formatted empty virtual destructor.
- `include/Game/LiveActor/ShadowVolumeModel.hpp`: Use upstream declared destructor and its upstream body owner.
- `include/Game/LiveActor/ShadowVolumeOval.hpp`: Retain explicit recovered destructor declaration/body; adopt upstream member annotation.
- `include/Game/Map/CollisionParts.hpp`: Adopt upstream annotations and u32 area-list declaration matching both recovered bodies (old local header said void).
- `include/Game/MapObj/ChipCounter.hpp`: Upstream exe* declarations replace old direct Nerve executors.
- `include/Game/MapObj/MapPartsRailRotator.hpp`: Upstream enum naming, empty exeDone and member annotations align the full implementation.
- `include/Game/MapObj/MapPartsSeesaw1AxisRotator.hpp`: Use upstream field names, inline isMoving and angle-limit helper with equivalent original behavior.
- `include/Game/MapObj/MapPartsSeesaw2AxisRotator.hpp`: Use upstream field names, inline isMoving and ordinary exeStay instead of direct Nerve body.
- `include/Game/MapObj/PlantGroup.hpp`: Use upstream mHintIndex/mTouchType and explicit matrix constructor parameter; call sites pass nullptr.
- `include/Game/MapObj/StageEffectDataTable.hpp`: Upstream rearranges all existing public APIs, preserving complete table/helper interface.
- `include/Game/MapObj/WarpPod.hpp`: Use upstream field naming and single WarpPodMgr declaration (same header, now before WarpPod).
- `src/Game/Animation/AnmPlayer.cpp`: Upstream shared TVec quaternion header contains setEuler implementation; remove duplicate specialization here.
- `src/Game/AreaObj/CollisionArea.cpp`: Use upstream complete actor/AreaPolygon methods; retain verified hitCheck including negative-sentinel shift guard, plus local DynamicCollisionObj destructor.
- `src/Game/AreaObj/ImageEffectArea.cpp`: Equivalent selection-sort traversal with upstream local names/types.
- `src/Game/AreaObj/LightAreaHolder.cpp`: Equivalent selection-sort traversal and order; upstream direct array access.
- `src/Game/AreaObj/WarpCube.cpp`: Equivalent pairing null/self/ID checks and draw math, reconciled upstream locals.
- `src/Game/Boss/SkeletalFishBaby.cpp`: Upstream J3DJoint getter/include spelling, same joint-index test.
- `src/Game/Boss/SkeletalFishBoss.cpp`: Upstream J3DJoint getter/include spelling, same joint-index test.
- `src/Game/LiveActor/Binder.cpp`: Keep complete verified local bind/sweep/contact/reaction bodies and explicit HitInfo assignment; use upstream surrounding constructor/declarations/constants.
- `src/Game/LiveActor/ClippingActorInfo.cpp`: Equivalent bool test, swap-last erase and find locals.
- `src/Game/LiveActor/ClippingJudge.cpp`: Equivalent viewing-volume six-plane construction with upstream scalar and vector names.
- `src/Game/LiveActor/DisplayListMaker.cpp`: Adopt upstream complete display-list and material-difference methods, including previously missing flag methods.
- `src/Game/LiveActor/DynamicJointCtrl.cpp`: Adopt upstream complete node/controller methods; countdown decrement and branch are equivalent.
- `src/Game/LiveActor/EffectKeeper.cpp`: Upstream relocates floor-code and Binder helper definitions; retain one implementation rather than auto-merge duplicates; callback semantics unchanged.
- `src/Game/LiveActor/HitSensorInfo.cpp`: Equivalent original position/offset and optional host matrix handling with upstream names.
- `src/Game/LiveActor/LodCtrl.cpp`: Equivalent nested guard/LOD-distance decision tree; upstream constructor and methods remain complete.
- `src/Game/LiveActor/MaterialCtrl.cpp`: Adopt newly complete upstream view/projection/Mario-shadow material controllers and renamed mirror fields; addUpdatingTexMtxFromName is already owned by the upstream header, so no out-of-line duplicate.
- `src/Game/LiveActor/MirrorCamera.cpp`: Upstream retains complete S16/F32 resource parsing, reflected view/projection construction and lifecycle; reorganized helper placement.
- `src/Game/LiveActor/RailRider.cpp`: Upstream forwardGoal naming and original accessor; preserve new nearest-point and next-point methods.
- `src/Game/LiveActor/SensorHitChecker.cpp`: Upstream owns relocated SensorGroup once; retain locally verified checkAttack arithmetic grouping.
- `src/Game/LiveActor/ShadowController.cpp`: Use upstream complete controller and new setProjectionPtr; retain verified local direction/projection/length/gravity bodies, avoiding duplicated moved definitions.
- `src/Game/LiveActor/ShadowSurfaceCircle.cpp`: Equivalent original surface circle math and packed alpha 0x80.
- `src/Game/LiveActor/ShadowSurfaceDrawer.cpp`: Upstream Color8 spelling and inline destructor owner.
- `src/Game/LiveActor/ShadowVolumeCylinder.cpp`: Upstream named scale constant and new destructor, retain recovered matrix arithmetic.
- `src/Game/LiveActor/ShadowVolumeLine.cpp`: Retain verified local loadModelDrawMtx arithmetic; upstream remainder unchanged.
- `src/Game/LiveActor/ShadowVolumeOval.cpp`: Retain recovered matrix arithmetic and explicit destructor; upstream remainder and include ownership.
- `src/Game/LiveActor/ShadowVolumeOvalPole.cpp`: Retain recovered matrix arithmetic; preserve upstream named constant and matching helper.
- `src/Game/LiveActor/ShadowVolumeSphere.cpp`: Equivalent fully initialized scale matrix and projection position with upstream locals.
- `src/Game/LiveActor/ViewGroupCtrl.cpp`: Equivalent original group flags, last-match behavior, counts and LOD bindings with upstream variable names.
- `src/Game/Map/CollisionCategorizedKeeper.cpp`: Complete upstream keeper and zone methods preserve recovered encounter, filtering and bound checks; adapt to upstream fixed arrays and descriptive fields.
- `src/Game/Map/CollisionParts.cpp`: Retain verified local query/motion and equal-scale bodies within upstream surrounding lifecycle; adopt complete matching area return type and shared getScale template ownership.
- `src/Game/MapObj/ChipCounter.cpp`: Use complete upstream exe* methods/nerve macros, retain local empty destructor declaration implementation.
- `src/Game/MapObj/MapPartsRailRotator.cpp`: Equivalent wait/done/update behavior with upstream nerve macros, named message/axis constants and destructor owner.
- `src/Game/MapObj/MapPartsSeesaw1AxisRotator.cpp`: Equivalent complete torque/inertia/friction/hipdrop/limit behavior with upstream member names and inline helpers.
- `src/Game/MapObj/MapPartsSeesaw2AxisRotator.cpp`: Equivalent complete gravity/torque/restore and hipdrop behavior, upstream field names and ordinary exeStay.
- `src/Game/MapObj/PlantGroup.cpp`: Complete upstream group/member logic retains original placement, item, shake and clipping behavior; reconcile descriptive fields, constants and nerve macros.
- `src/Game/MapObj/StageEffectDataTable.cpp`: Complete upstream original camera/pad/sound tables and methods; use one table and consistent renamed fields.
- `src/Game/MapObj/WarpPod.cpp`: Retain complete local initPair/initDraw/drawCylinder (upstream path has uninitialized intermediate vectors), rename only member identifiers to upstream declarations.
