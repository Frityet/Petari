# Original ClipArea holder and Binder filters — 2026-09-12

## Problem and change

The current port has real original CollisionParts, HitSensor ownership, and collision-query virtual filter dispatch. `MR::setBinderExceptSensorType` nevertheless still threw an old message claiming these owners were unavailable. This blocks the first Binder setup in original Coin initialization and any other caller of this shared filter API.

Imported the existing original `ClipAreaHolder.cpp`, its header, and the `ClipArea` interface header directly from decomp. The native SceneObj factory now constructs this original holder with its original CP932 name. The source owns its member list, activity flag and region queries; `ClipAreaCollisionFilter` retains the caller's live center pointer and radius and delegates to this holder. There are no stage, actor-name or placement special cases.

Restored the three original LiveActorUtil Binder filter setters in `OriginalBinderFilterUtil.cpp`: parts filter, except-actor filter and triangle filter. The original ClipArea utility now installs its actual filter on the original Binder. The obsolete throwing implementation was deleted.

The holder constructor's genuine draw dependency is now supplied: original `DrawUtil::setupShadowVolumeDraw`, copied into `OriginalShadowVolumeDraw.cpp`. The port's stale DrawUtil header was synchronized with the reference declarations to compile the original holder unchanged. `source-correspondence.json` verifies byte-identical full imports and exact copied helper bodies. No new decompilation or Game behavior modification was performed.

## Rendering and lifetime review

The original holder registers its pre-draw functor for `DrawType_ClipArea` through the existing scene executor. `NameObjCategoryList::execute` returns before calling a pre-draw functor when its category has no objects. Constructing this holder alone therefore does not alter GX state or issue a draw.

When actual clip actors are eventually added, the original callback configures untextured register-color TEV, disables indirect stages, enables clipping, and uses `GX_GEQUAL` depth with depth writes off. Original GameScene orders this category after ClippedMapParts. The bounded host scene's normal draw helper does not currently execute this clip category; concrete clip-shape rendering is not enabled by this checkpoint.

The existing original NameObjGroup destructor releases the holder member array. Existing scene pre-draw registration retains the functor's Game allocation domain and retires the functor when execution is cleared. Filters created under the Game allocation scope follow original scene-heap lifetime. Tests assert that holder/filter allocations belong to the actual scene arena and that the arena and object registrations retire across two cycles.

## Verification

The initial isolated native object batch compiled all seven affected production/test translation units successfully. Logs and `native-compile.json` retain this bounded compilation evidence. The original holder compiles with one existing Functor attribute warning; the collision fixture has existing JKR header override warnings.

Extended `GameActorPhysicsRealOrAbsentTests` to verify the installed original filter and borrowed center, replacing its obsolete expected throw.

Extended `OriginalCollisionPartsOwnerTests` against actual archived KCL parts to verify:

- With no ClipArea holder, even a clip-field sensor remains collidable, as original code requires.
- The native factory creates the actual active, empty holder on the Game heap.
- With a holder and no containing areas, a clip-field part is excluded through original MR line-query dispatch.
- The same filter preserves ordinary map collision and the original CollisionParts identity.
- Original holder activity toggles and scene teardown remain effective.

This fixture now uses the shared `SceneExecutionFixture`: the original holder's pre-draw registration correctly requires the actual initialized executor. Parent owns full Xmake builds and real-disc runtime execution; their outcome should be appended here after completion.

## Remaining limits

No concrete ClipArea shapes, clip actor factories, mirror actors, Coin factory activation or Gateway progression were added. Concrete shape source still has unrecovered/PowerPC-specific pieces. Coins retain separate mirror/shadow prerequisites. This checkpoint expands the common original holder/filter surface; it does not establish clipped rendering or completed coin gameplay.

## Fixture migration follow-up

Running the older test targets exposed stale fixture assumptions. The physics fixture used positive LiveActor appearance without a scene initialization owner, then expected a null original shadow-list pointer and mutations of historical shadow metadata. It now creates the actual runtime and shared scene execution fixture and checks the original ShadowController fields for collision/gravity mode, drop data and validity. The no-projection clipping check supplies its required real controller.

The collision fixture also incorrectly required individual rollback frees to restore free space inside the scene JKRSolidHeap. That heap intentionally ignores individual free calls; its entire arena is reclaimed at retirement. The fixture retains immediate original object/slot cleanup assertions and its two-cycle weak-domain expiration/object-registration checks at scene retirement. No production allocator behavior changed.

## Binder authority cleanup

A subsequent runtime regression found a real correctness defect: original `resetPosition` called original `Binder::clear`, but the compatibility ground/wall/roof queries still read a separately recorded native snapshot. They therefore reported old contacts after a reset until the next host integration tick.

Deleted `ActorBinderContactState` completely, including the registry member, recording/clearing/query APIs and the entire per-tick contact-copying pass. The renderer-facing player snapshot reads the original Binder query. All test references to the removed state now use original Binder fields and MR queries directly. Source search confirms no remaining references to the deleted contact-state API.

Restored original Binder-backed queries for grounded state, contact presence, original normal pointers, wall hit power, rebound, roof/ground crushing pressure and `offBind`. The latter keeps original contact flags until the original Binder next clears or updates them. Also restored both original `calcGravityOrZero` overloads: the grounded fallback copies the negated original normal rather than reading and renormalizing a native snapshot.

The pressure helper required one compile-only repair: wrap its local declarations in a scope so the original early `goto LABEL_FALSE` does not illegally skip C++ initializations. This was applied to decomp first, the port Game source and the native source slice. Calculations and control flow are unchanged. `binder-query-source-correspondence.json` verifies 13 exact reference bodies after that repair; `binder-native-compile.json` records all nine affected TUs compiling successfully.

Whole LiveActorUtil activation is still blocked by unrelated unimplemented owners such as MirrorActor and retained model/submodel paths. Original Binder itself is already compiled directly from Game; these utility slices remove alternative simulation state while respecting that present source closure boundary.

The reset regression now establishes an actual Binder wall contact, confirms the normal pointer is the original Triangle storage, calls resetPosition and verifies the contact disappears immediately without another integration tick. The collision fixture initializes the actual process scenario catalog before resolving stage resources, matching other original scene fixtures.

## Integrated runtime fixture audit

The focused targets now run against the actual initialized original scene executor and real-disc resources. Each failure was traced in LLDB before changing its fixture:

- Collision camera setup requires the original DemoDirector first, matching GameScene order. ResourceHolder calls require the exact archive filename: decomp ResourceHolderManager forwards to makeObjectArchiveFileName, and decomp FileUtil adds directory prefixes but no extension. The test now passes `HeavensDoorSmallPlanet.arc`.
- Collision arena retirement was already working. Its pre-session baseline incorrectly counted the pointer layouts later created by the deliberately longer-lived StageSessionBinding. The baseline now follows that binding, so both scene cycles still must retire all their own objects and the actual Game heap.
- Physics clipping assertions now respect the original ClippingActorInfo constructor default (far level6,100m), then verify explicit maximum distance uses the camera far plane and explicit100m takes effect. Shadow utility probes check original controller state, and omit an invalid null-name request for a multi-controller list (original lookup uses strcmp).
- The two native Binder fixtures now own the actual CollisionDirector and scene execution graph. The same-frame movement probe completes original scene initialization before scheduling.
- The capped-plane projected-retry probe used a70-unit vector precisely at a subdivision threshold. Original TVec3f::length returns69.9999924 there, and original Binder correctly selects2steps; LLDB records this in `native-kcl-step-count.log`. The fixture now uses80units and checks its three-step displacement, retaining its intended capacity/full-plane regression without assuming exact sqrt rounding at a boundary.

The original Binder reset/normal-pointer regression, same-frame derived actor probe, and gravity fallback probe passed during these integrated reruns. Final consolidated target outcomes are recorded separately in `focused-results.json`; intermediate backtraces remain as diagnosis evidence.

Further fixture corrections preserved original contracts: the second collision cycle now enumerates each actual CollisionParts prism rather than treating process-wide stable Triangle identities as per-service array indices. The model-less native scale tests provide an explicit base-matrix override, because original setBaseTRMtx requires a real model. The physics center assertion now uses epsilonEquals; TVec3f has no value `operator==`, so its implicit Vec-pointer conversion made the prior assertion compare object addresses.

## Final integrated results

All three targeted Xmake builds succeeded against the final shared source graph. Real-disc collision ownership passed both scene construction/retirement cycles, including actual ClipArea filtering of archived KCL and original shadow projection. Actor physics passed8/8 groups, including resetPosition immediately clearing original Binder contact flags and normals pointing at original Triangle storage. The Aurora native test executable passed each relevant selected group: KCL collision queries and Binder resolution, derived actor same-frame Binder ownership, and Coin math/gravity. `focused-results.json` contains the exact current exits and durations; corresponding `.build.log`/`.run.log` files are the final receipts.

The unfiltered Aurora-native suite was not certified: parent identified unrelated older ownerless WPad/GX fixture failures. Its generic name filter was used to validate this changed Binder scope without claiming unrelated suite success. These focused tests do not establish bunny chase or Rosalina gameplay completion.

Independent review by the process-services agent confirmed the collision prism enumeration correction: globally unique triangle IDs are not per-service indices; the existing parts/prism overload resolves exact owner identity; original getTriangleNum and registration both exclude the dummy prism consistently. Actual filtered line queries and shadow checks continue to establish enabled registration.
