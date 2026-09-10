# Original Mario construction-child ownership

The prior demo shutdown report showed the process LiveActorRuntimeState registry retaining model owners after scene teardown. Its late static destruction attempted to release a J3dModelSourceRegistration after that registry's mutex had already been destroyed. The corresponding saved stack is notes/layout-actor-teardown-20260910/runtime/showcase-mutex-crash.stack.json.

The showcase's GatewayMarioOwner previously retained only the MarioActor root in a unique_ptr. Its original init ran as a separate direct call outside AuthoredPlacementInstantiator and without that system's construction-child graph. MarioActor::~MarioActor deletes its Mario module but does not delete the raw NameObj descendants created by initDrawAndModel, initParts and other initialization helpers. Examples in the original source include DrawAdaptor instances and MarioParts (PartsModel/LiveActor), whose native model metadata otherwise remained registered.

GatewayMarioOwner now uses the existing generalized NameObjChildOwner capture around the original constructor and a new initialize wrapper around the unchanged original init. The same graph owns the root first and subsequent unclaimed children in construction order; reverse clear destroys children before their MarioActor parent. Existing SceneObj/service ownership claims are honored by NameObjChildOwner. Constructor exceptions and init exceptions use the existing capture rollback/adoption behavior. Player service and actual MarioHolder pointers are detached before destruction.

The owner retains the active scene JkrAllocationDomain. Each original allocating call enters that domain inside its capture lambda, then exits before the host ownership vectors adopt children. This also gives original non-NameObj Mario allocations the real scene arena lifetime instead of relying on ambient host allocation.

The PlayerActorBridge initializer additionally supplies the coordinated exact read_nerve_change_enabled callback, directly invoking original MarioActor::isEnableNerveChange. The audio/runtime agent owns the corresponding service query implementation.

## Validation

The actual current compile_commands.json Showcase command compiled the edited translation unit to an isolated ARM64 native object with return0. Exact command and source SHA are in native-proof.json, and the one-path checkpoint is source-manifest.json. No Game or resource-registry/mutex implementation was edited. No root Xmake lane was used by this change. Source was frozen and parent notified before the next demo build.

The parent rebuilt the showcase successfully with this graph. Its first-frame failure then unwound past the previous static model-registry mutex error. A second cleanup failure exposed a separate native effect metadata allocation boundary; this diagnosis is below. Live first-frame and clean-exit success are still pending further runtime verification.


## Native effect metadata lifetime and original Mario center

The current binary was run under LLDB with an OSPanic breakpoint, using the actual Korean RVZ and the bounded 120-frame Gateway command. `cleanup-panic-lldb.log` captures the exact secondary failure: RuntimeContext destruction → EffectService event vector → EffectEvent keeper copy → EffectKeeperRegistration string → global delete → JkrAllocationDomain.cpp:250 provenance panic. The referenced string was allocated in the already-retired original scene arena. The debugger process was killed after capture; no fixture or renderer remained running.

EffectService is a process-owned native service. Its metadata calls previously inherited the caller's original Game allocation scope. Native keeper and transform records, active emitters, history records, resource caches, and copied query results could consequently retain original scene memory beyond disposal. Allocating public entry points now enter the existing JkrHostAllocationScope before constructing any owned strings or collections. The scope also covers frame particle updates and native draw records. Private allocating helpers execute beneath these public entry scopes. Release functions remain ordinary provenance-aware deletion; no registry or mutex is made immortal and no invalid free is suppressed.

`JpcBillboardTests.cpp` now has a focused regression using the actual JKR root/scene heaps. It performs both identity-keyed and name-keyed registration, transform publication, emit/delete/re-emit/delete-all, and metadata copying under the original Game allocation scope with strings larger than the string optimization buffer. The scene heap must retain its exact free size, the following ordinary new must still select that original scene heap, and retirement must return the full scene allocation. The fixture then inspects and releases native records after the original root itself has retired. This verifies the lifetime that failed in the demo without requiring renderer or fabricated game resources.

The coordinated PlayerActorBridge center callback now returns `&MarioActor::_2A0`, the same actual storage used by original MR::getPlayerCenterPos. The service and MR bridge implementation is owned by the audio/runtime agent. It does not substitute the actor translation or a computed approximation.

The updated Showcase, RuntimeServices, and JpcBillboardTests each compiled to isolated native ARM64 objects with exit0 using the current project compiler command. Per-file commands/logs are saved here, and `effect-center-source-manifest.json` lists the coherent three-path hashes. The RuntimeServices whole-file hash includes the coordinated player query work from the sibling agent. The parent owns the global showcase rebuild and the `smg-pc-jpc-billboard-tests` runtime run; no runtime success is claimed yet for this effect change.
