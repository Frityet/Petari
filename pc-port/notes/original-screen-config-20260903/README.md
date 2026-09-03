# Original camera context and console screen configuration

The native build now selects the complete original `CameraContext.cpp` and
`RenderMode.cpp`, with their original headers. All four files are byte-identical
to the root source (`source-manifest.json`). No camera algorithm or Game source
was changed. The camera TU disables implicit floating-point contraction, matching
the scalar arithmetic boundary already verified in
`original-camera-context-20260903` (1,540 cases, 24,640 projection components).

`MR::isScreen16Per9` is the complete original SystemUtil method; the native
`MR::getScreenWidth` now uses its complete original body. Both query the actual
SC aspect flag. The previous inference from a published camera pose is removed.

RuntimeContext loads its title directory first, optionally imports an actual
console NAND tree from `SMGPC_NAND_DIR` with Preserve semantics, then owns the
actual SystemConfigService before creating JUTVideo or screen/camera objects.
JUTVideo receives the result of the original `MR::getSuitableRenderMode`.
Absent SYSCONF uses the original SDK accessor defaults. `SMGPC_SAVE_DIR` keeps
its existing current-title-directory meaning. The console import is read-only
with respect to the host tree; this checkpoint does not implement SCFlush or
console configuration export.

Validation in the regular macOS ARM64 LLVM 23 build:

- `smg-pc-original-camera-context-tests`: original CameraContext construction,
  valid/invalid SC aspect values, view/inverse, near/far clipping, 90-degree FOV,
  shake offsets, live aspect changes and NameObj retirement pass. This uses the
  real OS memory, SC owner and Game camera object without a RuntimeContext pose.
- `smg-pc-runtime-context-construction-tests`, with the actual RMGK01 RVZ and
  Metal: allocation and logger failures release SC and all existing owners;
  two successful startups import 4:3 and 16:9 SYSCONF files and select the exact
  original VI widths, screen widths and CameraContext aspects. Both retire all
  owners and return full MEM1 capacity. The test also loads the real scenario
  catalog. Existing Metal teardown messages report pending map abortion/device
  destruction; the process exits successfully.
- `smg-pc-app` builds with the new startup chain.

Logs retain commands/results in this directory. The complete CameraDirector,
scene camera selection, original effect-owner activation and jumping remain
unfinished. This is camera-context/bootstrap validation, not gameplay proof.
The independent VI capability correction is recorded separately in
`original-vi-boot-audit-20260903`.
