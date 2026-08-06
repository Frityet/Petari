# GX IA8 channel-order correction

## Outcome

GX IA8 textures now decode their two source bytes as alpha followed by
intensity. This removes the opaque black rectangles that surrounded the
`mr_steam00_ia` particles in the HeavensDoor capture. The correction is in the
general TPL/GX texture decoder; it does not depend on the stage, actor, effect,
or route.

## Dolphin and disc evidence

The local Dolphin clone was used as the format oracle. Both
`VideoCommon/TextureDecoder_Generic.cpp::DecodePixel_IA8` and the x64 decoder
interpret an IA8 word in host memory as alpha in the first source byte and
intensity in the second. The PC decoder read the big-endian word correctly,
but then assigned its high byte to intensity and its low byte to alpha.

The affected texture was identified from the real RMGK02
`ParticleData/Effect.arc` / `particles.jpc` data as `mr_steam00_ia`, a 64x64
IA8 texture. Interpreting its raw samples with the Dolphin ordering produced:

- intensity range: 93 through 245;
- alpha range: 0 through 246;
- 801 completely transparent texels;
- corner sample: intensity 245, alpha 0; and
- center sample: intensity 201, alpha 246.

Those values describe a bright steam sprite with a transparent surround. The
old ordering instead turned the bright transparent border into dark opaque
pixels, which exactly explains the captured rectangles.

## Code and regression coverage

Commit `266073a44` swaps the IA8 assignments in
`src/resource/TplTexture.cpp`. The native regression test feeds the decoder a
source sample `{0x12, 0x34}` and requires RGBA
`{0x34, 0x34, 0x34, 0x12}`. The complete native suite passed 23/23 after the
change.

## Visual evidence

- [Before the correction](screenshots/before-ia8-channel-fix.png): transparent
  parts of every steam sprite appear as opaque black rectangles.
- [After the correction](screenshots/after-ia8-channel-fix.png): the rectangular
  surrounds are transparent and the steam gradients remain visible.

The after capture is from the complete title, file-select, five-page
picturebook, and HeavensDoor scripted route. It also shows the compatibility
player at the crater after movement and a core-swing input.

## Separate remaining particle gap

The corrected sprites are still stacked near the screen center. The current
non-child JPC path emits screen-space `TexturedQuad2D` packets instead of
camera-facing world billboards. That is a distinct, generalized JPC rendering
gap; it is not an IA8 decoding defect and was intentionally left out of this
format correction.
