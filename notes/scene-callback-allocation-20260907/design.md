# Scene callback allocation boundary — source review, 2026-09-07

SceneScheduler calls original Game methods without selecting a JKR allocation
domain. Global `new` therefore uses host allocation unless its caller happened
to establish a scope. Initial effect-keeper construction explicitly enters its
EffectSystemOwnership domain, but a later original EffectKeeper::addEffect can
allocate AutoEffectInfo during movement outside that scope. Repeated assignment
can orphan its pointer. Scene heap reclamation can recover such trivial records;
host allocation cannot recover an untracked lost pointer.

## Smallest reusable dispatch change

Retain an explicitly supplied `shared_ptr<JkrAllocationDomain>` as the
scheduler's scene execution domain. Use the existing JkrAllocationScope around
the calls into original code, and leave native sorting, trace capture, cache
mutation, and temporary containers under host allocation. An internal invocation
helper or small scope wrapper can centralize the domain requirement. It must
retain a local copy of the domain for the duration of a callback, so unregister
or teardown requests cannot release the current heap while its stack is active.
Existing nested JkrHostAllocationScope and JkrAllocationScope already support
native escapes and explicitly owned SDK subdomains.

Cover all the dispatch surfaces, rather than only LiveActor::movement:

- Movement and calcAnim switches in SceneScheduler.cpp, including NameObj,
  LayoutActor, and original LiveActor callbacks.
- Clipping helpers: ActorPhysicsRuntime::update_actor_clipping calls the virtual
  startClipped/endClipped methods.
- Sensor updates and attackSensor delivery, plus receiveMessage broadcasts.
- Original DrawBufferHolder entry/draw calls and draw-category execution,
  including the original pre-draw functor. SceneDrawBufferService already keeps
  its callback object and allocation-domain lease alive during replacement.

Do not select an effect-specific domain from the scheduler. The current heap is
an execution property shared by all Game code. Explicit owners may still select
their own heap for resource construction. No EffectKeeper or Game method needs
to be rewritten for the routing fix.

## Ownership boundary that must be chosen explicitly

RuntimeContext::begin_scene_draw_buffer_registration currently supplies
ResourceHolderService::allocation_domain. RuntimeContext constructs that cohort
once and ResourceHolderService has no per-stage reset. Retaining it for callback
execution is the smallest wiring change, and prevents untracked host allocations,
but memory would be reclaimed on RuntimeContext destruction. It does not prove
per-scene reclamation.

For actual per-scene retirement, a generic scene owner should create and retain a
separate JkrAllocationDomain and supply it to SceneScheduler before callbacks can
run. SceneObjHolderBinding is an existing scope owner for the original scene
helper graph; alternatively RuntimeContext's scene-registration lifetime can own
an explicit execution-domain binding. Retire scheduler registrations, actors and
their typed native owners, then original helper roots and SDK finalizers, before
the final execution-domain release. A callback retains its own lease while it
executes. The existing EffectSystemOwnership heap should continue to be treated
as its explicit SDK/system allocation owner, not repurposed as the general scene
heap.

JKRSolidHeap intentionally reclaims its allocations as a unit. This fixes
untracked host leaks at scene retirement, but it does not make repeated original
allocations within one scene reclaim individually. The scene byte budget remains
real and exhaustion must be reported rather than silently moving allocations to
the host. Typed owners are still required for objects with external backing or
destructor work; an allocation scope does not replace them.

## Relevant existing iteration hazard

sorted_entries_for_movement and sorted_entries_for_calc_anim return Entry pointers
into the scheduler's std::vector. A callback that connects or removes objects can
invalidate these pointers; the current code dereferences its entry again for
post-callback player synchronization and debug trace. Later loop entries can also
be invalid. Draw-type execution already copies its batch and re-resolves current
registrations after callbacks. General dynamic actor creation should use stable
registration identities and revalidation for movement/animation too. This issue
is independent of allocation routing and is not introduced by the scope change.

## Focused validation

Use original NameObj/LiveActor test subclasses which allocate from movement,
calcAnim, clipping, sensor, message, and draw/pre-draw callbacks. Assert that
JKRHeap::findFromRoot points to the supplied scene domain and that the caller's
previous current heap/routing is restored after normal return, nested dispatch,
and exceptions. Check the host registry backing stays outside the scene arena,
then release all registrations and typed owners and prove root free-space
balance. Include callback registration mutation before claiming general dynamic
actor creation is safe. A dynamic original effect addition is a useful secondary
integration check; the primary test should be independent of effects or any
particular actor/stage.

No dispatcher, owner, or Game source was changed during this review.
