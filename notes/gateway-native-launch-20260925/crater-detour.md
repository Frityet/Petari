# Grounded detour after chip 799

**The earlier nearest-first direct 799→806 recommendation failed.** Its point clearance from the black hole did not account for the crater opening. Keep the live actor-ID bindings, but replace that leg with the KCL-supported detour below. This is route planning only; no production change, game input, build or test was performed.

## Where the run fell

- Chip799 reached Got at frame25340. Mario was still grounded at25360 near `(13855.2,-2907.1,-4095.7)` on host1142/prism2157.
- Travel toward806 passed near Tico787 and entered MarioTalk at25910, at `(13453.4,-2498.6,-3764.5)`. Dialogue had ended by26460.
- Last sampled grounded position: **26520**, `(13285.1,-2114.2,-3689.3)`, prism681. First sampled ungrounded: **26530**, `(13257.8,-2034.5,-3693.8)`. The crossing therefore occurred in this ten-frame interval.
- At loss of ground, Mario remained about1092units from the black-hole center. He then fell inward: distance605 at26560,473 at26570, effectively at the center by26770. This establishes a terrain-edge fall before entering the 500-unit eye sensor region.
- `ground_triangle` remains host1142/prism681 throughout the fall. A nonnull/stable triangle is **not** current-ground proof. The actual movement grounded bit `0x40000000` cleared at26530 and stayed clear; use it with changing position/contacts when supervising short movements.

## Surface detour toward chip806

Stay on the **positive-X / positive-Z side, near cage1010**, until north of the crater, then turn toward806. The failed trajectory moved west (decreasing X) too early. The optional cage1004 heuristic from the earlier note is not the preferred detour: the computed mesh path through it is longer and then has to return around the same opening.

The route follows59 adjacent original KCL faces and is about3029units long. Its face-centroid boundary clearance is at least128.6units; this is a planning margin, not proof of movement clearance at every intervening point. The filter keeps outer faces with radius≥1040 and outward-normal/radial dot≥0.75, avoiding the crater interior and steep walls. Actual KCL shared-edge adjacency prevents jumping directly across the gap in the planning graph.

Overview ground targets:

| Step | Ground-face center (world) | KCL prism | Face-center boundary clearance |
| --- | --- | --- | --- |
| 0 | `(13879.1, -2894.8, -4072.3)` | 2157 | 347.7 |
| 1 | `(13744.4, -2750.6, -3882.4)` | 2193 | 304.8 |
| 2 | `(13723.9, -2550.1, -3701.2)` | 2825 | 264.9 |
| 3 | `(13649.4, -2314.5, -3587.6)` | 603 | 364.8 |
| 4 | `(13746.2, -2077.8, -3500.8)` | 594 | 338.8 |
| 5 | `(13796.6, -1813.0, -3482.6)` | 398 | 310.8 |
| 6 | `(13740.6, -1554.4, -3540.3)` | 528 | 261.0 |
| 7 | `(13689.9, -1328.0, -3652.4)` | 450 | 320.1 |
| 8 | `(13488.0, -1286.6, -3752.3)` | 454 | 258.2 |
| 9 | `(13285.7, -1339.5, -3846.7)` | 406 | 225.4 |
| 10 | `(13173.4, -1460.1, -3869.6)` | 491 | 128.6 |

Use short, ordinary-control steps along the **dense59-face sequence** in `terrain/terrain-routes.json` (`routes.806.steps`), checking actual grounding and current terrain. The table is an overview; untested long cuts between distant entries could repeat the problem. These coordinates are ground-face centers, not Mario-center targets or teleport destinations. The final ground point lies below the hovering chip806; observe actual pickup rather than trying to place Mario's feet at the chip model center. Leave cage1010 intact unless ordinary interaction is needed; it is a landmark, not a new gate.

The same conservative graph did not connect the subsequent greedy chip pairs. That can reflect excluded terrain/steep passages or mesh topology; it does not prove those chips require jumping. This bounded pass supplies **only** a candidate detour from799 to806, not an invented safe route over the rest of the planet.

## Mesh evidence

Read-only extraction of `/ObjectData/HeavensDoorBlackHolePlanet.arc` from the local disc into ignored build artifacts. Archive SHA256 `d7da48a8a318f52651548f6fa96f38caf0c4a3d166fe2ef26d05fca91bb02dfb`; KCL SHA256 `d0a78baf9b55180cf702fac971229f9e701c5f0c2401db099afa7569f0d2ecb0`;4352triangles. Decoded using the actual `KCollisionServer::getPos` prism construction, then applied the native quantized zone rotation. Comparison against four observed ground prisms2157,2200,1228,681 reproduces world vertices within0.00048units. Thus the mesh/world transform is grounded in the live trace.

`crater-detour.json` retains compact milestone evidence and waypoint metadata. `terrain/inspect_terrain.py` is a notes-only reproduction script; full extracted archive/geometry are in `build/gateway-native-launch-terrain-20260925/`. The candidate route has not been traversed live and does not validate the separate renderer-prefix crash during restart.
