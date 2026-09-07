# Original actor sensor ownership and Mario setup

The PC actor registry now owns an actual original `HitSensorKeeper`, its `HitSensorInfo` entries, sensor objects and contact arrays. It publishes `LiveActor::mSensorKeeper` and retires the complete cohort before the Game arena is released. Registry buckets stay on the host; original keeper construction remains in the selected Game allocation domain. Retiring an actor removes borrowed contacts from surviving actors before storage is freed.

`HitSensorKeeper.cpp` and the new `HitSensorInfo.hpp/.cpp` are copied from the reference. The missing `HitSensorInfo::update` was recovered from retail: callback registration publishes the info and immediately invokes the original virtual callback; subsequent updates preserve the callback result. Actor-relative and external-position offsets use the actor base matrix's linear part when present, while explicit matrix bindings use that matrix's translation. Scalar multiply/add ordering is preserved with `-ffp-contract=off` on the imported info TU.

Native `HitSensor` previously omitted initialization of its contact count, pointers and validity flags after header restoration. Its constructor now uses the original complete initializer list. The existing native contact backend remains in place; this checkpoint does **not** activate the original six-group `SensorHitChecker` owner or claim its group encounter/filter ordering is closed.

The obsolete host binding map has been removed. Position/matrix/joint registration delegates to real keeper entries, with existing absent-joint/matrix rejection at the native resource boundary. Joint matrices are captured at registration as in the original. Original callback utility exports, taking lookup, offset assignment and apart-link mutation are restored from `ActorSensorUtil.cpp` in the existing utility provider. Ordinary host validity and actor lifecycle system validity are separate: appearance/unclipping never revalidates a sensor explicitly invalidated by the actor.

The scheduler now checks contacts at the original SensorHitChecker category, using positions published by previous actor updates. It only publishes contact arrays. The original keeper delivers those contacts from `LiveActor::movement`, before nerve/control, and updates sensor positions once after control/camera/light. This removes direct scheduler attack delivery and redundant callback updates. The scheduler allocation regression fixture now calls its original `LiveActor::movement` base, so sensor delivery follows the actual virtual boundary.

## Reference evidence

`reference-compile-results.json` records all five complete Wii compilation commands, source hashes and successful exits. Fresh RMGK01 results:

- HitSensorKeeper: all 16 functions 100%.
- HitSensorInfo: constructor and contact dispatch 100%; newly recovered 604-byte update 99.7351%.
- MarioMessenger: constructor, movement, enqueue and destructor 100%.
- MarioActorMorph: `initMorphStringTable` and its complete 80-byte/two-row table 100%.
- MarioActorSensor: whole verified historical recovery restored from `ba6cad1ff`, including setup 99.39759%, sensor callback 96.65714%, attack/send/reset/initForJump 100%. Detailed functions remain in the saved objdiff report; this is functional recovery, not an exact assembly claim.

`hit-sensor-info-update-retail.asm` is the retail disassembly. `sensor-restoration-provenance.json` identifies the pre-flatten source. Numeric `_468` comparisons are corrected to numeric zero in both trees after truthful union typing; native `startPadVib(0U)` disambiguates the LP64 integral overload. Native `_9D4` already uses the true HitSensor pointer; reference declaration was corrected to match its observed load/dereference.

## Validation and remaining frontier

All 12 affected native source/test TUs pass isolated LLVM23 syntax in `native-syntax-results.json`; full native target/runtime validation is pending the parent's coordinated lane. Existing `smg-pc-actor-sensor-real-or-absent-tests` now adds actual keeper allocation provenance/reclamation, immediate callbacks, original matrix/null-matrix branches, single-sensor lookup, host/system validity, contact delivery before control, retirement, and real 32-entry MarioMessenger FIFO/category execution. `smg-pc-scene-scheduler-heap-tests` retains callback Game allocation and actor retirement coverage with the original base movement call.

The original Mario setup must still run with its full init2 model/animation/StarPointer owners. Full Sensor virtual activation reaches original Msg/DefensiveMsg/Item owners; those are the next bounded provider closure. Neither syntax results nor generic keeper tests establish working Mario jumping or Gateway gameplay.
