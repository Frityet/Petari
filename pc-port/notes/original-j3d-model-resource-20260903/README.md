# Complete retained J3D model construction

The native resource owner now combines the existing typed INF/JNT/EVP/DRW,
VTX/SHP, MAT/MDL and TEX components into actual J3DModelData and J3DMaterialTable
instances. The original factories, hierarchy builder, shape-node setup,
VCD/VAT sorting, important-matrix selection, billboard setup and binary
display-list conversion perform construction and finalization.

This supplies the model/resource boundary needed by original ResourceHolder and
MarioAnimator. Their runtime activation and the original Mario jump/floor loop
are separate pending integration work. The existing showcase still uses its
current runtime model path until that migration is complete.

## Original source and native ownership

- Two missing original SDK dispatchers were recovered in root and committed as
  61d7acb95. Their 276 relocated bytes equal retail. Native dispatch preserves
  null/unknown returns, v26 BMT3, and both BDL3/BDL4. Ordinary BMD3 is supported;
  the recognized v21 BMD2 implementation remains explicitly unavailable.
- The existing original J3DJoint::addMesh inline body is imported. It prepends
  only when a previous head exists and otherwise preserves the incoming next
  pointer. Hierarchy source and finalization correspondences are recorded in
  finalization-correspondence.md and finalization-source-evidence.json.
- Actual model/table and material-factory allocations use a retained original
  JKR domain. Native parsers, pointer metadata, caches, registration and typed
  component backing use host allocations. Initial geometry command storage
  remains host-owned; subsequent SDK command allocations use the active domain.
- SDK load calls accept only a registered bounded source. Explicit archive
  aliases verify the complete byte contents and extent and retain the decoded
  resource/domain. Registry locks are released before original work and before
  final owner destruction. SDK return values are borrowed from that owner.
- Original material address-derived identity uses disjoint, aligned original
  cached-address reservations instead of truncating host pointers. This is an
  identity namespace, not a physical-memory mapping. Freed intervals coalesce
  and are reused; their metadata remains independent of Game heap lifetimes.

## Validation and caller state

The retained hierarchy is checked against all actual tables before invoking
the original recursive builder. The check preserves legal pointer overwrites
and shared links, and rejects out-of-bounds references, missing required parent
contexts and cycles. Shape matrix references must fit the actual draw-matrix
allocation, retaining the original 0xffff matrix-reuse sentinel.

Binary texture patching is validated in original material order. Overlapping
display-list views share a 32-aligned preview allocation. Each step checks its
read/write extent and texture/GX slot, then uses the actual SDK loadTexNo
primitive to produce the bytes later materials will see. This handles MDL
aliases without assuming that later views still contain pristine indices. The
preview restores GD, selected texture and texture-scale state; the real original
indexToPtr routine then patches the retained original command views.

The full finalization runs inside a native command scope that pins cooperative
SDK execution and restores the caller's GD object and interrupt state. Original
makeVcdVatCmd and indexToPtr have verified one-time static interrupt snapshots;
their algorithms are preserved. This native ownership boundary prevents one
load's entry state from changing a later native caller's state.

Destroying the currently selected texture clears that borrowed J3DSys pointer.
Destroying an older model leaves a different live model's texture selected.
Actual SDK objects and components are destroyed before the last heap reference.

## Current native evidence

The macOS arm64 LLVM 23 shared build and OriginalJ3DModelResourceTests pass:

- Null/unsupported dispatch, unsized unregistered-pointer rejection, and the
  original empty BMT3 texture fallback.
- Exact source aliases, repeated aliases, resource/domain lifetime and release.
- Original-width identity lifetime, state bits, nonaliasing and reuse.
- Complete retail Mario models in normal, locked and patched material modes
  (flags 0x01200000, 0x01201000 and 0x01202000), with all actual table counts,
  forward/reverse hierarchy links, shared display lists and original joint
  matrix calculations.
- First finalization with interrupts disabled, later finalizations enabled,
  and restoration of an existing caller GD object.
- Older/current selected-texture retirement, ordered aliased-patch rejection,
  malformed joint bounds, and BDL3/BMD3 format variants of retained retail data.
  Those header variants are fixtures; they are not claimed as additional retail
  files. The BMD fixture verifies original no-matrix flag finalization.

The material component has its own six-group normal and selected ASan/UBSan
evidence in original-j3d-material-table-20260903. Its sanitizer run found an
unaligned original four-byte TEV-order assignment; the native SDK header now
uses an exact four-byte copy, preserving the opaque fourth byte. No Game
algorithm is changed by that architecture fix.

The complete owner passes all four groups under ASan/UBSan with leak detection,
including all three real Mario model flags. The run instruments 58 objects
covering the new owners, actual factories/finalizers/matrix traversal and heap
lifecycle, and verifies 246 included workspace files and 17 frozen archives.
Remaining uninstrumented dependencies and reproduction are listed in
sanitizer.md and native-asan-evidence.json. These are CPU resource/SDK checks,
not a rendered animation or gameplay jump test.

The final shared showcase build succeeds. Fresh material-table, JMap, BckCtrl,
Xanime-player and actual-camera regression binaries pass; integration-gates.json
records their exit codes. Fresh title/Gateway smokes also pass with the existing
600-frame cap, completing at two and five rendered frames respectively. These
exercise the existing showcase route and do not yet route its renderer through
the new complete model owner. Aurora cbd08e9 is pushed and supplies the verified
4x4 matrix identity/copy providers; its real CMake matrix target passes.
