# Exact MessageArea AreaObj closure

This closure mirrors `MessageArea.{hpp,cpp}` byte-identically from the
decompiled source into `pc-port/src/Game/AreaObj`; no host policy or fallback
behavior was added under `Game/`.

The generalized AreaObj registry now exposes the two RMGK01 factory names:

- `MessageAreaCube` (`AreaForm::Type_Cube2`)
- `MessageAreaCylinder` (`AreaForm::Type_Cylinder`)

Both descriptors share the retail `MessageArea` manager at table order 42 with
capacity `0x10` and no finalizer.

The real-disc fixture constructs the two rows present in
`HeavensDoorGalaxy` scenario 1 through the exact JMap initialization path:

- `MessageAreaCube`, `HeavensDoorSmallZone`, common `areaobjinfo` row 1,
  `l_id=1`, `Obj_arg0=0`
- `MessageAreaCylinder`, `HeavensDoorMysteriousZone`, common `areaobjinfo` row
  3, `l_id=14`, `Obj_arg0=1`

Both rows have absent switches and placement metadata (`-1`), and all
remaining object arguments are `-1`. The integrated focused suite passes
10/10, the neighboring NameObj placement suite passes 2/2, and strict stage
construction still fails honestly with 187 unsupported placements. The two
MessageArea rows account exactly for the reduction from the CubeCamera-only
frontier of 189; the first unsupported object remains `RestartCube`.

See `verification.log` for exact hashes, commands, and results.
