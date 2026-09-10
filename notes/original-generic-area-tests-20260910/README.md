# Generic AreaObj placements and independent manager catalog

Independent audit and fixture changes by depth_completion. Parent owns the implementation and root checkpoint. No Game source was edited.

## Retail correspondence

Parsed the actual relocation symbols and string labels in retail `asm/Game/NameObj/NameObjFactory.s` and `asm/Game/AreaObj/AreaObjContainer.s`, under `notes/gateway-audit-20260907/restoration/retail/`. Every one of the decomp's 67 manager rows matches its retail name, capacity, order and concrete manager creator. All 33 original generic AreaObj placements match the decomp factory table and the new runtime descriptor name/form/manager/order/capacity. No specialized area was substituted with AreaObj.

The final independent manager catalog has 66 entries: all 61 original base AreaObjMgr rows and the five ready concrete CubeCameraMgr, WarpCubeMgr, WaterAreaMgr, LightAreaHolder and ImageEffectAreaMgr rows. Only GlaringLightAreaMgr (order 31) remains explicitly unsupported. All 66 catalog rows were independently compared to the retail table, including creator type and camera finalizer presence. See `retail-mapping-audit.json` for every original row and comparison results.

Retail AreaObjContainer::init (`AreaObjContainer.s:218-267`, addresses 8001EC64-8001ED08) unconditionally iterates all 0x43 manager rows. The compare at 8001ECD4 proves manager existence is independent of whether any placement uses it. Separating real manager readiness from actor placement readiness preserves that behavior without marking specialized actors complete. In particular, an empty RestartCube AreaObjMgr can return a real miss while a RestartCube placement remains blocked.

## Fixture changes

`tests/AreaObjRealOrAbsentTests.cpp` now verifies every manager catalog row in retail order and its actual capacity, the original prefix lookup, all 61 concrete base AreaObjMgr instances, and exact manager metadata agreement for every completed placement. The old exact16 placement-count assertion was stale even before this change; previously asserted descriptors are now looked up by canonical identity, while all published descriptors must be unique and ordered.

A new manager/placement readiness test checks that the actual RestartCube manager exists and returns null with no placed actor, while specialized RestartCube placement preflight remains unavailable. The actual WarpCubeMgr is checked for its original null active-cube state and empty volume result; specialized WarpCube placement remains unavailable. Queries for unavailable GlaringLightAreaMgr must still throw.

A new synthetic BCSV test runs original AreaObj::init for EffectCylinder and SmokeEffectColorAreaCube, preserving actual object names, transformed scale, all eight signed arguments, original scene manager registration and dynamic types. It asserts an empty-manager miss before init, a real-volume hit, exclusive radial/side edges, cube-top miss, above-cylinder-top miss, below-base misses, invalidation/revalidation and final NameObj registry restoration. These are the same manager and argument APIs MarioEffect::doCubeEffect uses (MarioEffect.cpp:1256-1275).

## Validation status

The first shared target build succeeded. The first whole-fixture run exited 1 with six failures (see runtime.json/log): the new manager catalog and manager/placement separation groups passed, but the new effect init plus existing real-disc init groups lacked the now-required active scene executor, and the old LightArea fixture also lacked that owner. The old exact source-boundary check additionally finds LightAreaHolder::sort present in the port and absent in decomp. These are retained as explicit limitations, not hidden by claiming the full fixture passed.

The effect init fixture now uses the existing SceneExecutionFixture with real original executor, SceneObj owner, and Game allocation domain. An optional test-name substring argument permits bounded runs; no argument still runs every test and retains all old checks. The fixture translation unit compiled with the root's exact LLVM23 settings, exit 0. Parent then assigned the shared build lane: the final target build passed 0, and all four bounded area groups passed 0. The ready JPC billboard target also passed all seven groups, including effect metadata surviving original scene-heap retirement. The lane has been returned to the parent.

Retail boundary detail: AreaFormCylinder::isInVolume calls inclusive MR::isInRange for axial height (AreaForm.s:962, MathUtil.s:2436-2462), then strictly compares radial distance. The test uses safely inside/above-top samples rather than requiring the nominal exact world-space top: LLDB measured the initialized axis as y=1.00000012, so the nominal point projects slightly beyond the inclusive height. See cylinder-top-lldb.log. This was a test expectation correction, not a production math change.

## Final bounded runtime proof

All commands and exit codes are in `bounded-runtime.json`. The four area filters are `generic effect`, `manager readiness`, `scene holder owns`, and `descriptor registry`, each exit 0. Exact area binary SHA256: `0a4161703953cf5ad00d8e86d74e795ceed4612409cfc269dac8e2725a025106`. The actual synthetic BCSV init group passes all geometry, original registration, eight argument and teardown checks.

JPC billboard runtime: 7/7 groups passed, exit 0; exact binary SHA256 `b6ea1143b6ad14dbb34983352332dd907475b046f158f5146ecb227e1a972d68`. This additional run was requested by the parent; no particle production/test source was edited here.

The old whole-suite result remains explicitly exit 1 in runtime.json/log. This bounded validation does not claim that the unrelated legacy fixture ownership migrations or missing decomp LightAreaHolder::sort correspondence were repaired.
