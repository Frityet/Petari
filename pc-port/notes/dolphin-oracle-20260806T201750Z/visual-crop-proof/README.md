# Crop-before-size comparison proof

Captured 2026-08-06 UTC. This verifies the generalized `--crop-first` behavior independently and
through `scripts/render_parity.lua`.

## Synthetic control

`expected-640x480.png` and `actual-640x456.png` deliberately have different purple/blue pixels
outside the shared rectangle `80,80,320,200`, but are byte-equivalent after that rectangle is
extracted. ImageMagick was used only to create these test fixtures; neither the visual-diff binary
nor the parity runner depends on it.

The existing behavior remains available and rejects unequal source dimensions before doing any
RMS calculation:

```text
$ smg-pc-visual-diff --crop 80,80,320,200 \
    --max-full-normalized-rms 0 --max-crop-normalized-rms 0 \
    expected-640x480.png actual-640x456.png
expected_size: 640x480
actual_size: 640x456
error: image sizes differ; RMS requires matching dimensions
exit status: 1
```

With crop-first enabled, each source is cropped before the equality check and the exact-zero
threshold passes:

```text
$ smg-pc-visual-diff --crop 80,80,320,200 --crop-first \
    --max-full-normalized-rms 0 --max-crop-normalized-rms 0 \
    expected-640x480.png actual-640x456.png
expected_size: 640x480
actual_size: 640x456
comparison_crop: 80,80,320,200
comparison_size: 320x200
full_rgb_rms: 0.000000
full_normalized_rms: 0.000000
crop_rgb_rms: 0.000000
crop_normalized_rms: 0.000000
exit status: 0
```

An out-of-bounds rectangle and `--crop-first` without `--crop` both return status 1 with a clear
diagnostic. An additional 640x480-versus-800x456 run also produced an exact-zero 320x200 result,
exercising different source row strides as well as different heights.

## End-to-end runner proof

The fresh `file_select_far` command in the parent README invoked the binary with
`--crop 0,150,640,240 --crop-first`. Its output was:

```text
expected_size: 640x456
actual_size: 640x480
comparison_crop: 0,150,640,240
comparison_size: 640x240
full_rgb_rms: 72.612429
full_normalized_rms: 0.284755
crop_rgb_rms: 72.612429
crop_normalized_rms: 0.284755
```

The runner exited 0 and wrote a passing manifest. `runner-dolphin-file-select.png` and
`runner-pcport-file-select.png` are the exact input screenshots from that invocation.

## Fixture hashes

```text
03728597bfcaecfb77b0fa305ed0479cfb44374ca1fc91bca483d5da7bc25243  expected-640x480.png
8db671deb3688d3bb16fcfcc25757d1ceacac239cc195f1c26456dffe83b5e80  actual-640x456.png
ca700cfbbd36852c5f95d78fbb4abf5b3d3bef6f8f3c373dbb22ee09361301de  runner-dolphin-file-select.png
f100e44af973418fa5140837b06f83b347b69d50cf035fa1ab015971756fa7c8  runner-pcport-file-select.png
```
