# Aurora Work Needed For The Demo

## Current Aurora Status

Aurora is already a first-class dependency in this branch:

- Root `xmake.lua` enables Aurora GX, CARD, DVD, and includes `aurora`: `xmake.lua:6`, `xmake.lua:24`.
- `smg-pc-render` depends on `aurora-core`, `aurora-gx`, and `aurora-vi`: `src/render/xmake.lua:1`.
- The app binary depends on `aurora-main`: `src/app/xmake.lua:12`.
- Aurora itself advertises SDL3 app, GX/WebGPU, PAD, DVD via nod, CARD, ImGui, and RmlUi support: `aurora/README.md:6`.

So the missing work is not "integrate Aurora at all." It is making Aurora and the PC-port rendering/input/resource bridge complete enough for an original-shaped SMG gameplay stage.

## GX/FIFO And Original J3D Compatibility

The current renderer is a hybrid: `RendererService` initializes Aurora and emits Aurora/GX calls through PC-native renderer abstractions, while `J3dModelRenderer` translates J3D material state into custom draw batches. That may be fine for title/file-select visuals, but HeavensDoor gameplay pushes harder on original J3D/GX behavior.

Needed:

- Decide on one authoritative path: either execute original GX/GD display lists through Aurora, or make the J3D-to-Aurora translator complete enough for original stage objects and actors.
- If using original display lists, handle CP array-base writes. Aurora currently logs that `CP_REG_ARRAYBASE_ID` is unsupported and expects the custom `GX_LOAD_AURORA_ARRAYBASE` command instead.
- Add partial XF matrix/register write support. Aurora currently checks that position, texture, normal, and post-transform texture matrix copies are full writes; original J3D/GD may emit partial updates.
- Verify TEV, fog, indirect texture, projection-map, alpha, blend, and lighting behavior against HeavensDoor assets.

Why this matters for the demo:

- `HeavensDoorDemoObj` uses projected map matrix setup and indirect map object scene connection.
- The route includes transparent/animated NPCs, spot lights, shadows, map objects, and stage effects.
- If the original actor/model path starts emitting J3D/GD display lists, the unsupported CP/XF cases become hard failures rather than visual polish issues.

## Copy, EFB, And Texture Format Gaps

Needed:

- Implement or intentionally emulate `GXSetCopyFilter`; it is currently empty.
- Resolve the `GXCopyTex` destination-alpha TODO.
- Implement or stub with proof the bounding-box APIs if any imported effects/shadows use them.
- Fill texture/depth copy conversion gaps. Aurora has conversion shaders for common color formats, but missing copy conversion pipelines still fatal.
- Implement `C14X2` texture conversion if any route asset uses it; current texture conversion fatal-errors on `GX_TF_C14X2`.
- Audit HeavensDoor object archives for paletted and depth formats before assuming this is safe.

Why this matters for the demo:

- Earlier title/file-select work already pointed at copy/presentation mismatches as a general blocker.
- HeavensDoor map objects and UI/dialogue may depend on copy/fade/framebuffer behavior.
- Stage rendering quality will be difficult to debug if the copy path silently diverges.

## Input And Controller Model

Needed:

- A route-grade WPAD/KPAD model with pointer, A/B, nunchuk analog stick, Z/C, shake/spin, and scriptable deterministic input.
- Proper mapping from host keyboard/gamepad/mouse into that model.
- Frame-script input for proof runs.

Current `RendererService` maps keyboard keys to a small set of digital `InputButton` values. That is enough for menu navigation and not enough for a playable bunny chase.

Aurora's PAD layer supports SDL gamepads and gyro/mouse at the library level, but SMG route code expects Wii Remote/Nunchuk-style runtime state through the PC-port's WPAD/KPAD services. The integration layer needs to translate Aurora/SDL input into those SMG-facing services.

## DVD, Archives, And Resource Semantics

Needed:

- Keep Aurora DVD/nod as the disc backend, but put one coherent resource manager above it.
- Preserve original-like archive lifetime, async/sync behavior, and missing-resource diagnostics.
- Make stage/scenario/object archive loading explicit for the HeavensDoor route.
- Verify all object models, animations, message resources, effect resources, and camera/demo resources load from a real disc image.

Current PC-port has useful pieces: DVD service, RARC/Yaz0/BCSV parsing, archive cache, and placement resolution. The issue is not raw file access. The issue is matching the original game expectations once broad `Game/` code is imported.

## Audio

Aurora does not solve SMG's JAudio/AX layer by itself. The first demo can be silent if the state calls are deterministic, but the route uses many sound and BGM APIs:

- `setStageBGMState`
- `startStageBGM`
- `startSubBGM`
- `stopStageBGM`
- `isPlayingStageBgm`
- limited/level sound calls

Needed for first demo:

- Stateful no-op or event-backed audio service that answers route queries consistently.

Needed for polished demo:

- Real JAudio/stream/sequence playback, mixing, and volume/fade semantics.

## OS, Memory, And Build/ABI Cleanup

Aurora provides OS/memory/time/cache/arena/alloc wrappers, but importing more original `Game/` and JSystem code may expose gaps. Since API/ABI stability is not required:

- Remove temporary ABI shims once the toolchain/build is aligned.
- Prefer original-compatible allocation/heap boundaries over per-call compatibility glue.
- Expect more JKernel/OS surface to appear when Mario, demo, and map-object systems are copied.

## Suggested Aurora Proof Probes

Add small route-focused probes before doing a full gameplay run:

1. **Asset format scan:** enumerate all HeavensDoor/rabbit object archives and report texture formats, copy formats, material TEV counts, indirect stages, and animations.
2. **GX fatal scan:** run representative J3D display lists through Aurora with fatal-on-unsupported enabled and record CP/XF/TEV/copy failures.
3. **Copy smoke:** exercise `GXCopyDisp`, `GXCopyTex`, copy filter, and destination alpha on the same formats used by HeavensDoor assets.
4. **Input smoke:** replay a deterministic nunchuk/pointer/A/B script and verify KPAD/WPAD samples match expected game-facing values.
5. **DVD/resource smoke:** load stage, scenario, object, animation, message, camera, and demo resources for `HeavensDoorGalaxy` scenario 1 from the real disc.
