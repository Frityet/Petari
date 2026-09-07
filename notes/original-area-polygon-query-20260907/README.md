# Original area polygon collection — 2026-09-07

Restored the missing native `MR::createAreaPolygonListArray` used by original shadow code through the existing scene-owned collision service. There is no actor or stage special case and no fabricated CollisionDirector, CollisionParts, sensor, or empty result.

## Original data contract

The API collects polygon **bounding-box** overlaps; input points do not define an exact convex clipping volume. The recovered `CollisionCategorizedKeeper` body forms a world AABB, visits numeric zones and each zone's current part order, skips disabled parts, and applies the original expanded-axis sphere/box predicate. `CollisionParts` transforms every input point by its full inverse matrix, forms the local AABB, and calls `KCollisionServer::checkArea3D`.

That original KCL method expands each collapsed local axis by one unit, clamps converted coordinates to the KCL integer grid, visits octree leaves in z/y/x order, ignores nonpositive prism heights, deduplicates repeated prism pointers, tests inclusive triangle AABB overlap, and stops at the supplied capacity. It does not add prism thickness or replace encounter order with file/BVH order. Original stack limits are 32 points and 512 output prisms. The native boundary rejects oversized/null/nonfinite requests and safely handles zero capacity.

`invalidateCollisionParts` calls original `CollisionZone::eraseParts`, which swaps the last member into the removed slot. Revalidation appends. Native retained membership observers apply those changes synchronously, including multiple enable changes between queries; callback updates allocate nothing. Membership weak ownership expires on scene clear/destruction. The existing actor dead flag is checked before any borrowed actor/sensor data; owner release removes membership before retiring that flag.

## Native ownership and identity

Each existing KCL source retains its exact bytes, matrix, PA bytes, registration state and prism-to-stable-Triangle identity mapping. The area query lazily attaches a real `OwnedKCollisionServer` / `KCollisionResource` so original octree search and geometry methods operate on correctly relocated native records. Original near-parallel classification disables those prisms before computing the original part bounding radius. The original area query then returns the same stable native surface identities used by line/sphere queries, with their actual world vertices/normals, PA row and optional placement provenance.

Geometry-only legacy fixtures omit usable octrees. They continue to support their existing line/sphere consumers; attempting the new area operation with invalid octree bytes raises the resource validation error. No empty-result fallback substitutes for a missing resource or scene owner. All retained source/cache/query allocation happens in a host allocation scope, including a fresh copy of a caller-provided source name. Game callback arena teardown cannot destroy native collision bookkeeping.

The imported original `objectSpaceToLocalSpace` uses the existing Aurora Gekko `truncate_s32` primitive at the native architecture boundary, preserving overflow saturation instead of undefined host float-to-int casts. Native `checkArea3D` uses an unsigned one-bit shift. The rest of the imported checkArea3D/outCheck bodies remain original.

## Source and Wii proof

Read `decomp/AGENT_DECOMP_GUIDE.md`. Missing array methods and predicate were recovered in decomp first. The recovered Game reference TUs are byte-identical in the native tree. Pre-existing native zone methods were preserved and added to the shorter decomp reference; `source-parity.json` records unchanged existing native bodies. `CollisionParts.cpp` remains explicitly excluded from native archive selection because complete original construction still needs camera/CollisionDirector ownership; live area collection uses the actual native collision/resource owners.

An initial whole-TU mirror accidentally dropped pre-existing native-only zone methods and triggered two fixture link failures. This was corrected before checkpoint: all old native methods are restored unchanged, mirrored in reference, and the later fixture and final query builds pass. Those failures were not mistaken for new missing dependencies.

Fresh whole-TU Metrowerks compiles all pass. Against the supplied retail objects:

| Function | Match |
| --- | ---: |
| CollisionCategorizedKeeper::createAreaPolygonListArray | 93.14% |
| CollisionParts::createAreaPolygonListArray | 92.28% |
| CollisionCategorizedKeeper::isSphereOverlappingWithBox | 100% |
| KCollisionServer::checkArea3D (existing original body) | 96.84% |
| KCollisionServer::outCheck | 100% |
| KCollisionServer::objectSpaceToLocalSpace | 100% |
| Existing CollisionZone::calcMinMaxAndRadius | 98.64% |
| Existing CollisionZone::addAndUpdateMinMax | 100% |
| Existing CollisionZone::eraseParts | 86.90% |

The existing erase implementation retains the actual find/swap/decrement behavior; its lower score is from the emitted find loop. The parts array method retains retail's unused copied/zeroed matrix and transforms through the actual inverse matrix. No inline assembly was introduced. `wii-compile-results.json`, `wii-proof.json`, decoded assembly and full objdiff JSON retain reproducible evidence. Initial `*-retail.json` self-diffs are disassembly evidence only, not match proof.

## Native validation

Final `xmake build -vD` and `xmake run` both exit zero for:

- `smg-pc-area-polygon-query-tests`: five cases cover leaf/node traversal, duplicate leaves, negative heights, capacity, AABB corner/collapsed-axis/slab boundaries, numeric zones, swap erase/reappend, dead/released owner filtering, stable identity retirement, transformed geometry, PA retention, host heap escape, oversized/invalid octrees and saturated large coordinate conversion.
- `smg-pc-stage-collision-registration-tests`: both existing empty/explicit registration cases pass, including the geometry-only fixture without a valid octree.

These are deterministic native KCL fixtures, **not real-disc gameplay validation**. The sibling agent also rebuilt the affected real-disc Talk and authored-placement fixtures successfully after the corrected native mirror. There is no claim here that Mario shadow drawing or the Gateway demo has run end-to-end.

`native-tests.json` records exact final commands/results; named build/run logs contain output. `native-syntax.json` records all five production/test translation units passing LLVM 23 syntax checks. `source-manifest.json` hashes final sources. The final EOF whitespace trim was followed by a fresh Wii compile and unchanged semantic proof; no additional runtime rerun was needed for that whitespace-only edit.
