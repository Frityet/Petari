# Delete ActorRuntimeRegistry through actual NameObj and LiveActor owners

Read-only implementation plan, 2026-09-25, current round21 source tree. No production edits, build, tests or Git changes were made for this audit. This refines round20/next-actor-owners.md against the actual current owners and consumers.

## Recommended bounded batch

Delete both ActorRuntimeRegistry files in one coordinated batch. The smallest useful parallel boundary is **NameObj lifetime/borrow notifications** versus **LiveActor resource ownership**, with a third integration lane doing scene/process/scheduler callers and existing fixtures. Do not move either unordered_map or either runtime-state struct into a new directory/class. Do not combine the work with JkrAllocationDomain removal, broader actor destructor recovery, or new test infrastructure.

Current direct include count is 69 files: 27 src (including the provider cpp) and 42 tests. There is also a provider friend declaration in NameObjGroup.hpp. Of these, 7 only include the header and use no remaining API: runtime/{RuntimeContext,RuntimeServices}.cpp and tests/{Model3DFor2DContract,NPCActorRealOrAbsent,AuroraNative,GameActorPhysicsRealOrAbsent,OriginalCollisionPartsOwner}Tests.cpp. Across actual API tokens there are 25 non-provider source files, including the NameObjGroup friend header, and 37 test files. The two maps and all their storage can disappear; source callers need no alternate registry.

## Contract to agree before parallel edits

Add these native ownership operations to the actual NameObj class. The spelling below is a concrete proposed shared contract; no compat aliases survive.

```cpp
struct NativeRegistrationMarker { u64 mNextGeneration; };
using NativeRegistrationFilter = bool (*)(const NameObj*, const void*) noexcept;
static u64 nativeGeneration(const NameObj*) noexcept;
static bool isNativeOwnershipClaimed(const NameObj*) noexcept;
void claimNativeOwnership(const void* owner);
void retireNativeLifetime() noexcept;
static NativeRegistrationMarker markNativeRegistrations() noexcept;
static std::vector<NameObj*> snapshotNativeObjects();
static std::vector<NameObj*> snapshotNativeObjectsSince(NativeRegistrationMarker);
static NameObj* newestNativeObjectSince(NativeRegistrationMarker,
                                      NativeRegistrationFilter = nullptr,
                                      const void* context = nullptr) noexcept;
static bool wasNativeRegisteredSince(const NameObj*, NativeRegistrationMarker) noexcept;
virtual void releaseNativeReference(const NameObj*) noexcept;
static void notifyNativeSensorRetirement(const HitSensor*) noexcept;
virtual void releaseNativeSensorReference(const HitSensor*) noexcept;
```

The two notification virtuals default to doing nothing on NameObj; real borrowers override them. These are ownership/borrow operations on existing classes, not a facade or new event service. A forward declaration suffices for HitSensor. Original movement virtuals and flag synchronization remain unchanged.

Add these public methods to actual LiveActor:

```cpp
void releaseNativeResources() noexcept;
std::shared_ptr<ModelManager> retainNativeModel() const;
void adoptNativeLodCtrl(std::unique_ptr<LodCtrl>);
void releaseNativeReference(const NameObj*) noexcept override;
void releaseNativeSensorReference(const HitSensor*) noexcept override;
```

Model construction, sound construction/domain selection, nerve, rail, binder, light and switch ownership stay inside existing original init methods. There is no reason to expose replacements for register_actor_runtime_state, adopt_actor_animation_helpers, adopt_actor_sound_object, or actor_scene_allocation_domain. The existing original APIs already express those operations.

## Lane A: NameObj identity and actual borrower invalidation

Own NameObj.hpp/cpp, NameObjGroup.hpp/cpp, LiveActorGroupArray.hpp/cpp, and the small overrides/declaration updates in TalkDirector, DemoDirector, DemoExecutor and BaseMatrixFollowTargetHolder. This lane can also own NameObjCategoryList's generation-query substitution. It does not edit LiveActor or the scene/process integration files.

Put generation, claim pointer and intrusive previous/next pointers directly on each NameObj, with private static head/tail/next-generation on NameObj. Insert during the NameObj constructor before NameObjRegister::add, preserving objects made before any holder is selected. If holder registration throws, unlink through the same idempotent retirement path. NameObjHolder still owns the original registration/lookup/placement arrays and mNativeHolder relationship; the lifetime list does not replace that behavior.

Generation queries receive possibly stale pointers. Search only currently live intrusive nodes and compare their addresses; **never dereference the supplied pointer first**. Address reuse gets a new monotonic nonzero generation. Snapshot methods preserve construction order with host-allocated vectors. Reverse suffix lookup starts at the actual tail and needs no allocation; this is used during constructor unwind. Claimed identities remain observable but cannot be adopted a second time. Reject a null owner, a retired object, or an already claimed object as today.

retireNativeLifetime must work both from the destructor and from the existing early scene/process prepasses. Those prepasses intentionally leave raw Game array elements allocated until heap retirement; replacing them with delete would be invalid. Retire marks/unlinks identity once, notifies the remaining live borrowers, and never frees the object. The ordinary NameObj destructor still disconnects actual scheduler execution and detaches its real NameObjHolder.

Notifications must permit nested destruction. Do not use a snapshot of raw pointers with no generation checks, and do not keep an unprotected next pointer over a borrower callback: DemoExecutor and BaseMatrixFollowTargetHolder already delete owned helper objects while releasing borrows. A small stack-local intrusive traversal cursor, repaired when a node unlinks, gives allocation-free mutation-safe traversal; cap traversal at its starting generation so newly created objects are not unexpectedly notified. This is internal iteration state on NameObj, not another object registry. Alternatively a generation-safe rescan is correct but avoid an O(n^3) whole-scene teardown from repeated linear rediscovery.

Promote the four existing exact-signature releaseNativeReference methods to overrides. Move only the current group array compaction into NameObjGroup::releaseNativeReference and LiveActorGroupArray::releaseNativeReference; remove NameObjGroup's compat friend. Group fields remain private and the actual group manages its own array. AllLiveActorGroup inherits the correct original group cleanup automatically.

For sensor notifications, MsgSharedGroup::releaseNativeSensorReference clears mMsg to -1, mSensor and mSensorName when its queued sender matches. This preserves cancellation between sendMsgToGroupMember and the later movement phase without a registry dynamic_cast policy.

## Lane B: LiveActor resources, sensors and clipping owners

Own LiveActor.hpp/cpp, HitSensorKeeper.hpp/cpp, ClippingActorHolder.hpp/cpp, ClippingGroupHolder.hpp/cpp, the three LodCtrl adoption call sites in LiveActorUtil.cpp, and the exact shared-model substitutions in DrawBufferExecuter.cpp and MarioAnimator.cpp. Coordinate LiveActorGroupArray with lane A; do not edit it independently.

Put only necessary native ownership fields on LiveActor: its shared ModelManager, retained sound allocation domain, owned LodCtrl, actual clipping holder/group back-references and an idempotent retirement flag. Existing original raw fields should themselves own their exclusive resources, using local unique_ptr for constructor rollback and exchange/delete for replacement. Do not duplicate every raw Game pointer with another private unique_ptr or embed the old LiveActorRuntimeState under a different name. The ModelManager shared owner is necessary because actual draw executers and MarioAnimator retain it beyond actor retirement; the raw mModelManager remains its original observation.

The original init methods take over their current registry helpers:

| Current helper | Actual owner operation |
| --- | --- |
| register_actor_runtime_state | LiveActor constructor registers with actual AllLiveActorGroup/ClippingDirector when MR::getSceneObjHolder exists; actual fields carry successful clipping borrows |
| replace_actor_spine / update_actor_nerve | initNerve owns/replaces mSpine; movement calls its existing update directly |
| replace_actor_rail_rider | initRailRider owns/replaces mRailRider |
| adopt_actor_stage_switch | initStageSwitch owns/replaces mStageSwitchCtrl |
| replace_actor_light_ctrl | initActorLightCtrl owns/replaces mActorLightCtrl |
| configure_actor_binder | initBinder owns/replaces mBinder, preserving effect-keeper rebinding |
| initialize_actor_model | initModelManagerWithAnm calls ModelManager::createNative with original caller heap and retains returned shared owner |
| adopt_actor_animation_helpers | same method owns mAnimKeeper/mCameraCtrl directly; constructor failure frees whichever helper succeeded |
| actor_scene_allocation_domain / adopt_actor_sound_object | initSound retains model domain when available, otherwise actual caller heap; create replacement before publishing and keep previous domain alive until old sound deletion |
| adopt_actor_lod_ctrl | three MR createLodCtrl functions transfer their existing unique_ptr to actor.adoptNativeLodCtrl |
| retain_actor_model | actor.retainNativeModel, used by actual DrawBufferExecuter and MarioAnimator |
| release_actor_runtime_state | actor.releaseNativeResources from existing destructor and prepasses |

Retain the existing ModelManager::createNative behavior: its original resource loading may wait for main-thread work, so do not place a persistent JkrAllocationScope/heap mutex around that call. Existing later synchronous matrix/animation initialization still uses its original JkrAllocationScope + J3DSys::CommandScope. Preserve ModelManager shared ownership and the no-model-replacement-before-draw-retirement guard.

Resource retirement must be idempotent and publish cleared raw fields before destroying children to tolerate reentrancy. Preserve existing order-sensitive edges: unregister clipping, release effect/collision ownership, unregister runtime StarPointer/model borrows and release the target, detach LodCtrl/view-group/light dependencies, destroy actor sound while its retained domain still lives, release animation helpers before the shared model. The original raw fields must not retain dangling observations after early retirement. The destructor still safely handles shadow-controller list and sensor keeper; do not turn releaseNativeResources into delete-this.

Clipping unregistration belongs on ClippingActorHolder, e.g. `unregisterNativeActor(LiveActor*, ClippingGroupHolder*) noexcept`: locate through findOrNone across the four actual lists, remove once, decrement _C, remove that info from each real clipping group, then delete the info. Never use the original find fallback when absence is legal: ClippingActorInfoList::find returns slot zero on a miss. Holder destructors call retireNativeLifetime before deleting their actual arrays so LiveActor::releaseNativeReference clears matching clipping back-references first; no global actor-map sweep is needed. Keep existing generation checks for ClippingInfoGroup's holder and ShadowController's holder, migrating them to NameObj::nativeGeneration.

HitSensorKeeper owns sensor-borrow cleanup. Add `releaseNativeReference(const HitSensor*) noexcept` to remove taking/taken and compact contact arrays. Before deleting its sensor infos, notify the actual live NameObjs once per retiring sensor; LiveActor's sensor override forwards to its current actual keeper, while MsgSharedGroup's override cancels its queued message. This covers a replacement keeper and attack-callback deletion, and avoids any global keeper/actor map. Preserve existing SensorGroup/HitSensor unregister behavior. The migration must not drop the old cross-keeper and queued-sender cleanup merely because the map disappears.

Check constructor failure after AllLiveActorGroup registration: NameObj's unwind notification removes the group borrow even if clipping registration fails. The current map insertion precedes clipping registration, so deleting that map is also an opportunity to eliminate its stale entry on constructor exceptions. Do not expand into unrelated derived actor destructor recovery.

## Lane C/root: callers, fixtures and final pair deletion

Own the two compat-file deletions and integration substitutions in:

- Game/Scene/{Scene,GameScene,SceneObjHolder,StageDataHolder}.cpp and app/OriginalGameApplication.cpp.
- runtime/{SceneScheduler,OriginalProcessTrace,RuntimeContext,RuntimeServices}.cpp.
- Game/{Demo/DemoStartRequestHolder,Effect/ParticleCalcExecutor,Effect/ParticleDrawExecutor,Player/GroupChecker,Map/CollisionDirector,Map/LightPointCtrl,LiveActor/ShadowController}.cpp.
- Existing fixture include/API substitutions; root/tests build files only if a wholly obsolete target actually needs retirement.

Map identity-presence checks to `NameObj::nativeGeneration(p) != 0`, generation comparisons directly to that API, and claims to `object->claimNativeOwnership(owner)`. Marks/suffixes use the agreed nested marker and NameObj static methods. In GameScene only the existing raw marker field needs extraction of mNextGeneration. Scene/process teardown retains its current order: release actual actor/layout resources, detach NameObjHolder, retire actual NameObj lifetime, then let original arrays/heaps retire. Do not replace bulk-array retirement with per-element delete.

Scheduler suspension uses `entry.name_obj == nullptr || (entry.name_obj->getFlag() & 1U) != 0`; the pending suspend/resume bits must still wait for original syncWithFlags. No new suspension API is needed. OriginalProcessTrace keeps its existing ID fields but obtains generation from NameObj. The two unused runtime includes simply go.

Fixture migration is mechanical, not new coverage. Preserve actual address-reuse/generation/borrow assertions. Replace name count comparisons by existing native snapshots' size (or omit only redundant sidecar-count clauses). `has_actor_runtime_state` and actor_runtime_state_count have no production callers, so delete these APIs without building a second actor identity counter. Post-teardown actor assertions use NameObj generation zero; remove redundant map-count clauses. PointLightRuntimeTests calls actual initActorLightCtrl; StarPointerRealOrAbsentTests calls actual releaseNativeResources. Remove the five unused test includes listed above. Keep authored actor, sensor, shadow, draw/model and process assertions; no replacement fixture framework or additional tests.

## Integration sequence and completion boundary

1. Land agreed NameObj/LiveActor declarations first so lane boundaries compile against a single API.
2. Implement lane A and B independently; lane A owns the common notification dispatcher and MsgSharedGroup override, lane B owns the LiveActor override and keeper cleanup.
3. Root substitutes callers while preserving the exact teardown order, deletes both compat files and removes all API/friend/include references in src/tests.
4. One integrated app build and the established short opening smoke remain sufficient for this bounded ownership change under the user's testing limit. No new suite or broad fixture run is proposed.

No donor decompilation or new runtime service is needed. The concrete risks are stale-pointer dereference during generation lookup, nested notification traversal, actor construction rollback, sensor sender/contact borrows, model retention after actor retirement, and sound destruction after heap retirement. Each has an existing actual owner and an explicit migration boundary above.
