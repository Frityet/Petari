# Reached mirror-actor creation during stage loading

The actual Coin initialization path reached MR::tryCreateMirrorActor, whose compatibility implementation unconditionally threw. The original function first queries MirrorArea at the host actor's position and returns nullptr when outside that area. MirrorAreaCube already has its original AreaObj/Cube2 placement owner, original area-manager capacity and real spatial lookup, so no replacement area system was needed.

Removed the unconditional throw and restored the complete original creation path beside the existing common submodel-name allocator in PlanetMapRuntimeCompat.cpp. The helper retains the original CP932 mirror-model suffix and constructs/initializes the actual MirrorActor only for contained hosts. Coin and every other caller now share this same original behavior.

Imported the complete reference MirrorActor source/header unchanged. Its real model, joint-animation copy, shared material animation, bounds, clipping/visibility and mirror-plane-side decisions remain original. MirrorCamera already supplies the real reflection-plane owner. MirrorActor registers normally through LiveActor/NameObj and uses the existing scene heap/holder lifecycle; this change adds no competing retirement owner.

Removed the unused matching-only MirrorActor_FORCE_MATCH_SDATA2 dummy from the reference before copying. All actual methods are unchanged. Original Metrowerks compilation and retail comparison pass: eight methods plus vtable have weighted match 99.94%, each 99.79–100%. The helper's control flow was compared directly with LiveActorUtil.s under its exact tryCreateMirrorActor symbol.

Native compilation passes for MirrorActor.cpp, PlanetMapRuntimeCompat.cpp and GameActorPhysicsCompat.cpp. No tests/shared build were run here. Peirce was notified to run the next actual stage-loading build after the independent rollback fix. Files/hashes are in source-manifest.json; compile and comparison receipts are retained alongside this note.

Thirty-first production build passed. Twenty-second actual stage run passed Coin creation and reached the SphereAir model resource load. That next failure is the resource layer rereading an already mounted archive while the model command scope has disabled scheduling. This run establishes construction progress; it does not exercise rollback again or establish completed stage loading.
