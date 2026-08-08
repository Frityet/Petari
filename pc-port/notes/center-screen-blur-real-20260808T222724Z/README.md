# Center-screen blur: real GPU closure

Date: 2026-08-08 UTC

## Outcome

`CenterScreenBlur` is an exact Game-source mirror and now runs through a real,
scene-owned captured-frame blur service. The compatibility implementation uses
Aurora's GPU `GXCopyTex` result, a real 128x64 offscreen framebuffer, and a real
history texture. Missing scene ownership, capture state, or active GPU state is
a hard diagnostic; there is no event-only, no-op, or synthetic-image fallback.

`FullScreenBlur.cpp` is also retained as an exact source mirror, but it is
intentionally excluded from the PC build. Retail renders its history scratch
image into otherwise hidden EFB storage and then captures a 128x64 rectangle at
`(0, 464)`, which reaches row 527. Aurora's logical Wii EFB is deliberately
640x456, so executing that source literally would address storage that does not
exist. The generalized compatibility service preserves the effect with an
actual offscreen target instead of emulating those rows or silently dropping
the pass.

The excluded retail translation unit is nevertheless syntax-compiled unchanged
by `smg-pc-file-select-exact-source-compile`. Exact `Color.hpp` and
`DirectDraw.hpp` mirrors plus Aurora's generalized `revolution/gx.h` umbrella
close its header surface; a compile-target-only declaration supplies the
already-retail `MR::nonFilteredCapture` signature without creating a production
FullScreenBlur provider.

The PC draw schedule now matches the relevant retail ordering:

1. `DrawType_CenterScreenBlur`
2. `DrawType_CaptureScreenIndirect`

This means the visible blur samples the previous completed capture and the
indirect capture then records the resulting frame for the next frame.

## Source boundary

The following PC files are byte-identical to the root decomp sources and are
covered by `GameSourceMirrorTests.cpp`:

- `Game/Screen/CenterScreenBlur.hpp`
- `Game/Screen/CenterScreenBlur.cpp`
- `Game/Screen/FullScreenBlur.hpp`
- `Game/Screen/FullScreenBlur.cpp`
- `Game/Util/Color.hpp`
- `Game/Util/DirectDraw.hpp`

The RMGK02 object report recorded 100% fuzzy match for every emitted section of
both translation units, including `.text` (Center: 928 bytes; Full: 656 bytes).
The project configuration still labels these units `NonMatching`; the recorded
object comparison, rather than that stale label, is the evidence used here.

## Active-frame GPU proof

The focused suite was run under Xvfb with Aurora's Vulkan backend on an NVIDIA
GeForce RTX 3080. It performed three GPU frames:

1. draw an asymmetric source image and materialize the real
   `CaptureScreenIndirect` `GXCopyTex`;
2. run the exact Center actor's current-frame blur and resolve the first 128x64
   history copy;
3. run a second Center draw that samples the history texture and refreshes it.

At the second blur draw:

- blur draw count: 2
- history capture count: 2
- history valid: true
- `AuroraHasTextureCopy(history.mImage)`: true
- changed display bytes: 875,169 / 1,228,800 (71.22%)

The scene was then destroyed, an empty Aurora frame drained the GX teardown
commands, and `AuroraHasTextureCopy` returned false for the former history
address. `JUTTexture` now releases both its GX texture object and any captured
copy before freeing capture storage, preventing stale GPU content from being
reused at a recycled host address.

Visual readback:

- [unblurred baseline](baseline-frame.png)
- [captured-frame blur](blurred-frame.png)

The screenshot comparison shows the real expanded, history-backed image rather
than a debug overlay. SHA-256 values and exact commands are in
`verification.log`.

## Honest absence diagnostics

The focused tests also prove that:

- creation without a scene-owned `SceneObjHolder` throws;
- start before creation throws;
- draw without the scene-owned blur service throws;
- an appeared actor without the real `CaptureScreenDirector` texture throws;
- Aurora reports no active frame and no completed copy outside real GPU work.
- scene destruction evicts the captured history texture from Aurora.

This keeps the production behavior binary: the real captured-frame path runs,
or the missing dependency is reported precisely.
