# Live BlackHole / Small targets

**Later evidence:** the direct799→806 nearest-first leg fell across the crater rim. Keep these actor bindings, but use `crater-detour.md` for the replacement candidate surface route. The original point-clearance heuristic did not establish terrain safety.

Bound to latest complete trace snapshot **frame 11970**, binary `a0a0912c63f60d8197f2aae7142714eb71bfb9f0ba814f750c31f6b82a78b25b`. Only the last 1 MB of `route-actors.jsonl` was read; no game input or memory modification. Positions below are exact observed values rounded to three decimals, with full values and match distances in `live-targets.json`. Static position matches differ from the authored calculation by less than one unit. The inactive checker/key are bound by unique type/name and actual child-construction code, not by their zero coordinates.

| Authored placement | Live ID | Observed position | Snapshot state |
| --- | --- | --- | --- |
| BlackHole YellowChip 16 | 793 | `(13587.924, -1989.661, -5968.262)` | alive |
| BlackHole YellowChip 17 | 799 | `(13917.750, -3029.464, -4070.988)` | dead/inactive |
| BlackHole YellowChip 18 | 803 | `(13044.479, -877.463, -4618.346)` | alive |
| BlackHole YellowChip 19 | 806 | `(13085.542, -1389.111, -3784.093)` | alive |
| BlackHole YellowChip 20 | 809 | `(14871.933, -1200.611, -4756.813)` | alive |
| BlackHole CrystalCageS 29 | 1004 | `(12908.613, -2329.049, -4305.976)` | alive |
| BlackHole CrystalCageS 30 | 1007 | `(13915.051, -2896.996, -4075.312)` | alive |
| BlackHole CrystalCageS 32 | 1010 | `(13753.385, -2119.626, -3469.261)` | alive |
| BlackHole Tico 25 | 787 | `(13586.588, -2321.229, -3515.312)` | alive |
| Root SuperSpinDriver 8 | 871 | `(12958.847, -830.847, -4648.078)` | dead/inactive |
| Small Tico 9 | 757 | `(6680.000, 2925.405, -5310.000)` | alive |
| Small CapsuleCage 16 | 1036 | `(6680.000, 2643.584, -5310.000)` | alive |
| Small ChildKuribo 3 | 1135 | `(7112.836, 1852.793, -6205.965)` | dead/inactive |
| Small ExterminationKuriboKeySwitch 21 | 1133 | `(0.000, 0.000, 0.000)` | dead/inactive |
| Small owned KeySwitch | 1137 | `(0.000, 0.000, 0.000)` | dead/inactive |
| Root SpinDriver 2 | 1056 | `(6704.375, 2943.039, -5296.367)` | dead/inactive |

## Nearest-first chip route

From first launch path endpoint `(13436.447,-2651.243,-3706.727)`, approach **cage 1007** (~652 units), hit it, then collect **chip 799** after its initial 40-step pickup immunity. Its `dead` state in this snapshot means not yet appeared, not already collected.

Then visit **806 → 803 → 793 → 809**, wait for completion/outro, and approach **launch 871** after it appears. This is a greedy nearest-next order from observed positions, not a shortest overall tour. Straight distances between targets are approximately 799→806 1862, 806→803 980, 803→793 1832, 793→809 1934, and 809→871 1952 units.

Use tangent surface steering and the actual selected gravity; do not try to move along world-space chords through the planet. The largest greedy turn is 799→806 (~98 degrees); optional **cage 1004** provides a real surface-side waypoint for that leg, without needing to break it. Keep away from the authored black-hole center `(13955.342,-1910.785,-4524.826)` and its 500-unit eye sensor. The route's straight segment center clearances are all over 800 units, but that is only a point-distance calculation: it does not validate terrain, crater edges, Mario's volume, or meteor collisions. BlackHole itself is omitted from this run's trace filter, so no live hazard actor ID is claimed.

## Small encounter

**Tico 757 → owned Goomba 1135 → key 1137 → cage 1036 opens → freed Tico 757 → launch 1056.**

Tico dialogue's B1124 activates the owned encounter; confirm its live state. Stomp/hip-drop the actual Goomba or use the original spin-then-kick behavior; one spin is not proof of defeat. Checker1133 owns key1137 (`全滅用キースイッチ`) and spawns it at the last child's position. Follow key1137 once alive, allowing its first60 Appear steps before pickup. Both key1137 and checker1133 currently have zero positions; neither is a navigation target at the origin. **Key1126 is the other encounter's key**, not the Small owned key.

Key pickup writes1125; wait for capsule1036's original opening. Freed Tico's metamorphosis writes1127 and releases driver1056. Static cage/Tico positions help approach, but use live tracking after Tico/key/Goomba movement. Details of the authored switch chain and source evidence remain in `../gateway-launch-traversal-20260925/blackhole-small-route.md`.

This is a target binding and route suggestion, not evidence that these interactions have succeeded in the current run. Actor IDs apply to this process only; no builds or tests were run.
