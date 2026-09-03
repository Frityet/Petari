# Original JPA Overwrite closure — field checkpoint (2026-09-03)

Five complete original methods were recovered into `src/Game/System/Overwrite.cpp`, preserving existing methods. The only additional includes are `JPAFieldBlock.hpp` and `JPAParticle.hpp`. The prior frozen `original-jpa-resource-loader-20260903` package was not changed.

| Method | Retail address / size | Original compiler objdiff |
|---|---|---:|
| JPAFieldAir::prepare | 0x803A62C0 / 0x8C | 77.942856% |
| JPAFieldVortex::prepare | 0x803A634C / 0x64 | 99.8% |
| JPAFieldVortex::calc | 0x803A63B0 / 0xEC | 83.47458% |
| JPAFieldConvection::prepare | 0x803A649C / 0xA0 | 99.875% |
| JPAFieldSpin::prepare | 0x803A653C / 0x98 | 99.60526% |

The two lower scores have concrete current-header inlining differences: Air's TVec3 copy constructor becomes paired-single copy instructions instead of the original out-of-line constructor call; Vortex's subtraction and squared-length helpers become the original paired-single operations inline. The branches, field accesses, transforms, normalization helper, force calculations, and final calcAffect are retained. No compiler pragmas or header edits were added to force a match. `field-inlining-comparison.asm` records the compiled functions with relocations; the five `Field*.asm` files record the DOL routines. The three close matches differ at relocated constant loads/header details. These are functional recoveries, not exact instruction-match claims.

`OriginalJPAFields.cpp` is a literal native extraction of the five root bodies and is staged only under ignored `build/original-jpa-overwrite-20260903/staged`. The full root source at this checkpoint is frozen as `Overwrite-fields.cpp`. `root.patch` is the exact append/include diff against `Overwrite-before.cpp`; `root-evidence.json` records the configured original compiler and source hash. The parent owns the root commit and subsequent native activation.

The standalone CPU fixture constructs and prepares **all 2,097 actual JPAFieldBlock objects** from the retained host-order real JPC data. Their original field vtables now link. It also checks actual Air, inner/outer Vortex, Spin and Convection force results on deterministic real-class fixtures, and destroys the real JKR domain. No emitter substitute, draw stub, GPU context or shared build is involved. `native-field-evidence.json` records the commands and passing runtime output; `field-probe.cpp` is the complete fixture.

The next closure is seven original draw callbacks and their original anonymous projection/direction/rotation/base-plane/list-step helpers and static tables. Those methods have not yet been added at this checkpoint. Completing them will permit the unchanged full original Resource::init dispatch graph to link; the manager fixture must then run before claiming full manager/emitter ownership is proven.
