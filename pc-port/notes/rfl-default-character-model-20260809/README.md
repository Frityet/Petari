# Default Guest A RFL render proof — 2026-08-09

This ignored evidence bundle records a direct, isolated render of the retail
default-Mii CharacterModel implementation in Aurora. It is not a fallback,
diagnostic shape, or route-specific substitute. The model is built from the
decompiled RVLFaceLib algorithms, the six retail 74-byte default records, and
the real `RFL_Res.dat` extracted from `MiiFaceDatabase.arc`.

## Inputs

- `MiiFaceDatabase.arc`: 209,120-byte RARC in the RMGK01 extraction
  (`sha256 7203c3172941e321433477d2f118d6cda5ff94ac4c8b18e3697714864bcfc313`)
- `RFL_Res.dat`: 686,372 bytes, version `0x039d`, 18 subarchives
  (`sha256 7af8e9389f27b7adefb05d883c1bef76865a30d026d9754640c6413f56417223`)
- default record: Guest A, index 0
- mask resolution: `RFLResolution_256`
- generated expressions: `RFLExp_Normal | RFLExp_Blink`

## Render evidence

- `default-guest-a-normal.png`: complete retail face, hair, eyes, brows, nose,
  mouth, and faceline; 640x456
- `default-guest-a-blink.png`: complete model with the real closed-eye mask;
  640x456
- `default-guest-a-normal-repeat.png`: normal selected again after blink;
  pixel-identical to the first normal image
- `default-guest-a-normal-blink-normal.png`: fresh uniquely named triptych used
  for visual inspection, avoiding image-viewer caching of overwritten paths

PNG SHA-256:

```text
8edd8afa8e063d4f5ccccade8c8a047d230adfeaacae92ab3659c1005dd1f652  default-guest-a-blink.png
bdb87360891493a7d04eb0d6b4eb8f51f37e995c85bc435e2d25d074ab15ef85  default-guest-a-normal-blink-normal.png
57308d42d482590eb5d226b0fccb1513015b04c36dd98f466af76eae70dde3da  default-guest-a-normal-repeat.png
57308d42d482590eb5d226b0fccb1513015b04c36dd98f466af76eae70dde3da  default-guest-a-normal.png
```

Five runs with separate fresh `XDG_DATA_HOME` directories produced the same
raw display-copy statistics and hashes every time:

```text
normal        nonzero-rgb=39549 nonzero-alpha=40700 opaque=35825 fnv1a=0x70dd7d3fe211aff9
blink         nonzero-rgb=40455 nonzero-alpha=40700 opaque=36785 fnv1a=0xcc2cccd8eaaae6ad
normal-repeat nonzero-rgb=39549 nonzero-alpha=40700 opaque=35825 fnv1a=0x70dd7d3fe211aff9
```

The direct test requires exact RGBA equality between normal and
normal-repeat. Normal versus blink changes 11,506 of 1,167,360 RGBA bytes;
every changed pixel is inside the conservative eye region. An independent RGB
difference calculation measured the changed bounding box as exactly
`142x38+249+203`; hair, faceline, brows outside the eye raster overlap, nose,
and mouth are unchanged.

The earlier apparent alternating/incomplete image was an evidence-viewer cache
artifact caused by inspecting an overwritten path. A separate real issue in
the first version of the test was differing EFB clear alpha between the
model-construction frame and later frames. The proof now applies one transparent
GX copy-clear value to every frame before rendering; no CharacterModel behavior
was changed to resolve it.

## Generalized GX prerequisites

The model exercises three generalized Aurora fixes, independently tested without
RFL:

1. `GXSetZScaleOffset` follows retail deferred dirty-state behavior. The setter
   emits no FIFO bytes; the next viewport/dirty flush emits adjusted `sz` and
   `oz`, and a later flush emits nothing. Aurora requires WebGPU
   `DepthClipControl` and `ClipDistances`: pre-remap GX near/far clipping is
   preserved while Mario's nonzero depth-range remap remains unclipped.
   `GX_CLIP_DISABLE` is explicitly unavailable rather than approximated.
2. `GXCopyTex` and `GXCopyDisp` are FIFO ordering/completion boundaries. They
   drain queued GX commands and wait only for pipelines used by a copied pass.
   Five independent cold-cache runs produced:

```text
draw -> GXCopyTex -> sample -> GXCopyDisp  fnv1a=0x79a06d1c07c3db25
red -> green -> GXCopyDisp                 center=64,128,0,159 fnv1a=0xbda95ea81d584325
```

3. Texture, TLUT, and copy-texture destruction now snapshots stable identities
   into the normal GX FIFO whenever GX is alive, preserving load/destroy order
   between frames. Late calls after GX shutdown are safely ignored. The final
   GX/destruction test set passes 173/173 cases, and the real Vulkan depth proof
   covers originally out-of-range near/far geometry as well as Mario's
   `(scale=1, offset=0.00001)` path.

Published Aurora commits:

```text
f07c8db  Make GX depth and resource lifetime exact
dcb5221  Add retail default character model foundation
```

`CharacterModel` and its validated resource parser are now compiled into
Aurora. The production RVLFaceLib bridge remains intentionally disconnected:
the exact Game-facing C API/service registry, `RFLCharModel` replacement and
ownership rules, and exact Mii holder lifecycle are not yet wired. Therefore
this proves a real renderer foundation, not an activated FileSelector or Mii
runtime.
