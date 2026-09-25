# Boss, Enemy and NPC import integration

Round17 first pass covers 319 newly imported source units in these three directories. Root owns the central donor imports and integrated build. This lane ran no compilation or tests.

- Deleted compat/OriginalTripodBossQuery.cpp. The complete imported Game/Boss/TripodBossAccesser.cpp supplies its accesser identity and MR queries; Game/Boss/TripodBoss.cpp supplies the original getJointMatrix body. The duplicate provider is no longer required.
- Changed getTripodBossGravityHostID's declaration and definition from u32 to uintptr_t. The imported donor narrowed an actual Accesser pointer; the already native GravityUtil/PlanetGravityManager accept pointer-width host identities. This preserves host exclusion semantics on 64-bit builds without truncation or an identity mapping hack.
- Restored nine original inline reaction/action helpers missing from the old NPCActor.hpp: four reaction-start predicates, four argument-taking setDefaults variants, and the two-argument setTalkAction overload. Existing zero-argument helpers stay intact; the later diagnostic repair restores the donor generic joint-controller pointer as described below.
- Adopted the donor NPCActorCaps::mUseShadow field name/type and updated the three existing native consumers (NPCActor, Tico, Rosetta). Newly imported Butler/TicoShop/TicoFat/HoneyQueen/etc already use this original declaration. No alternate field alias was retained.

Static first-pass inspection compared all Boss/Enemy/NPC native headers to current donor method names and scanned narrow pointer casts. RunawayRabbit's older field naming and combined init helpers remain consistent with its existing retained native implementation; no speculative rewrite. TalkBalloonInfo's out-of-line empty/update methods already supply the corresponding donor inline behavior. No shared core header or build file was changed.

Root centrally removed the SceneObj availability gates and enabled the complete original factory, including TripodBossAccesser. This lane did not edit the shared factory/header. No stubs were introduced.

The initial seven paths were recorded in owned-manifest.json and before/. TripodBossAccesser.cpp's before image is the root's fresh import, not a preexisting native file. Exact patches are under patches/. First-pass sources are frozen pending concrete integrated compiler errors.

## Concrete compiler diagnostic repairs

The first shared diagnostic pass was read from `../actors-compiler-errors.json`. This lane now records 27 owned paths. All implementation files are frozen for the root's second shared pass; this lane ran no build or test.

- Added actual defining headers for FixedPosition, FootPrint, Color8, NPC utilities (including TakeOutStar), LayoutActor utility overloads, and Mercator conversion overloads. No substitute declarations or local stubs.
- Restored the complete donor ModelObjNpc class and its constructor, destructor, init, control, and matrix-update bodies in Game/LiveActor/ModelObj.hpp/.cpp. The existing native ModelObj bodies remain unchanged. Its original createLodCtrlNPC call retains the existing native actor LOD adoption behavior.
- Removed the duplicate BossStinkBugFollowValidater class from the stale BossStinkBug header; its original dedicated header remains the sole definition. BossStinkBug.hpp now agrees with the donor.
- Restored NPCActor::mDelegator to the original JointController pointer. This admits the original Kinopio-specific delegator without unsafe casts.
- Replaced nullptr where donor source incorrectly used it for scalar bool/integer values (BallBeamer, OtaRock, HammerHeadPackun, BossKameckStateBattle, BegomanBase, Syati).
- Made the two mixed long/int random argument pairs use matching signed int operands; on the original 32-bit platform they selected the same integer distribution.
- Replaced removed C++ ptr_fun/not1 adapters with an equivalent noncapturing predicate in the two KoopaJrShip active-enemy searches.
- Removed the misplaced JKRArchive::getExpandedResSize body from KinopioAstro. Root adds that exact original body to the canonical SDK owner with its const declaration. Root also supplies the missing SDK RFLStoreData type.

Shared Functor includes, DrawType_None, MR::fabs, JPA/JUT fixes are owned by root; shared JGeometry/vector/quaternion methods are owned by decomp_validation. Their changes are not claimed as this lane's implementation. Source snapshots preserve any central import/Functor changes present before each edit. No xmake changes are needed beyond root's already enabled source glob and deletion of the old OriginalTripodBossQuery provider.

## Link closure repair

The full integrated build reached the linker; this lane used `../link-errors.json` and did not run a separate compiler or test. Source is frozen again after adding five paths (32 total).

- Added the 11 original NPCUtil definitions missing from the partial native file: initDefaultPose, turnPlayerToActor, vector setNPCActorPos, setNPCActorPose, setDefaultPose, convertPosOnGround, isActionContinuous, invalidateLodCtrl, tryTalkNearPlayerAndStartMoveTalkAction, tryTalkForceAtEndAndStartTalkAction, and tryChangeTalkActionRandom. Existing native utility bodies and CP932 literals are unchanged.
- Replaced the old partial DynamicJointCtrl source/header with the complete donor rate/node/controller/keeper implementation. Restored typed node/controller fields and the integer joint-count parameter; these had previously been unknown placeholders.
- The original dynamic-joint keeper's temporary JMapInfo parser is scoped with unique_ptr. Each DynamicJointCtrl copies its joint name into an owned byte array because native JMapInfo returns decoded cache strings. This permits parser destruction on success/unwind without dangling names or a leaked raw-archive borrow. The obsolete explicit template instantiation with a const-bool return was omitted because the canonical native JMapInfo template returns bool. Physics and joint callback bodies remain original.
- Added defaulted TripodBoss and SkeletalFishGuard destructors on the actual classes. The current donor headers declare these as virtual key functions, but neither donor source defines them. These are explicitly authorized missing-body definitions; they provide base/member destruction, not reconstructed original specialized cleanup.

No build graph changes are required: all modified canonical source files were already enabled. FileSelect/Mii are reserved to decomp_validation; root handles other utilities, SDK and disabled audio.
