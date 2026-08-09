# Gateway Mario stand/walk milestone — 2026-08-09

## Outcome

The development Gateway route now creates the real `MarioActor`, loads retail
`Mario.arc` and `MarioAnime.arc`, updates it through the normal
`RuntimeContext` scheduler, drives it from the ordinary host-to-WPAD input
path, and submits its opaque/translucent model from the retail
`DrawType_Player` callback.

The route uses the exact scenario-1 StartInfo, start camera, mysterious-planet
BDL/KCL/PA, and child-zone point gravity from RMGK01. The global production
`NameObjFactory` Mario rows remain absent; the bounded showcase owns Mario with
the exact typed creator and clears `MarioHolder` before destruction.

Visual evidence: `gateway-mario-walk.png` is a 960x720 display-copy capture at
frame 60 while the real RuntimeContext input script held UP. It visibly shows
the retail Mario model in its Run BCK on the retail Gateway planet.

## Runtime proof

The focused real-disc test proves all of the following in one route:

- exact Gateway scene collision and scene-owned body-sensor provenance;
- real point gravity and exact NoSlip/Lawn KCL attribute decoding;
- exactly one RuntimeContext movement, calc/view, and DrawType pass per frame;
- host input sampled before scheduler execution;
- stable Wait, grounded stick walk across the real convex KCL seam, and Run;
- 325.068 units of materially tangential travel with 12 Mario packets;
- Run retained during release inertia, followed by Wait at physical rest;
- 60 further zero-input, zero-bias grounded idle frames;
- finite, orthonormal, ground-aligned Mario model matrices;
- ordered scheduler/MarioHolder teardown and successful actor recreation.

The generalized collision fix keeps public exact-radius queries unchanged.
Binder-only motion queries use the retail 1.2-unit contact shell plus a 0.01
large-coordinate numerical tolerance. Simultaneous host KCL planes are resolved
with the minimum-norm half-space correction; retail component extrema remains
the fallback for infeasible or degenerate manifolds. Synthetic tests cover a
10-degree convex seam, a Gateway-scale one-ULP shell, and the eight contact
normals captured from the real stopping frame.

## Verification

From `pc-port`:

```sh
xmake build -y -j2 smg-pc-binder-kcl-mario-walk-tests
xmake run smg-pc-binder-kcl-mario-walk-tests

xmake build -y -j2 smg-pc-mario-gateway-walk-tests
env SMGPC_ENABLE_VSYNC=0 \
    SMGPC_REAL_DISC=/workspaces/pcport/RMGK01.iso \
    xvfb-run -a xmake run smg-pc-mario-gateway-walk-tests

xmake build -y -j2 smg-pc-stage-collision-registration-tests
xmake run smg-pc-stage-collision-registration-tests

xmake build -y -j2 smg-pc-gateway-demo-scene-tests
env SMGPC_REAL_DISC=/workspaces/pcport/RMGK01.iso \
    xvfb-run -a xmake run smg-pc-gateway-demo-scene-tests

xmake build -y -j2 smg-pc-showcase
env SMGPC_ENABLE_VSYNC=0 \
    xvfb-run -a xmake run smg-pc-showcase gateway \
    --disc /workspaces/pcport/RMGK01.iso --smoke --max-frames 360
```

All passed. Showcase smoke closed after 28 rendered frames and proved same-frame
animated Mario packets with an on-screen actor center, GPU submission, point
gravity acceleration, and exact planet KCL contact.

The visual capture command was:

```sh
env SMGPC_ENABLE_VSYNC=0 SMGPC_DEBUG_WPAD_BUTTON_SCRIPT=20-80:UP \
    xvfb-run -a xmake run smg-pc-showcase gateway \
    --disc /workspaces/pcport/RMGK01.iso \
    --screenshot notes/mario-gateway-walk-20260809T080016Z/gateway-mario-walk.png \
    --screenshot-frame 60 --exit-after-screenshot --max-frames 90
```

## Deliberately deferred

This is a stand/walk slice, not a claim that all Player states are linked.
Unsupported eager Wii/audio/alternate-form state modules remain excluded and
fail explicitly if reached. Audio remains deferred. The next bounded milestone
is the authored Gateway spin-unlock checkpoint: retain the real prompt/event
timing and prove swing input is rejected before the flag and accepted after it.
Visible retail spin action/animation is a separate later closure.
