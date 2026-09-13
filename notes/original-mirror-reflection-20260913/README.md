# Original mirror-reflection dependency closure

The actual SimpleMapObj/MapObjActor link graph required the three original LiveActorUtil mirror helpers. Restored their exact bodies against the existing ModelManager/DisplayListMaker/texture owners. This exposed the genuinely missing MirrorCamera owner and mirror material setter; the latter is recovered separately by the parent task.

Completed canonical decomp/src/Game/LiveActor/MirrorCamera.cpp with the original constructor, vertex-format lookup and model-derived plane extraction, then copied the complete TU unchanged to native Game. The original camera initializes its two matrices, derives the plane from the first authored position/normal, reflects the actual camera view and updates the projection texture matrix only in MirrorArea. Model vertices already have native scalar byte order through J3dGeometryData's common SDK resource conversion.

Installed actual SceneObj_MirrorCamera construction in the existing scene factory and removed the obsolete duplicate MR::getMirrorCamera shim. The original MR getter now resolves the real scene-owned object. Native TPosition3 received the exact existing reference setTR helper required by its matrix construction. Existing CameraUtil already supplies getMirrorCameraViewMtx and getMirrorModelTexMtx.

changeModelDataTexAll was cleaned up in the reference first: its manually expanded 32-byte copy/rebase became the actual J3DTexture::setResTIMG operation. This retains a 100% retail instruction match and uses the existing native pointer-width-aware SDK boundary. Native J3D texture records and the screen image reside in the retained MEM1 resource range, preserving original signed relative image/palette offsets. The original per-material usage check and 0x04020000 current-difference flag remain unchanged.

OriginalMirrorReflectionUtil.cpp contains the four exact reference helper bodies while the larger LiveActorUtil TU remains excluded; no mirror-state substitute or alternate renderer was added.

## Necessary compile proof

Original Metrowerks compilation passed for the complete MirrorCamera and LiveActorUtil TUs. Recovered MirrorCamera constructor 99.64%, model-derived plane 97.66%, all other owner methods 99.65–100%. Mirror wrappers 99.4–100%; texture replacement 100%. Reports are retained beside the compiler receipts.

Four affected native TUs compile: MirrorCamera, OriginalMirrorReflectionUtil, SceneObjHolderCompat and CameraUtilCompat. No component tests or shared build were run in this lane. Peirce received the frozen-source signal for the next actual stage-loading build/run. source-manifest.json records the exact eight affected source/header files; the shared files contain unrelated existing changes and their index was left untouched.
