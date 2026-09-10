# Original area managers and generic placements — 2026-09-10

Original Mario movement queries area managers even when no actor of that kind is placed. The prior host container derived manager existence from supported actor placements, causing valid empty EffectCylinder, RestartCube and WarpCube queries to fail during startup. The original AreaObjContainer constructs its manager table independently.

The compatibility registry now separates manager ownership from complete actor placement support. It constructs 66 actual original manager types in their retail order and capacities: all 61 base AreaObjMgr entries and five available concrete managers. The unavailable GlaringLightAreaMgr remains explicit. Creating an empty manager never enables an unsupported specialized actor; RestartCube and WarpCube placement preflight still reject their missing full routes.

All 33 generic AreaObj factory variants are now registered with their exact original forms, manager names and capacities, adding 28 to the five previously registered. No specialized area is replaced with the generic type. No Game source changed. A separate agent parsed the retail assembly table/string relocations and verified all 66 manager and 33 generic placement rows; see the adjacent original-generic-area-tests-20260910 notes.

Validation: fresh showcase and area fixture builds succeeded. All four bounded area groups passed: original effect area initialization with synthetic BCSV and real scene ownership; independent manager/placement readiness; actual 66-manager scene lifetime; and descriptor registry consistency. The full legacy area fixture is not clean: its prior run retains unrelated older owner/setup and source-mirror failures. The targeted selection is explicit in bounded-runtime.json.

The real-disc showcase now completes the first original Mario movement and original CameraDirector movement. It still stops in MarioActor::calcAnim with nonfinite root/hand model positions; matrix recovery is separate ongoing work. This checkpoint is not a playable-demo claim.
