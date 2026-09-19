# Collision test ownership migration

The retired targets `smg-pc-binder-kcl-mario-walk-tests` and
`smg-pc-collision-triangle-filter-tests` used native `add_kcl` registrations as
though they were original CollisionParts/Keeper entries. Their synthetic KCL
also omitted a valid original octree and used edge axes accepted by the old
host reconstruction rather than the original query. Their previous passes
cannot validate the restored sphere pipeline. Complete pre-migration sources
are retained as `BinderKclMarioWalkTests.cpp.before.gz` and
`CollisionTriangleFilterTests.cpp.before.gz`.

| Previous assertion family | Current owner or replacement | Boundary |
| --- | --- | --- |
| Margin only after actual penetration; no-margin flag; skip initial contact | `OriginalBinderSphereTests::margin_and_initial_step` | Real original Director/Keeper/Parts/Binder, synthetic valid KCL |
| Floor/wall/roof caches and temporary/allocated plane storage | `classification_and_storage` | Original query transforms and Binder predicates |
| Rejected geometry does not push, accepted later parts survive | `filtering_and_capacity`, `rejected_parts_do_not_consume_hit_slots` | Real part pointers, no fabricated actor registry |
| Candidate versus accepted capacity and KCL leaf ordering | `OriginalSphereQueryTests::encounter_and_filter_capacity` | Original candidate capacity can be consumed before per-triangle filtering within one part; the old broad "filter before capacity" assumption was too strong |
| Nonorthogonal/opposing reaction range and moving-reaction flag | `reaction_range_and_moving_parts`, duplicate-contact case | Calls actual original reaction method; independent fixed expected vectors |
| Raw matrix offset, zero-radius binding, duplicate contacts | `offsets_and_duplicate_contacts` | Original Binder and original sphere queries |
| Full plane array still stops an unstored projected retry | `full_plane_array_still_stops_projected_retry` | Original 35-unit stepping and capacity behavior |
| Derived actor sees fresh Binder before scheduler returns; no second integration | `derived_movement_observes_same_frame_binding` | Real scene execution fixture and registered original actor |
| Public dead/no-bind calls versus normal movement death gate; nerve/displacement step | `OriginalActorUtilityTests` | Existing real original SceneController/Clipping owners |
| Actual player executes once per observed process frame | `OriginalProcessPlayerOwnerTests` cadence observation | Actual disc/process/Mario owner, read-only |
| Transformed geometry/feature identity, scale-thickness contract, moving sweep | `OriginalSphereQueryTests` | Original source geometry and world-space query semantics; not native host BVH behavior |
| Owner name separate from resource, retained zone, rename/null sensor host, deactivate/reactivate/release/clear | Moved to `StageCollisionRegistrationTests::test_owner_name_and_zone_remain_separate_from_resources` | Direct native line query and native storage adapter; explicitly host metadata coverage |
| Source matrix identity and lifetime, generated geometry publication/quarantine/retirement | Existing `StageCollisionRegistrationTests` retained | Direct host storage and original generated writer under actual registration owners |
| Independent native line/sphere cache loading, edge/tangent/attribute behavior | `AuroraNativeTests::test_host_kcl_collision_service_queries` retained | Host service only, no claim it supplies original Keeper membership |

The old convex-seam fixture's exact sampled route, nearest-line actor-filter
selection, three-source sensor pointer sorting, and .pa MarioMapCode interaction
are not fully reproduced by the new bounded sphere test. They remain explicit
coverage gaps, not silently claimed passes. General line/filter suites and
actual process player/KCL checks remain available independently. The old
negative/zero model-scale tests were model-less approximations; the replacement
checks the actual Binder raw matrix contract, while full model-scale behavior
requires an actual J3D owner.

`StageCollisionService::move_sphere` had no production callers. Its response
result structure and implementation are removed, rather than retained as a
parallel collision algorithm or taught to manufacture original membership.
The absence and heap-routing cases in StageCollisionRegistrationTests now call
the service they actually construct; they no longer call original MR queries
while holding only a native host registry.

No fresh build/run has been made at the time of this migration note. Pending
results must be recorded before calling the replacement coverage passing.
