# Camera and billboard basis: real or absent

The host camera, free-camera, J3D model, and JPC billboard paths no longer
replace degenerate vectors with guessed world axes. A camera or transform basis
must normalize successfully; otherwise the operation reports explicit absence
or throws at the compatibility boundary.

Changed host-side paths:

- `src/camera/StageStartCamera.*` exposes `InvalidCameraBasis`;
- `src/camera/CameraPose.cpp` rejects invalid view bases;
- `src/render/effects/JpcBillboard.cpp` rejects invalid billboard bases;
- `src/render/J3dModelRenderer.cpp` requires valid camera, normal, and mesh
  bases;
- `src/runtime/RuntimeContext.cpp` rejects degenerate free-camera bases.

Focused evidence is included in `StageStartCameraTests.cpp` and
`JpcBillboardTests.cpp`. The final aggregate matrix passed 31/31 tests, including
4/4 stage-start camera tests and 6/6 JPC billboard tests.
