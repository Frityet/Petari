# Authentic title showcase release

This release packages the strongest route that is currently real end to end:
the retail `TitleSequenceProduct` running on the PC host with resources loaded
from the user's disc through Aurora DVD.

![Retail Korean title at frame 210](hero-title-frame-210.png)

## What is in the showcase

- `pc-port/src/Game/Screen/TitleSequenceProduct.cpp` and `.hpp` are
  byte-identical to the regular decompilation source and header.
- The executable owns only host bootstrap, window, render-loop, input, and disc
  plumbing in `pc-port/src/showcase`; it does not modify or fork Game behavior.
- The run resolved `TitleLogo.arc`, `PressStart.arc`, and `SysPALInfo.arc` from
  the RMGK01 disc, loaded 1,994 Korean messages and 3,327 particle names, and
  emitted all seven retail title-logo effects.
- The public target is title-only. A direct stage mode was removed instead of
  skipping an unsupported `GlobalPlaneGravity` switch/follower lifecycle. The
  real stage route remains absent until the exact demo-simple-cast and
  base-matrix-follower closure exists.

## Artifact

- Archive: `pc-port/build/showcase-dist/smg-pc-title-showcase-linux-x86_64.tar.gz`
- Archive SHA-256:
  `f818fa6b2a2d404cbab0629728af8af7d4137f7795f5e53a46761254314290f9`
- Executable SHA-256:
  `acd6f950068ddcc040a4205497d7abdfd2112d4bcceeb25e13dd891e4c2ab1fe`
- Executable size: 23,294,984 bytes
- Parent source commit: `2862d051bf1ce58cef7e9dd6d59544c8755bec04`
- Aurora commit: `9465d71d2a329dfa097159e015921589db5a7848`

The bundle includes no disc image, extracted file, or copyrighted game asset.
It requires a user-owned readable disc image at launch.

## Verification

- Release build: passed.
- Packaged-binary RMGK01/Xvfb screenshot run: exit 0.
- `xmake test -g aurora -j 1`: 32/32 test targets passed.
- `ldd -r`: no missing library or symbol.
- Direct dynamic dependencies are only glibc, libstdc++, libgcc, libm, and the
  system loader; SDL, Dawn, Aurora, and other package dependencies are linked
  into the executable.

See `verification.log` for the exact commands and observed resource counts.
