# Round 9 image-effect ownership

Removed both ImageEffectOwnership files and both JutTextureConstruction files. Actual original classes now retire their native allocations; no replacement capture service, texture registry, or scene-id cleanup switch was introduced.

| Owner | Allocations reclaimed |
| --- | --- |
| ImageEffectResource | All nine lazily created shared JUTTextures |
| ImageEffectSystemHolder | ImageEffectResource; director remains a separately registered NameObj |
| ImageEffectDirector | Its five ImageEffectState objects |
| BloomEffect | Two Mtx arrays; every texture is borrowed |
| DepthOfFieldBlur | Private `_28` JUTTexture only; other texture pointers are borrowed |
| WaterAreaHolder | Five pointer arrays; entries and WaterCameraFilter are separate NameObjs |
| WaterCameraFilter | Its capture texture and archive-backed JUTTexture wrapper |

Constructor-local unique_ptr guards protect partial allocations in ImageEffectSystemHolder, ImageEffectDirector, BloomEffect and WaterAreaHolder. They release only once construction finishes. Lazy shared texture creation publishes immediately into the actual resource, preserving original reuse and allowing a later failed effect constructor to leave already-created shared textures with their real owner. DepthOfFieldBlur has no throwing operations after its private texture allocation. WaterCameraFilter initialization failure is reclaimed through the existing generic NameObj rollback and its new destructor.

The SceneObj factory now directly constructs the same original six classes and original CP932 names. Removed its image-effect allocation/capture/rollback/retirement plumbing and the field/forward declaration in SceneObjHolderRuntime. Generic registration order, rollback, scene allocation scope and draw callback handling are unchanged.

Removed only construction-capture hooks from runtime/jut/JutTexture.cpp. SDK heap finalizer registration/unregistration, allocation commit/rollback, GX texture destruction and JutTextureAllocation remain unchanged. Explicit actual-owner deletion unregisters those finalizers, preventing duplicate heap retirement.

JutTextureOwnershipTests no longer tests the retired synthetic capture/adoption mechanism. Its existing actual JKR freeAll/freeTail/domain, explicit deletion, borrowed storage, constructor failure, address conversion, dimensions and provider-retirement checks remain. No new cases or fixtures were added.

Every owned path was snapshotted before editing. `owned-manifest.json` records before/after hashes and `scoped.patch` captures only this subtask, preserving the preceding Ocean factory edit. No builds, tests, Git or xmake operations performed.
