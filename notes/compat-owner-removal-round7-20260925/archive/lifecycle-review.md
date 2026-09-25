# Independent CSV owner lifetime review

Read-only review of round 7 working-tree changes, including the new Light/Demo fields present during review. No build or source edits performed for this review.

## Actionable failure path

`LiveActor::initModelManagerWithAnm` assigns `mAnimKeeper`, then constructs `mCameraCtrl`, and only then calls `adopt_actor_animation_helpers`. If the second construction throws, the completed ActorAnimKeeper is not in ActorRuntimeRegistry's unique ownership and is not deleted during registry rollback. Its newly owned CSV parser therefore retains an archive token through attempted rollback. Sent to gateway_gap_audit for a narrow exception cleanup around the two helper constructions/adoption. A fix must avoid calling the existing adoption function twice with the same non-null unique_ptr-owned pointer.

## Reviewed normal paths

- `StageDataHolder`: constructor zeros every child pointer; destructor scans the complete array, including a partially initialized child whose count was not incremented. Claiming child ownership excludes it from SceneObjHolder's independent owned-object list. Child tables and separate object-name map release before their archives. AssignableArray destroys JMapInfo elements normally; each native disposer unregisters on explicit destruction. Root's stage object is retired with the scene before archive removal.
- `ScenarioData` and `ScenarioDataParser`: separately allocated galaxy name and two distinct JMap objects have one owner each. Constructor catches release already-created state; parser cleanup deletes every completed ScenarioData. FixedArray/Vector holds pointers and does not delete pointees again. Actual process explicitly destroys the parser before the FileLoader.
- `ParticleResourceHolder`: particles borrow group names, and particles retire before the auto-effect parser. ParticleNames and AutoEffectList are distinct JMap objects. JPAResourceManager explicitly unregisters its heap finalizer, preventing repeated manager destruction. JPC decoded backing is separately retained in the JPC registration until archive retirement; actual effect scene ownership retires before the process particle catalog.
- `ActorAnimKeeper` and `ActorPadAndCameraCtrl`: record arrays are deleted in each destructor, then their unique_ptr JMap fields release. ActorRuntimeRegistry destroys camera/animation helpers before its ModelManagerOwner, preserving the holder and its archive while the parsers detach. Constructor catch cleanup handles their own partially allocated arrays. The pre-adoption caller failure noted above remains separate.
- `LightDataHolder` / `LightZoneInfo`: existing destructors free record arrays; new unique_ptr parser fields then release. LightZoneDataHolder's array invokes each element destructor once. LightDirector already destroys these concrete holders.
- Current Demo keeper destructors are defaulted and release only newly owned parser fields. Existing DemoDirectorOwnership remains responsible for its manual Action/Camera/Time/SubPart/Player arrays, so those changes do not double-delete the arrays. Sound/Wipe's existing AssignableArray bases remain their array owners. DemoPadRumbler's new array destructor has a single new caller in MoviePlayingSequence cleanup; no competing manual delete was found.

## JMap cache and borrowed string findings

No `DeferredJMap` cycle exists. The registration owns DeferredJMap, whose cached decoded JMapInfo owns BcsvTable and DataCompat. The cached object is created by `from_bcsv`, so its source owner is the BcsvTable, not an attachment. Each external attachment owns a separate bundle containing DeferredJMap plus one archive token. The callback captures only a weak token.

`BckCtrl` uses a stack JMapInfo and stores returned name pointers. These remain valid because the archive registration retains the same decoded DataCompat string cache after the stack attachment dies. No raw-string rewrite is required. ResourceHolder releases BckCtrl before FileLoader retires the registration.

The review establishes normal cleanup ownership from current source. It does not claim successful runtime teardown; root is testing the latest owner batch and identifying any remaining live archive leases.
