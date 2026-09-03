# Original OnlyCamera regression coverage

`pc-port/tests/OnlyCameraTests.cpp` exercises the actual imported `OnlyCamera`, actual `CameraMan`, and `CameraPoseParam`, then checks their ownership in `OriginalCameraView`. The target is `smg-pc-only-camera-tests`. No approximation or replacement camera calculation is compiled into the tests.

The numeric expectations come from the root class and the separate complete original-compiler/retail verification recorded in `README.md` and `compiler-evidence.json` in this directory. The source paths are `src/Game/Camera/OnlyCamera.cpp`, `src/Game/Camera/CameraPoseParam.cpp`, `src/Game/Camera/CameraDirector.cpp`, and `src/Game/Camera/CameraLocalUtil.cpp`. The tests use ordinary axis geometry and explicit boundary values, rather than evaluating a second copy of the controller algorithm.

## Cases

1. **First watch position:** coincidence yields the one-unit canonical negative-Z displacement. Nonzero distances 0.5 and 299 extend to 300; 300 and 450 retain their distance. The input eye becomes the desired-position reference.
2. **Later close watch position:** with an earlier 450-unit negative-Z watch displacement, input distances 0 and 0.5 reuse that displacement at the new eye. Exactly 1 switches to the 300-unit extension of the requested positive-X direction. Values 299, 300 and 450 cover the other distance boundary.
3. **Up and pose fields:** a first up vector `(0,3,4)` becomes `(0,0.6,0.8)` while looking down negative Z. The later calculation removes its Z component, giving positive Y. Watch-up and local/global offsets are copied without normalization; roll is copied. OnlyCamera retains its own FOV/front-offset/upper-offset fields and does not mutate the manager pose.
4. **Invalid up:** looking along positive Y with zero or parallel up produces positive-Z up. Later zero-up calculations preserve previous positive-Y up for matching or opposite Z views, and transport it to positive Z when turning toward positive Y.
5. **Ideal movement:** an explicit 100-unit lag with desired translation `(10,20,0)` yields `(11,20,0)` after one unit of acceleration. Another desired translation `(10,5,0)` yields `(23,25,0)` at speed 2. Holding the target still produces `(26,25,0)` at speed 3. The requested watch and desired-position reference remain distinct from the corrected eye.
6. **Braking, cap, and arrival:** at speed 10, distances 40 and exactly 50 distinguish braking from acceleration. Speed 3.5 and distance 5.5 distinguish the original integer-cast stopping threshold from a squared-speed replacement. Speed 99.5 reaches the cap of 100. Distances 0.5 and 0.25 cover arrival after acceleration and braking. Exactly one unit at initial speed zero reaches the desired point while retaining active state until the following calculation.
7. **Reset and zero-frame flag:** reset clears ideal activity, `_24`, and resetting, executes the canonical first-pose up recovery, and retains speed 8. It leaves the zero-frame flag set on the start path; the next safe path clears it. No test reads `_24` before initializing or resetting it. The retail verification confirms the flag has no read in these methods; the tests do not assign it an invented movement effect.
8. **Native manager integration:** a coincident raw eye/watch is corrected before matrix construction. Manager FOV 70 reaches the view while the processed OnlyCamera FOV remains 40. The raw manager pose is unchanged, and the manager receives the actual inverse view matrix. Projection metadata and output-scope restoration are checked. Destroying the native owner leaves the borrowed manager alive.
9. **Native pose lifetime and reset:** the processed pose outlives the first input manager. A second manager reuses the preceding watch displacement until an explicit reset; requesting reset alone does not calculate. The next update consumes it through the original first-pose path.

The ideal branch is deliberately enabled through the public original fields in its dedicated tests. The source audit found no current camera-side assignment that enables this branch. This tests the available original calculation without inventing a production trigger.

## Validation

Source and target registration are ready for the parent's serialized native build. No native build or runtime result is claimed by this test-writing task yet.

```sh
cd pc-port
xmake build smg-pc-only-camera-tests
xmake run smg-pc-only-camera-tests
```
