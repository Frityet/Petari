# Original collision-shadow native preparation

This package prepares the complete existing `CollisionShadow`, `MarioActorShadow`, and `FootPrint` implementations for the actual actor draw pipeline. It does not replace collision queries or publish a synthetic shadow texture. Production application, shared linking, and GPU validation belong to the parent integration task.

## Source and compiler evidence

- `src/Game/Player/MarioShadow.cpp`: replace obsolete `extern "C"` declarations accepting `int` with the actual typed GD headers and equivalent enum constants. The complete original TU compiles with GC 3.0a3. `CollisionShadow::createDL` at RMGK01 `0x80314CCC` (396 bytes) matches retail **99.79798%**, with the same direct call sequence. The corrected types recover the actual ABI; the old incorrect declarations were not instruction-identical.
- `libs/RVL_SDK/include/revolution/gd/GDGeometry.h`: declare the existing original `GDSetGenMode(u8,u8,u8)` implementation.
- `src/Game/System/Overwrite.cpp`: recover `JUTTexture::captureDolTexture` at `0x803A462C` (152 bytes), **100%** original-compiler match. This is the game's actual JUT override: doubled copy-source extent for mipmap, original destination extent/format, `GXCopyTex(..., GX_FALSE)`, then `GXPixModeSync`.
- Native `OriginalJutCapture.cpp` contains that exact body. Native SDK declarations add the actual static method and original `GXXFFlushVal` enum (`NONE=0`, `SAFE=8`).
- `MarioActorShadow.cpp`, `MarioShadow.hpp`, `MarioSwim.hpp`, and both FootPrint files are exact root copies. The previous root recovery and blur-layout proof are in `../mario-shadow-view-20260903/`.
- The native actor header/initializer/draw synchronization includes the already proved `const AreaObj* _20C`, one 16-pointer blur array initialized in the original interleaved order, and the draw-bank stride of eight pointers. Concurrent native FurMulti and screen-box declarations are preserved.

Run `python3 pc-port/notes/original-shadow-native-20260903/verify-original.py`. The script verifies the local RMGK01 DOL SHA1, compiles both root TUs, checks direct call order and fuzzy thresholds, and writes `compiler-evidence.json`. Binary/compiler artifacts remain under ignored `build/original-shadow-native-20260903/`.

## Native preparation and boundary

`native.patch` is relative to `pc-port/`; `native-files.json` lists exact destination/source identities. The overlay is `build/original-shadow-native-20260903/staged/`. No native production sources or nested Aurora files were changed by this package. The parent must remove the existing `Player/MarioShadow.cpp` Game target exclusion when activating it. FootPrint and the capture provider use the ordinary source glob.

`python3 build/original-shadow-native-20260903/compile.py` passes all six isolated TUs: CollisionShadow, MarioActorShadow, FootPrint, capture override, MarioActorInit, and MarioActorDraw. The first five use current actual game target flags; MarioActorDraw uses the parent-approved live-walk target flags with native/Aurora includes before root fallback headers (IceStep is not yet mirrored). This is compilation evidence, not a native link or gameplay result. `native-compile.json` records complete commands. The root-first `setPositionFromLookAt` and alpha-clear provider are separately parent-owned.

The archive-only undefined-symbol inventory in `unresolved.txt` is a snapshot, not a final link verdict. Required real providers include existing Mario model/access methods and TDDraw helpers; original `MR::createAreaPolygonListArray` requires the actual CollisionDirector and keeper ownership chain. Root `MR::calcCylinderCenterPos`, `MR::isInitializeStatePlacementSomething`, `TDDraw::drawFillBox3D`, and `TDDraw::fix2Dpos` remain missing bodies in this snapshot. Do not substitute empty geometry or a dummy scene owner. Parent is closing the independently owned draw/math frontier.

## Ownership required by actual activation

`CollisionShadow` is a NameObj with an empty original destructor, not a JKRDisposer. Its arrays, triangles and display list are scene-heap allocations. Its constructor allocates a 160x160 I8 JUTTexture in the actual scene GDDR3 heap and selects it through `MR::setMarioShadowTex`. A native actor/scene owner must retain the corresponding resource cohort, retire scheduler and queued draw references, clear the selected shadow texture when appropriate, explicitly retire this exact JUTTexture, then free the cohort. Bulk heap release alone does not release the native mapped JUTTexture registry lease. Do not add a speculative Game destructor or delete unrelated texture leases.

FootPrint similarly retains its print array and a JUTTexture wrapper over an actual borrowed TIMG. Its resource archive must outlive that wrapper, and its registered movement/draw callbacks must be removed before retirement. The wrapper still needs its destructor for GX state retirement although pixel ownership is borrowed.

Construction of CollisionShadow::createDL must occur inside the real scene GD/J3DSys scope: the original method selects a stack GDLObj, and its scheduler/interrupt guard does not itself restore the selected GD pointer. Drawing needs the original scene phase matrix/state ownership. The existing SceneJ3dScope is the integration boundary. The original `viewCalc3(3, nullptr)` behavior and the CollisionShadow mode-specific pointer preconditions are preserved; this preparation does not invent a fallback for them.

## Applied integration

The native patch and source selection are applied. The later generic JKR finalizer in `../jut-heap-retirement-20260903/` now retires constructed texture wrappers after original disposers and before heap storage reuse, superseding the manual-wrapper-cleanup requirement above. Queued-capture and heap teardown tests pass on Metal. Original collision ownership and TDDraw geometry still gate complete Mario runtime; no full shadow rendering claim is made.
