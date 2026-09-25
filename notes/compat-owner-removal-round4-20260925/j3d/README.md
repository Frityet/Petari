# Round 4: complete J3D joint, model and math owners

Nine complete canonical JSystem translation units replace fourteen compat providers. This restores their actual original ownership and removes duplicate shards without changing Game code, the native model-resource lifetime owner, or Aurora device APIs. Complete original donor code is the base; documented native boundaries remain explicit.

## Scope

| Canonical owner | Removed providers |
| --- | --- |
| J3DGraphAnimator/J3DJoint.cpp | J3DJointCompat, J3DJointEntryCompat |
| J3DGraphAnimator/J3DJointTree.cpp | J3DJointTreeCompat, J3DHierarchyCompat |
| J3DGraphAnimator/J3DModelData.cpp | J3DModelDataCompat |
| J3DGraphAnimator/J3DModel.cpp | J3DModelCompat |
| J3DGraphAnimator/J3DSkinDeform.cpp | J3DSkinDeformCompat |
| J3DGraphAnimator/J3DCluster.cpp | J3DClusterCompat |
| J3DGraphBase/J3DTransform.cpp | J3DTransformMtxCompat, J3DTextureMtxCompat; transform constants and translation/rotation helpers from J3DJointCompat |
| J3DGraphBase/J3DSys.cpp | J3DSysCompat, J3DDrawInitCompat; traversal globals from J3DJointTreeCompat |
| JMath/JMath.cpp | JMathVectorCompat, JMathQuaternionCompat; matrix scaling from J3DJointCompat |

The canonical JMath header additionally restores the original JMAVECLerp declaration and holds fastReciprocal inline as the original header does, retaining the native ppc_fres estimate. No full-pointer native layout, archive endian conversion, resource borrowing or allocation-domain ownership was altered.

## Native correctness retained

- J3DJoint and complete J3DJointTree keep original matrix-calculator selection, callback ordering, nullable-calculator quirk, child/sibling traversal and hierarchy material/shape attachment. The original transform initializers and local variable forms supersede equivalent older fragments.
- J3DModel retains every original method. Its pre-existing native createMatPacket error checks remain byte-for-byte at function level, so allocation/display-list failures continue propagating instead of being discarded by donor omissions. calcNrmMtx remains the identical inline body already in the native header. The original empty model destructor is now beside its actual model owner; the separate, currently unbuilt Game/Player/J3DModelX donor also contains this destructor and must not gain a second linked definition on future import.
- J3DModelData retains the original empty destructor out of line, matching its native header declaration. Original skin and cluster operations are restored as complete files; their vertex-buffer swap, normal blending and cache-store behavior are unchanged.
- All five existing native paired-single matrix translations retain their exact scalar/FMA/store order and alias behavior. The complete donor MW branches remain guarded beside native code. Texture-matrix and translation/rotation helpers are the complete original bodies. The native transform initializer retains implicit structure padding instead of introducing a conflicting explicit padding member.
- J3DSys now owns its original construction, reset commands, texture cache and traversal globals together. Its null IA8 texture still reserves 32 bytes for the complete 4x4 tile; the Wii 16-byte BSS object depended on neighboring zero storage and must not be restored as a native out-of-bounds upload. Aurora's existing GX umbrella replaces an unavailable leaf header; unused OSFastCast inclusion is removed.
- JMath quaternion interpolation and vector scale-add retain exact native branches. JMAVECLerp, previously declared only in donor headers with an assembly-only implementation, now has the native translation of its original subtract/fused-add/store sequence. Inputs are loaded before writes; XY paired stores retain signed-denormal flushing while scalar Z preserves its IEEE representation.
- Joint, Transform and JMath retain contraction-off compiler pragmas. Explicit std::fma stays only at translated fused instructions. No new source flags are required.

## Regression and evidence

OriginalJ3DJointTraversalTests adds a tenth group for the newly restored JMAVECLerp: a fused residual of -2^-46 that separate rounded multiply/add loses, positive/negative denormal XY stores versus scalar Z, and both inputs as output. Existing nine groups continue to exercise actual original hierarchy calculation, calculator inheritance/restoration, callback effects, scale modes, animation, exception cleanup and nested traversal.

`python3 notes/compat-owner-removal-round4-20260925/j3d/validate-source.py` passes **52 checks**, covering **79 unique restored function names**. It verifies complete donor method sets, exact unadapted bodies, one CPP provider per function name, exact old native paired-single branches and error paths, preserved MW bodies, tile extent and initialized traversal ownership. These are source checks only; parent owns builds and execution.

All sixteen edited/deleted existing files were clean at batch start. before.json records initial status and hashes; baseline/ contains their originals. source-changes.patch is the exact lane delta. restore-owners.py records the one-time donor transformation (do not rerun after integration). No initially dirty user file was changed.

## Build handoff

build-wiring.json lists the nine canonical additions and fourteen deleted providers. Parent can add the canonical paths to the native Game build source list; the compat wildcard naturally loses removed files. No additional source flags or test targets are needed. Existing original-j3d-joint-traversal tests now have ten groups.

Recommended existing validation covers joint-traversal, xanime-core/player, model/joint/geometry resources, matrix buffers, texture matrices, vertex buffers and GX command scheduling, followed by the usual original-process runtime check. No build, Git index, staging, commit or push operation was performed by this lane.

## Explicit remaining scope

OriginalJ3dJointTree.cpp/.hpp is still used by J3dModelRenderer plus joint-traversal and LiveActorUtil tests. It constructs a non-sharing native preview ownership graph and contains a duplicate partial hierarchy linker. Removing it coherently requires switching those renderer/test consumers onto complete model-resource ownership. It is not moved into a renamed directory here.

J3dSystemCompat only exposes renderer access to the actual J3DSys view matrix; that remaining accessor integration is separate. J3DTransformAnimationCompat owns animation sampling, not J3DTransform's geometric matrix helpers, and is unchanged.
