# Native resource integration checkpoint

The coordinated frozen build contains original archive enumeration, original
BckCtrl, retained JMap string lifetime, typed MAT3/MDL3/VTX1/SHP1/TEX1 resources,
the original material/shape factories and the restored J3DMaterial constructor.
Aurora now consumes original GD texture/palette BP words from retained aligned
MEM1 storage. Original command sizes and patch offsets remain unchanged.

All13 selected targets build, including the showcase. All12 CPU test programs
pass with the supplied disc available. `build-gates.json` records commands and
results; `native-gates.json` records executable hashes and complete test output.
These include actual Mario data: nine materials through all factory modes, nine
shapes/12 groups,22 textures, and the original30-joint/13-envelope tables.
The newly recovered BckCtrl runtime fixture passes all seven groups.

Fresh title and Gateway smoke runs pass, with two and five rendered frames
respectively. `runtime-gates.json` records commands and binary hashes. These are
regression checks on the current runtime; they do not claim that the new complete
model owner or original jumping has been activated. The prior fixed-step timing
change remains in place.

The next work is complete model/animation resource publication through the
actual ResourceHolder constructor, its heap/mutex prerequisites, and the
original CollisionParts/keeper query and floor pipeline. The current walk-only
Mario control path still rejects jumping. No substitute impulse or animation
name override is introduced by this checkpoint.

Component notes contain source correspondence, original-compiler comparisons,
native ownership contracts, and focused validation details:

- `../original-j3d-material-resource-20260903/`
- `../original-j3d-geometry-resource-20260903/`
- `../original-j3d-texture-resource-20260903/`
- `../original-resource-holder-20260903/`
- `../aurora-bp-textures-20260903/`

Only root-first decompilation is added under Game in this checkpoint: the
complete BckCtrl functions/header. Native JMapInfo keeps borrowed strings in
the retained table owner. New J3D and platform adaptation remains outside Game.
