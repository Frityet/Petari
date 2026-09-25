# Round 22: actual LiveActor ownership

Baseline `5cce8ba9447d11c149d4bc84dc3898bfca789461`. Eleven paths inspected/snapshotted before editing; exact before/after bytes and scoped patch accompany the manifest. No builds, tests or Git operations by this lane.

## Implementation

LiveActor owns its original raw exclusive pointers directly. The only added retained fields are its shared ModelManager, actual JKRHeap::Handle for sound, one owned LodCtrl, clipping holder/group back-references and idempotent retirement flag. No registry, pimpl or copied resource-state map remains. Original init methods construct replacements before publishing them and retire the prior child; model helper construction rolls back together if either animation helper fails.

The original constructor registers with AllLiveActorGroup/ClippingDirector. NameObj's actual unwind notification handles a constructor failure after group registration. ClippingActorHolder owns absence-aware removal across its four actual arrays and removes the same info from clipping groups. Both clipping holder destructors retire their NameObj identity before deleting arrays, clearing live actor borrows through the actual virtual notification. ClippingInfoGroup's existing identity-generation check uses NameObj directly.

HitSensorKeeper publishes empty storage during destruction, notifies actual NameObjs once per retiring sensor, then frees sensors. LiveActor forwards the notification to its current keeper, which clears taking/taken pointers and compacts contacts. This also handles a replacement keeper because the actor already points to the new keeper. MsgSharedGroup's pending-sender cancellation is implemented by lane A.

Model creation keeps the original async boundary free of a current-heap lock. Only subsequent synchronous J3D initialization and sound/animator construction use JKRHeap::CurrentHeapScope plus explicit Aurora ClientAllocationScope({true,true}). Sound replacement keeps the previous heap handle alive through deleting the old sound. MarioAnimator's retained deleter captures the actual heap handle; DrawBufferExecuter retains the actor's shared ModelManager directly.

Retirement preserves the previous dependency order: clipping info, effects/collision, runtime pointer/draw registrations, target, LOD/view binding, light, switch, rail, spine, binder, sound with heap, camera/animation helpers, shared model. The raw model pointer stays valid until original draw-buffer removal finishes because DrawBuffer::remove obtains its packets from that pointer; it is cleared before model destruction. The original destructor still handles shadows first and sensor storage last. Early resource release does not delete the actor or its original array allocation.

## Integration and limits

No new source build entries are required. Root deletes ActorRuntimeRegistry and JkrAllocationDomain, substitutes process/scheduler/test callers, and performs the integrated build/short smoke. Lane A supplies NameObj notifications; lane C supplies actual JKRHeap handles. The exact public actor APIs match next-actor-plan.md. This is native ownership consolidation, not additional audio output support or gameplay validation.
