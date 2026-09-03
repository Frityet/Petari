# Native scene metadata outside original allocation heaps

Entering real camera/collision constructors under a JKR allocation domain also routes ordinary native new allocations. Scheduler registration arrays, the SceneDrawBufferService wrapper, SceneObjHolderBinding services and its transaction bookkeeping must remain host-owned; otherwise scene heap reuse invalidates native state.

Registration methods now enter the existing JkrHostAllocationScope. DrawBufferService still explicitly re-enters its retained original domain when constructing original draw objects. SceneObjHolder::create enters the host scope only after the original factory and initWithoutIter complete, preserving the actual Game allocation scope. The holder's native services and descendant bookkeeping likewise use host allocation. No Game source or gameplay branch changes.

`smg-pc-scene-scheduler-heap-tests` creates an actual original heap/domain, constructs a real SceneObjHolderBinding and registers 128 host-owned NameObj instances while the Game domain is selected. The original heap's available capacity stays unchanged. After domain destruction the scheduler still executes and disconnects all entries successfully. This tests native metadata ownership; the full collision/camera object graph remains a separate integration gate.

The File Select far fixture also rebuilds and passes on the supplied disc after these changes, including the original camera's step60 transition and complete scene recreation. Registry host allocations and explicit pre-dispose actor-owner retirement are being handled by the collision-owner package; post-disposer JUT finalizers must not be reused to retire actor sound owners that are themselves JKRDisposers.
