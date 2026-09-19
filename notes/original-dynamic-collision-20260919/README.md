# Original generated collision ownership — 2026-09-19

## Scope and behavior

Original DynamicCollisionObj owns independent position, normal, prism and numeric-halfword octree arrays. The previous native KCollision triangle count subtracted the octree and prism pointers, which only has meaning inside a decoded contiguous disc resource. Native collision publication also retained only immutable disc bytes, so original syncCollision mutations would leave the cached surfaces unchanged.

The typed resource registry now retains an explicit triangle count for both decoded and generated geometry. `GeneratedKCollisionResource` retains the caller's actual independent arrays, validates every prism reference, converts the authored Wii-order halfwords into aligned native node/leaf storage, and retires the file identity when its final owner dies. It preserves the original special single-root shifts (-1/-1) and actual descending prism list. No octree or PA attributes are invented. Decoded resource behavior remains covered by the existing ten tests.

StageCollisionService can retain a generated resource with its actual original KCollisionServer, CollisionParts, sensor and placement zone. Geometry refresh reads the same live arrays, preserves original part/local-prism/global-surface identities and zone encounter order, and rebuilds the native spatial index after successful validation. Rejected mutation publication quarantines the source: original line/area queries and native ray/sphere/movement queries fail explicitly while it is enabled, instead of reading rejected live arrays or succeeding from stale cached geometry. A successful corrected publication clears quarantine; simply re-enabling membership does not. Previously published surface snapshots remain inspectable for diagnostics. Actor retirement disables the existing registrations before the original parts die; retained source storage remains valid until scene-cache retirement. Malformed topology, pointer replacement and bad prism references fail explicitly. Nonpositive prism heights retain stable identity slots while leaving the native query index, matching original KCollision filtering; restoring positive geometry reactivates the same slots. Original calcFarthestVertexDistance can mark near-parallel prisms inactive, and the native publication boundary preserves that supported state.

The original CollisionParts bounding-range update now publishes generated geometry through its native provider. The original DynamicCollisionObj::syncCollision timing is unchanged: update header, update triangles, recalculate farthest distance, update bounds. The new native DynamicCollisionObj provider only replaces createCollision to supply retained typed allocations and publish a real original CollisionParts; its other original methods are unchanged.

Canonical DynamicCollisionObj and recovered CollisionArea CPP/headers were imported into Game. The build excludes only DynamicCollisionObj.cpp in favor of its native provider. Exact source equivalence is recorded in `source-equivalence.json`: the four Game files differ only by the repository's compile-time CP932 literal handling, and the native dynamic provider differs only in createCollision and its support declarations. CollisionArea recovery is upstream decomp commit `3787203d8ad835372a917795ee5ac0cc85025b49`.

## Validation and limits

- Latest `resource-build2.log` and `resource-test2.log`: original KCollision resource target rebuilds and all 12 cases pass, including the existing ten decoded-resource cases and two generated-resource cases. New coverage proves separate-array count, authored leaf order/endian conversion, mutation visibility, no fabricated PA records, shared lifetime, final retirement, malformed topology rejection, duplicate-owner rollback and bad-index rejection.
- `registration-build4.log` and `registration-generated-test3.log`: `smg-pc-stage-collision-registration-tests --generated-only` passes. This runs the actual DynamicCollisionObj constructor and original updateCollisionHeader/updateTriangle/syncCollision methods, with actual original CollisionParts and KCollisionServer below CollisionParts::init. Its retained independent arrays are updated and queried through original octree traversal and the native spatial index. It checks stable part/local-prism/global-surface IDs, disabled membership, collapsed-face inactivity and reactivation, all-negative-height inactivity, a transform committed while all prisms are inactive, malformed-reference rejection and query quarantine, and retirement before the borrowed server is destroyed. The sensor argument is an explicitly labeled opaque identity token, never dereferenced at this boundary; this does **not** establish actor initialization, HitSensor construction/grouping, DynamicCollisionObj::createCollision or AreaPolygon::init.
- The default registration suite is **not passing** in the current repository. Its pre-existing first test dereferences an absent SceneObjHolder through MR::isExistMapCollision -> original MR::getCollisionDirector before the new case can execute. `registration-debug2.log` is the stack; `default-suite-frontier.json` confirms the failing first test body is identical to the committed baseline. The explicit `--generated-only` mode preserves the default suite and all pre-existing tests; it is focused coverage, not a hidden full-suite success. The first attempted focused fixture used SceneExecutionFixture and correctly rejected absent original GameSystem scene-controller ownership (`registration-generated-test.log`); the final fixture exercises only the lower collision boundary described above.
- Full original AreaPolygon probe: compiled successfully (`dynamic-build2.log`, `dynamic-build3.log`) but did not reach collision. Its RuntimeOwned fixture first required an explicit standalone resource-language owner (`dynamic-debug2.log`); the independent language fix has a passing focused test in `../original-resource-language-20260919/`. It then reached absent original FileLoader ownership during MessageHolder::initGameData (`dynamic-debug3.log`). FileUtil and font ownership are unchanged. This is not an AreaPolygon, gameplay or Rosalina completion result.
- CollisionArea remains absent from the placement factory until its original actor behavior and full scene ownership can be exercised. Its already existing manager descriptor is not equivalent to actor support. Do not enable the placement merely because the imports compile.

## Pending integration gate

Use an actual original process owner to run AreaPolygon's six face changes with initialized Mario mode, its real sensor and CollisionDirector, authored zone and camera-code context; compare original octree queries against the cached native geometry and exercise actor/scene retirement. The attempted RuntimeOwned Gateway fixture (`OriginalAreaPolygonIntegrationProbe.cpp`, preserved as diagnostic evidence rather than a default failing test target) is no longer a complete original-process substitute: repairing its FileLoader and likely font dependencies would expand beyond this collision task. The focused resource/publication tests keep this boundary explicit. Both final focused targets were built normally against the current Aurora FIFO source, and `git diff --check` passed for the changed collision/test files.

## Commit scope

- `src/resource/KCollisionResource.hpp` and `.cpp`
- `src/scene/StageCollisionService.hpp` and `.cpp`
- `src/compat/CollisionPartsCompat.hpp` and `.cpp`
- `src/compat/OriginalCollisionPartsCompat.cpp`
- `src/compat/OriginalKCollisionCompat.cpp`
- `src/compat/OriginalDynamicCollisionObj.cpp`
- `src/Game/MapObj/DynamicCollisionObj.hpp` and `.cpp`
- `src/Game/AreaObj/CollisionArea.hpp` and `.cpp`
- only the `MapObj/DynamicCollisionObj.cpp` exclusion in `src/Game/xmake.lua`
- `tests/OriginalKCollisionResourceTests.cpp`
- `tests/StageCollisionRegistrationTests.cpp`
- this note directory (including the blocked integration probe and logs)

The language owner is an independent checkpoint. No AreaObjRuntime or actor-factory changes are included.

The parent checkpoint may pin the already published decomp submodule at `cc77fe564c4da3f8ee35605134e2add83628022a`, which contains the earlier CollisionArea recovery and a later source-only ShadowVolumeLine recovery. This generated-collision change does not import or enable ShadowVolumeLine or PunchingKinoko; their native actor/dependency closure remains separate and absent.
