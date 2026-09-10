# Actual SensorHitChecker owner closure

Read-only scope report before native activation. The complete reference source exists in decomp/src/Game/LiveActor/SensorHitChecker.cpp; the native header already matches, but native CPP is absent. No missing method recovery is needed beyond two proven defects in the existing reference source.

## Current substitute and concrete differences

src/runtime/SceneScheduler.cpp: execute_movement_category calls execute_sensor_hit_check unconditionally at original category5. That method collects scheduled, live, unsuspended actor sensors and performs all pairs. It cannot provide the actual SceneObj needed by original StopSceneStateControl. It also ignores original six-group pair selection and one-way Eye contacts, and accepts tangent equality (distance > radius² rejection versus original distance >= radius² rejection).

Native HitSensor.cpp removed original MR::initHitSensorGroup, group add/remove on host/system validity transitions, and reclassification on setType. Merely adding a SensorHitChecker NameObj would leave its groups empty and preserve the wrong evaluator.

Original checker owns groups Player16, Ride128, Eye512, Simple2048, MapObj1024, Character1024. It clears active contact counts, performs the exact 13 cross-group pair passes and one within-Character pass; actor callbacks still happen later in actual HitSensorInfo::doObjCol. Direct classification helpers and original MR::isClipped already have providers.

## Reference defects with retail evidence

- SensorGroup::clear currently assigns nullptr to u16 mSensorCount. Retail8016B690–B6C0 stores halfword0. Restore numeric0 in reference first.
- doObjColInSameGroup starts its inner loop at0. Retail8016BAC0 copies outer r26 to inner r25 and8016BAC4 seeds matching pointer offset; start at outer i (including self, rejected by same-host check) to test each actual pair once.
- checkAttack retail8016BBA0–BBC4 computes (x²+y²)+z², sums radii p1+p2, and rejects >= (unordered comparison follows original). Existing source uses equivalent finite sums but different operand order; retain exact operations if recovered alongside the two defects.

## Coherent activation needed

1. Import complete checker and restore full reference HitSensor source (header ABI unchanged).
2. Add actual scene factory case, stable CP932 original name and an early required checker before any HitSensor is constructed. Original SceneFunction::initForLiveActor already creates this owner; bounded scene fixtures need the same genuine prerequisite.
3. Remove SceneScheduler's native sensor evaluator/declaration and category5 unconditional hook; the original registered checker movement is authoritative and can be stopped/resumed normally by original scene control.
4. Before freeing actual sensors, ActorRuntimeRegistry keeper teardown must invoke original invalidateBySystem so active group arrays never retain released sensors. Preserve existing cross-actor contact removal during callback retirement.
5. A native lifetime owner records the actual six SensorGroup allocations and pointer arrays, retains them through original scene-object retirement, then releases them. Original empty checker destructor reflects whole-scene heap teardown; do not fabricate a replacement class or leak process metadata.

No native files were changed for this report. Parent owns approval of the coherent integration/build timing; no new tests or repeated verification are proposed.
