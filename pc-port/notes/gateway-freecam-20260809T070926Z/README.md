# Gateway freecam and physics-probe demo

## Outcome

The standalone `smg-pc-showcase gateway` route renders the exact
`HeavensDoorMysteriousPlanet` BDL from RMGK01 and starts in the existing
RuntimeContext free camera. It does not enable the production Mario factory or
weaken StageHost placement policy.

The route also owns explicit development physics probes. Pressing `=` or
numpad `+` creates a visible UV sphere in front of the camera. Each probe:

- queries the exact child-zone `GlobalPointGravity` through the normal gravity
  manager;
- integrates the returned gravity vector into its velocity;
- moves through `StageCollisionService::move_sphere` against the exact planet
  KCL;
- rejects contacts without the expected KCL source and PA attribute bytes;
- changes from blue to green after its first verified real-KCL contact.

The generated sphere is deliberately identified as a development probe, not a
retail Game actor.

## Launch

```sh
cd /workspaces/pcport/pc-port
xmake build -y -j2 smg-pc-showcase
xmake run smg-pc-showcase gateway --disc "$PWD/../RMGK01.iso"
```

Controls: mouse look, WASD, Space/LeftShift, F9 mouse-capture toggle,
`=`/numpad `+` spawn probe, Esc quit.

## Verification

```sh
env SMGPC_ENABLE_VSYNC=0 xvfb-run -a \
  xmake run smg-pc-showcase gateway \
  --disc /workspaces/pcport/RMGK01.iso --smoke
```

The final smoke run exited successfully at frame 22 after proving:

- 17 renderable retail J3D meshes;
- 7,789 exact planet KCL triangles;
- active GPU draw submission (18 draws after cold pipeline compilation);
- gravity changed the probe velocity;
- exact source
  `HeavensDoorMysteriousPlanet.arc/heavensdoormysteriousplanet.kcl` was retained;
- the probe contacted real KCL triangle 4642.

Screenshot evidence is retained outside this ignored note at
`pc-port/demo-report/evidence/gateway-freecam-smoke.png`.

## Published commits

- Aurora upstream merge: `8b873fd1675f37bcc8f4e9ee14179d32321eebdd`
- Parent Aurora/xmake update: `f1b4df625`
- Mario render/KCL foundation: `1bda03fae`
- Exact Gateway subset scene: `c2df7d6d8`
- Interactive freecam/physics route: `f1e562b0e`

## Deferred

The interactive route currently demonstrates the exact environment and
generalized physics independently. The retained Mario constructor/walk slice
is being integrated separately and remains factory-absent until the same-scene
grounding, input, animation, rendering, teardown, and recreate gates pass.
