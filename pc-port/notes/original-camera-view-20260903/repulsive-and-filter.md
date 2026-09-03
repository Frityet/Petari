# Original camera repulsion and triangle-filter dependencies

This tranche supplies the area and filter dependencies used by the original
`CameraViewInterpolator`. The camera algorithm, runtime camera publication,
and `CameraUtilCompat` were not edited here.

## Source changes

- Imported `Game/AreaObj/CameraRepulsiveArea.cpp/.hpp` byte-for-byte from root.
  The existing root repulsion functions required no corrections.
- Recovered `MR::createTriangleFilterFunc` in root
  `src/Game/Util/TriangleFilter.cpp`, then mirrored that file exactly to PC.
  The existing root/PC `TriangleFilter.hpp` remains unchanged.
- Recovered `MR::calcCylinderUpVec` and `MR::getCylinderRadius` in root
  `src/Game/Util/AreaObjUtil.cpp`. The PC equivalents are exact body extractions
  in `compat/CameraRepulsiveAreaUtilCompat.cpp`.
- Added typed `CameraRepulsiveSphere` and `CameraRepulsiveCylinder` creators
  to the existing `AreaObjRuntime` descriptor table. The original
  `NameObjFactory` uses those concrete classes, and `AreaObjContainer` supplies
  their shared `CameraRepulsiveArea` manager at index **34**, capacity **0x80**.
  These are general authored object registrations, with no stage-name condition.

The PC `AreaObjUtil.cpp` translation unit is active and already supplies
`calcCylinderPos`; it has not been changed. Only its two missing methods are
extracted into compat. A future whole-file synchronization must remove those
two extracted providers to avoid duplicate definitions. The original
`AreaFormCylinder` implementation and header were already present in PC.

## Retail evidence

Verified executable: RMGK01 rev0, SHA-1
`25c5959534b3c21246c6c7e42021b916b41fb578`.

| Function | Address | Size | Exact instructions |
| --- | --- | ---: | ---: |
| `CameraRepulsiveSphere::getRepulsion` | `0x80020228` | `0x14` | 5 |
| `CameraRepulsiveCylinder::getRepulsion` | `0x8002023c` | `0xfc` | 63 |
| `MR::calcCylinderUpVec` | `0x804008f4` | `0x10` | 4 |
| `MR::getCylinderRadius` | `0x80400904` | `0x0c` | 3 |
| `MR::createTriangleFilterFunc` | `0x804081ec` | `0x48` | 18 |
| `TriangleFilterFunc::isInvalidTriangle` | `0x8009da5c` | `0x14` | 5 |

The sphere's zero return is **actual original behavior**, not an unfinished
decompilation placeholder. Retail loads `0.0f`, stores it to all three return
components, and returns. No position or radius is read.

The cylinder first calls `calcCylinderPos` for the cylinder base and
`calcCylinderUpVec` for its axis. It subtracts the base from the query position,
removes the projection onto the axis, and computes the cross-product length.
With `base = 2 * cross.length() / radius`, it multiplies `base` twice more and
divides the radial vector by `base^3`. The original floating-point operation
order is preserved. There is no clamp or singularity fallback added at the
axis or at a zero authored radius.

The cylinder-axis helper forwards to
`static_cast<AreaFormCylinder*>(area->mForm)->calcUpVec(output)`. The radius
helper reads that form's `_20`. Both follow the original form ownership rather
than recomputing geometry from placement names or scales.

The filter factory performs `new TriangleFilterFunc(func)`. The callback method
dispatches the stored function pointer and returns its boolean unchanged.

## Verification

All three full root translation units compile without diagnostics using the
original GC/3.0a3 compiler, wibo, and configured Game compiler flags. The
included verifier resolves branch/vtable relocations and checks the original
`0.0f`, `1.0f`, and `2.0f` constants at `0x806b7de4`, `0x806b7de0`, and
`0x806b7de8`. It also verifies the retail startup instructions setting
`r2 = 0x806bfc20` before resolving the constant loads.

Result: **98/98 instructions match exactly after 21 verified relocations**.
This covers the six functions listed above, not every function in those
translation units.

```sh
python3 pc-port/notes/original-camera-view-20260903/verify-repulsive-filter.py --compile
```

Compiler commands, compiled objects, and `verification.txt` are under ignored
`build/compat-camera-repulsive-filter/`. Initial retail disassembly is under
ignored `build/compat-camera-view/{repulsive,cylinder_helpers,triangle_filter}.asm`.
The source and header mirrors were compared byte-for-byte. Native build/test
execution is coordinated by the root task and is not claimed by this note.

## Ownership and Binder contract

`TriangleFilterBase` has no virtual destructor in the original header.
`Binder::setTriangleFilter` stores a borrowed pointer, and the native Binder
destructor does not delete it. A native owner for `CameraViewInterpolator`
must retain and delete the actual `TriangleFilterFunc*` with its concrete type,
separately from deleting its Binder. No virtual destructor or ownership policy
was inserted into the Game class to change that contract.

Filter polarity is preserved: `isInvalidTriangle == true` means reject that
triangle. The existing PC `make_collision_triangle_filter` adapter negates
the result before passing it into collision detection, so rejected triangles
cannot contribute a push vector or contact plane. `MR::isCameraCodeThrough`
already reads the real collision camera code through the shared map provider.

The camera's Binder uses a null matrix, live position/gravity vectors, and a
64-plane capacity. Those inputs are accepted by the existing compatibility
Binder; its offset is zero and the nonzero capacity preserves contact planes
and `mPlaneNum` for `CameraViewInterpolator::calcBinder`. The returned vector is
the resolved displacement, which the original camera consumes. No additional
Binder defect was established by this bounded dependency audit; this is not
a new claim of complete retail collision equivalence.

The unrelated `TriangleFilterDangerCode` method at `0x80408234` remains outside
this recovery. It is not called by the camera view interpolator, and no
substitute implementation was introduced.
