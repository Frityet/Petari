# Generalized stage-player compatibility slice

Date: 2026-08-06 UTC

## Outcome

The PC port now creates a compatibility-owned Mario actor from the selected
`StartInfo` for any stage host with valid start data. The actor uses the
existing scheduler, binder, arbitrary-gravity service, stage camera, input,
hit sensors, renderer, and original Mario/MarioAnime archives. No Gateway or
route name is used by the player implementation.

The preserved front-end and both opening branches have been exercised:

1. title
2. file select
3. five-page picturebook
4. either the debug picturebook-to-HeavensDoor handoff, or the normal Peach
   letter and `DemoPeachCastleGate` arrival
5. a rendered, controllable stage player restored to `Wait`

Representative captures:

- [title](screenshots/title.png)
- [file select](screenshots/file-select.png)
- [picturebook](screenshots/picturebook.png)
- [normal opening completed in Peach Castle Garden](screenshots/opening-arrival-complete.png)
- [Gateway player](screenshots/gateway-player.png)
- [Gateway swing/input proof](screenshots/gateway-spin.png)

## Source-closeness boundary

`pc-port/src/Game/Util/DemoUtil.{cpp,hpp}` and
`pc-port/src/Game/Util/PlayerUtil.{cpp,hpp}` are byte-for-byte copies of the
root decompiled sources and are excluded from the host build. Their currently
needed symbols live in `src/compat/DemoUtilCompat`, `PlayerUtilCompat`, and
`PlayerStateCompat` instead.

The new player itself is also compatibility-owned (`StagePlayerRuntime`), so
it does not pretend to be a reconstructed `MarioActor`. The only additional
hook in the already host-adapted `Game/LiveActor/LiveActor.cpp` registers its
private host model with the generic actor runtime registry. Camera resolution,
explicit-root stage setup, player lighting, and scheduler draw interleaving
are all data/category driven.

The audit recorded:

- 271 `pc-port/src/Game` files inspected
- 69 exact-source files
- 8 compile-only files
- 192 compatibility-temporary files
- 2 decomp-needed files
- all four `DemoUtil`/`PlayerUtil` source/header files classified
  `exact-source`

See [source-closeness-summary.md](artifacts/source-closeness-summary.md).

## Player and demo behavior implemented

- Mario model plus separate `MarioAnime` archive lookup
- named BCK loading and actor-local start frames
- original J3D attributes for once, reset, loop, reverse, and loop-reverse;
  BTK retains its independent renderer timeline
- `Wait.bck` loop (`attribute=2`, `frame_max=180`)
- `DemoPeachCastleGate.bck` once (`attribute=0`, `frame_max=299`)
- camera-relative Nunchuk movement, jump, arbitrary gravity, binder contact,
  and a follow camera derived from resolved start-camera data
- keyboard `X` as core swing; scripted `ONE` remains a deterministic shake
  stand-in
- player body/spin sensors and original player-punch message
- full external 3x4 puppet matrices, not translation-only overrides
- puppetable demo control ownership and restoration
- actor-owned demo state automatically released during actor/scene teardown
- opening-demo teardown restores `Wait`, control, position, zero velocity, and
  clears transient binder/matrix ownership
- original draw order: normal opaque, player light and `DrawType_Player`, then
  normal translucent
- all eight GX light registers initialized through the existing
  `LightFunction::initLightRegisterAll` scene contract
- explicit-root scenes share normal placement/collision/gravity/player setup;
  the explicit object is deduplicated from the matching placement by data
  identity

## Automated evidence

Focused real-disc player tests:

```text
[ok] basis tracks arbitrary gravity
[ok] camera-relative input is tangent
[ok] velocity handles ground, jump, and terminal speed
[ok] animation playback modes honor local lifecycle
[ok] follow camera preserves pose and follows translation
[ok] player service synchronizes actor state
[ok] wpad swing edge is frame-stable
[ok] optional real-disc player resources
8 stage-player runtime test(s) passed
```

Other verification:

```text
23 Aurora-native test(s) passed
4 stage-start camera test(s) passed (including real-disc HeavensDoor camera)
xmake build smg-pc: passed
root RMGK02 ninja: no work to do
git diff --check: passed
```

The new `opening_arrival_complete` route-smoke case runs the normal
title/file-select/picturebook/letter/arrival path to frame 10550 and validates
the semantic ordering, single PrologueDirector construction, BCK resource,
control teardown, stage placements, Mario packets, requested lights, and
garden models. Its result was:

```text
status=passed
render_packets=377
semantic_events=1645
nonblack_ratio=0.999805
PeachCastleGardenGalaxy placements=230 created=151 blocked=24 ignored=55
Mario packets=12; BCK frame_max=180; unsatisfied light mask=0
```

Artifacts:

- [opening route manifest](artifacts/opening-arrival-manifest.json)
- [opening semantic/model expectations](artifacts/opening-arrival-expectations.json)
- [opening trace validator log](artifacts/opening-arrival-trace-validator.log)
- [Gateway route manifest](artifacts/gateway-manifest.json)
- [Gateway trace validator log](artifacts/gateway-trace-validator.log)

The Gateway handoff result at frame 10350 had 585 render packets, 12 Mario
packets, requested light mask 23, loaded mask 255, and no unsatisfied requested
light bits. The trace records `demo_owner_released` for PrologueDirector before
the HeavensDoor player is created, preventing stale picturebook state from
blocking `DemoRabbit::exeGoal`.

## Screenshot SHA-256

```text
9b2f57887d8644b5ad8f714d9436a0be55038373dc05c27e126eac58a2320f23  title.png
f100e44af973418fa5140837b06f83b347b69d50cf035fa1ab015971756fa7c8  file-select.png
e145ba374696c0cf89cef835b32518ba3a18860f3ad9810642219003053f6196  picturebook.png
1920a2fc2874583d33ddcb4de5086ab7bc12e458bb9b1dd8fec316523028ba8d  opening-arrival-complete.png
132a46948ca3e1a695375f719a747677d0438913467263ef48a5f71d2b741827  gateway-player.png
a97e36d9ff77920c49bb53315d71c7f9e9521d246326d1fcf2519767eb98ed9f  gateway-spin.png
```

## Known next boundary

The IA8 steam texture channel order is fixed, but non-child JPC particles are
still submitted as screen-space `TexturedQuad2D` packets. That makes otherwise
transparent steam sprites stack near screen center. The decompiled original
oracle is `JPADrawBillboard` in
`src/JSystem/JParticle/JPABaseShape.cpp`: it transforms each world particle
center through the camera matrix and draws a camera-facing quad there. This is
a generalized renderer gap and should be fixed in the JPC compatibility path,
not with an HeavensDoor/effect-name exception.

Spin input is present for player-boundary testing, but faithful story
entitlement is a later Gateway event slice: a fresh file must keep spin locked
until the original story flag/InformationObserver flow enables it.
