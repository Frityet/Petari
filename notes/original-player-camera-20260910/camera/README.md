# Original CameraUtil activation — 2026-09-10

The native target now compiles the complete existing Game/Util/CameraUtil.cpp. Removed the cached CameraPose projection, unprojection, basis and scalar implementations from CameraUtilCompat, and removed all duplicate CameraUtil MR providers from OriginalCameraOwnerUtil plus the three single providers in CameraLocalUtilRuntime, SceneInitializationCompat and OriginalImageEffectUtil. The existing original CameraDirector/controller algorithms are unchanged.

## Why full activation

Initial direct compilation found a missing MirrorCamera header, absent native TVec4f, and an invalid null-to-enum cast. A temporary whole-TU probe showed that after resolving those compile boundaries, the only gameplay dependency absent from the current archive was MR::getMirrorCamera. This made complete activation practical. Root preferred actual Game source ownership over adding another copied utility slice. whole-tu-*-probe files record these preliminary compiler/dependency checks; whole-tu-existing-providers.json records the 86 existing strong CameraUtil duplicates across five compat objects before removal. Those preliminary archive observations are not final linked-provider proof.

## Original source and native boundaries

source-correspondence.json verifies the complete native CameraUtil source equals the resolved reference after precisely three documented changes: one debug-only native diagnostics include, one debug-only checked CameraContext lookup, and replacing `(GXProjectionType) nullptr` with `GX_PERSPECTIVE`. The last has the original numeric value 0; a function overload cannot make a prior ill-formed cast valid. Release CameraContext access retains the original precondition and all algorithms remain original.

Added the original Quaternion-backed JGeometry::TVec4 declaration/constructors/setters/conversions and TVec4f alias to the native SDK header. Native size/alignment/layout assertions verify its storage. MirrorCamera.hpp is copied exactly from reference. The temporary MR::getMirrorCamera provider checks for a real scene MirrorCamera and throws if that unsupported owner is absent; it never constructs a replacement camera or returns an invented matrix. The full MirrorCamera actor is still outside this activation.

Native animation resource retention remains in smgpc::compat::declare_event_camera_animation and CameraDirectorRuntime. It retains native CANM data before invoking the original MR declaration under the actual Game domain. Readiness now observes CameraParamChunkHolder::mIsSorted, the actual original completion state, instead of a separately published boolean. Host close_creating_chunks validates its original owner and delegates MR::completeCameraParameters; an original direct completion call therefore needs no extra host publication hook.

The existing GCapture query was initially classified incorrectly during cleanup. It belongs to decomp/src/Game/MapObj/GCapture.cpp, not PlayerUtil. That actor TU is not active, so its exact original isPlayerGCaptured body now remains in OriginalGCaptureQueries.cpp. Five actual PlayerUtil duplicate wrappers were removed as coordinated with the parent integration; this query was restored before final linking.

## Retail evidence

Full reference Wii compilation succeeds. reference-cohort-scores.json records function-level objdiff for the 25 context/projection methods. Twenty-two are 100%; normalized projection from view is 99.87342%, centered unprojection is 99.93827%, and screen-to-centered unprojection is 84.5098% (existing reference with constructor inlining FIXME). No new recovery or whole-TU 100% claim is made. These are existing reference methods activated natively.

Projection at 803C8594–803C86CC copies the actual CameraContext projection, divides by negative view Z, inverts Y and tests both normalized axes and projected Z. It accepts projected depth below -1; adding a near-plane rejection would change original semantics. Pixel conversion uses actual console width/height.

Unprojection at 803C879C–803C88DC reads original mFovy and screen height, uses height/2/tan(fovy/2) as focal length, chooses that focal distance for negative input distance, and multiplies the current inverse view. It intentionally ignores projection shake and permits a null centered-output pointer. Its ray result is not normalized. In widescreen, the original 832/456 pixel dimensions differ from the 16:9 perspective aspect; therefore generic strict projection/unprojection round-trip equality is not a valid retail expectation. The tests preserve and expose that original distinction.

## Verification and limits

activation-syntax.json records successful isolated native syntax checks for all seven modified production translation units. tests-and-gcapture-syntax.json records both focused tests and restored GCapture provider compiling. source-manifest.json records the frozen cohort hashes. No Xmake or commit/index operations were performed by this agent.

OriginalCameraContextTests now creates the actual scene CameraContext and asserts MR matrix-reference identity, current direct matrix edits without RuntimeContext pose publication, negative-Z projection, both screen overloads, axis/depth visibility, shake, console dimensions, view/basis/distance, J3D view loading, and original unprojection/ray conventions for both console aspect modes. CameraUtilRealOrAbsentTests checks the debug owner diagnostic and retains all caller-output preservation assertions; release intentionally retains the original owner preconditions. OriginalCameraDirectorTests retains its existing real CANM ownership and original completion/readiness coverage. StageStartCameraTests and ActorEventCameraTests were not edited because the parent owns that separate legacy-service migration.

Parent owns shared xmake activation, linked tests, actual movement/camera replay, final archive symbol proof and commits. Syntax or reference object correspondence does not establish runtime success.
