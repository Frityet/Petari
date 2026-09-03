# Original map query accessors — 2026-09-03

Recovered 20 original MapUtil routines, covering 1,752 retail instruction bytes.
Nineteen routines relocate byte-for-byte to the verified RMGK01 DOL. The shared
first-line-hit selector is 98.66279%; every instruction corresponds after the
two explicit register-allocation permutations recorded in the proof. All branch
destinations, calls, constants, fields, and operations are unchanged.

The recovered group is:

- Category hit count and first-line-hit selection, with sensor and actor filters.
- Map/water line queries, both public filter overloads, excluded actor/sensor
  variants, map-versus-move-limit choice, and normal output.
- All eight Collision namespace entry points: point, ball, moving-reaction ball,
  thickness ball, map line, sunshade line, hit-info access, and hit count.

The selector asks the actual keeper for its original bounded hit list with the
parts filter and **without** a triangle filter. It applies the triangle filter
while choosing the nearest recorded hit. Distance starts at the original
1,000,000; comparison is strictly greater, preserving the first equal-distance
hit. A caller's parts-filter pointer is passed directly into the original
keeper. Sensor/actor exclusions construct the real CollisionPartsFilter objects.
The combined map/move-limit query queries both categories and chooses move limit
on equal distance. Moving-reaction ball passes true; ordinary ball passes false.
Hit-info access returns the actual keeper entry without a new null fallback.

`verify-original.py` uses the configured original Game compiler, checks every
relocation's actual DOL target and constant bytes, and compares the complete
instruction sequences. `compiler-evidence.json` includes all 20 results and the
explicit selector register correspondence. DOL SHA-1 is
`25c5959534b3c21246c6c7e42021b916b41fb578`.

`root.patch` changes only `src/Game/Util/MapUtil.cpp`. The complete root MapUtil
and already existing original CollisionPartsFilter.cpp are copied to
`build/original-map-query-access-20260903/staged`. Both compile under native Game
flags against the staged original collision owners; `native.patch` is the exact
2-file native source delta. No shared build, GPU work, production native edit,
or placed collision runtime is claimed here.

Atomic activation still requires actual camera/zone ownership, quiesced scene
teardown, original point/area/same-host keeper and part methods, and remaining
MapUtil fast/sorted query bodies. `getNearPolyOnLineSort`, `getCameraPolyFast`, and
`getFirstPolyOnLineBFast` are the next coherent query group. They must not retain
the current static StageCollisionService provider when the real keepers become
authoritative. The unchanged root MapUtil has other missing helpers; selecting
the full TU requires the corresponding active-link closure.
