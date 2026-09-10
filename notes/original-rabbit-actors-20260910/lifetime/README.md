# Original NPC helper lifetime and shared actor groups

The rabbit/Tico imports use existing actual scene allocations and NameObj graph capture. This checkpoint adds their missing complete original LiveActorGroupArray owner, restores original borrowed NameObj name semantics, and removes stale group references on native early retirement. It adds no rabbit gameplay state service and changes no rabbit/Tico Game bodies.

## Complete group owner

Imported exact original LiveActorGroupArray.cpp/header, including the actual MsgSharedGroup, 32-group array, placement group-ID lookup, named/default groups, actor registration, and scheduled message broadcast. Existing scene construction capture owns registered MsgSharedGroup children; their NameObjGroup destructor frees its member array. The group's JMapIdInfo is plain original arena storage, with no host cache to destroy.

The only reference correction lifts `char defaultName[32]` from the if block in entry into function scope so it remains alive through createGroup. The Wii retains that same stack buffer across the call. Full Wii TU compile passed; **entry matches100% of188 bytes**. See group-proof.json/group-objdiff.json. This is one bounded symbol proof, not a newly recovered full-file accuracy claim.

`OriginalLiveActorGroupUtil.cpp` publishes the exact original MR::joinToGroupArray/getGroupFromArray/countHideGroupMember/countShowGroupMember plus their actual counting helper bodies from LiveActorUtil.cpp. Counting excludes the queried actor, and no matching group returns0. joinToGroupArray preserves the original iterator/getGroupID test; it requires the already-created group owner. The prior unavailable-manager thrower was removed from GameRuntimeCompat. SceneObjHolderCompat now constructs the actual group array with the original CP932 scene name; root adds it to both required scene-owner lists, as original SceneFunction::createGameScene does.

## General borrowed-name correction

MsgSharedGroup passes its member mGroupName buffer to NameObj before filling it. Eagerly copying bytes in the native NameObj constructor therefore reads an uninitialized buffer and freezes an incorrect string. NameObj now stores the original pointer in construction and setName. ActorRuntimeRegistry registers only identity/ownership and never inspects constructor name bytes. SceneNameObjRegistry::add and original NameObjHolder::add only store pointers, so registration remains safe before the derived constructor completes.

Host-generated names remain explicit host ownership: retain_name_obj_host_name(NameObj*, shared_ptr<const string>) retains a preconstructed string without rewriting mName or other aliases. Root integrates this before/after the creator call in the host NameObj factory. Fixed SceneObj names already use static owned strings; SceneScheduler's LayoutDrawAdaptor already has a separate name-owning base initialized before NameObj and destroyed after it. Original Game-owned literals, inline buffers and heap names retain their original behavior.

## Retirement

The existing registry already removes retiring actor pointers from every actual NameObjGroup. It now also compacts actual LiveActorGroupArray::mGroups when a MsgSharedGroup retires, preserving survivor order and clearing vacated slots. Before destroying an actor's sensor keeper, it cancels an actual MsgSharedGroup's pending message when that message borrows one of the retiring sender sensors. No callback receives freed sensor storage; no parallel host membership table is introduced.

## Rabbit/Tico helper audit

- AuthoredPlacementInstantiator captures the entire original construction registration suffix. It adopts RunawayRabbit/RunawayTico, SpotMarkLight/PartsModel and FootPrint NameObjs, and retires children in reverse construction order. Original empty actor destructors do not create competing ownership.
- WalkerStateRunaway/WalkerStateBlowDamage and TicoDemoGetPower are original NerveExecutor helpers; their Spine/raw state contains no native C++ cache. Their ordinary scene-heap storage can be reclaimed with the arena after execution stops. This does not require a second host gameplay owner.
- TicoDemoGetPower's registered action functor belongs to the actual DemoActionKeeper; the existing DemoDirectorOwnership removes actor-associated callbacks on retirement. Talk controls use the current original Talk owner.
- SpotMarkLight uses the normal PartsModel model/resource owner. FixedPosition stores matrices and borrowed host data and needs no extra native allocation service.
- FootPrint's JUTTexture already registers a general JKR heap finalizer. Its native texture object is destroyed before the scene heap is reused; FootPrintInfo arrays are ordinary arena storage. The current finalizer remains the authoritative lifetime mechanism.

Initialization needs actual model/texture archives (TrickRabbit or TrickRabbitBaby, SpotMarkLight, RabbitFootprint.bti), scene message/demo/group owners, effect resources, sensor/binder/gravity support, and the original scene postpass. RunawayRabbitCollect creates its children from authored child rows, then its postpass broadcasts the original wait message. This audit does not claim that entire chase has run.

## Validation scope

Source hashes are in source-manifest.json. A native compile harness attempt had a command-substitution error and evaluated no source; retained as native-command-attempt.json for clarity. Parent requested the single combined production build instead of repeated isolated checks, so no retry, root Xmake, runtime sweep or commits were performed by this agent. Parent supplies combined build/runtime evidence.
