# Original camera projection and native geometry — 2026-09-03

Applied the separately verified camera/shared-header package from
`original-camera-compile-closure-20260903/native-headers.patch`: native pointer
storage and numeric packed-halfword access, original missing declarations,
MSL mem.h include compatibility, and original generic matrix operations.
The original 4x4 concatenate retains sixteen temporaries so input/output
aliasing is safe. The independent JPA package's literal
`TVec3::setLength(const TVec3&, f32)` overload is also active. Complete camera
source/owner activation and the JPA loader package remain separate work.

The current root `CameraContext::updateProjectionMtx` has a non-matching
warning. Its original-compiler output differs in register allocation and
scheduling from the retail 724-byte function. Before native activation,
`verify-projection.py` executes both complete scalar instruction streams for
1,540 cases and compares all sixteen final projection components by binary32
bits. All 24,640 components agree. The interpreter rejects unknown arithmetic,
calls, data references and memory accesses; it ignores only verified stack
callee-save spills and restores. The retail object is relocated against the
verified RMGK01 DOL at 0x80097708 before being used as the reference.

The same cases are then compiled/run through the native JGeometry header,
including a concatenate whose output aliases its right input. All 24,640
components agree with the retail instruction stream. Cases cover eleven FOVs,
four aspect ratios, five clipping ranges and seven shake offsets, including
signed zero and nonzero shifts. This is a bounded projection arithmetic check,
using the same host double tan for both instruction streams. It does not claim
bit-exact Wii libm behavior, arbitrary inputs, a full CameraDirector runtime,
or working jumping.

The native perspective helper now preserves the original arithmetic boundary:
float multiply/divide for the half angle, double tan, float rounding, then a
float reciprocal. The previous native helper selected the float tan overload.
No root/Game projection algorithm was changed.

Validation: `smg-pc-game-math-rotation-tests` rebuilt with LLVM 23 after these
shared-header changes and passed. `math-runtime.log` retains its result. The
projection proof and native probe can be repeated with:

```
python3 pc-port/notes/original-camera-context-20260903/verify-projection.py
```

The next actual CameraContext dependency is the original screen/system
configuration chain (`isScreen16Per9` -> `isAspectRatioFlag16Per9` ->
`SCGetAspectRatio`). The existing native screen helper infers configuration
from a published camera pose; the original owner must receive actual system
configuration through the shared SDK boundary instead.
