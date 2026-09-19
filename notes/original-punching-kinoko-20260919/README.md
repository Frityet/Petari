# PunchingKinoko dependency audit and bounded recovery — 2026-09-19

The existing canonical PunchingKinoko implementation is complete. Its MWCC object has 98.034805% text similarity; the GroundChecker dependency compiles at 100%. The native archive lacks GroundChecker and thirteen non-inline utility APIs. Exact symbol inventory is in api-symbol-audit.json and native-defined-symbols.log.gz.

ShadowVolumeLine recovery published as decomp commit cc77fe564c4da3f8ee35605134e2add83628022a; origin/pcp-decomp remote SHA independently verified.

Native import and factory registration remain withheld. No PunchingKinoko, GroundChecker, shadow, math, sensor, or factory source changes landed in the port. This is a deliberate coherent stopping point after the user's priority changed to explicit source CP932 conversion.

## Dependency closure still required

- Original GroundChecker source/header and original PunchingKinoko source/header.
- Original sensor wrappers: sendMsgEnemyAttackFlipWeak, sendMsgEnemyAttackFlipWeakJump, sendMsgEnemyAttackFlipToDir, sendMsgEnemyAttackFlipMaximumToDir, sendMsgEnemyAttackToBindedSensor, sendMsgToEnemyAttackBlow; directional wrappers also need sendMsgEnemyAttackMsgToDir. Existing original binder sorting/deduplication send helper is present.
- Original matrix helpers scaleMtxToDir, orthogonalize, turnMtxToYDirRate; the latter needs original turnQuatYDirRate. Donors exist.
- Original calcStarPointerWorldVelocityDirectionOnPlane, using actual controller past pointer position/velocity and camera projection. Donor exists.
- initShadowController, addShadowVolumeSphere, addShadowVolumeLine, setShadowDropDirectionPtr need real original controller/drawer ownership. ShadowControllerOwnership already owns original sphere drawers, but VolumeLine currently has no drawer. Do not use a null drawer as successful support.

## ShadowVolumeLine recovery

The line drawer donor lacked drawShape. Recovered it in decomp/src/Game/LiveActor/ShadowVolumeLine.cpp from the complete retail assembly. It builds the eight endpoints using the two controllers' drop positions, directions, widths and drop lengths; rejects degenerate line/cross products; emits two four-vertex quads and a ten-vertex triangle strip in the original winding/order. No invented defaults or actor-specific geometry.

MWCC/objdiff: drawShape 97.91011%; full 1012-byte text 98.114624%; data, constants and vtable 100%. verify-shadow-line.py independently compares every reference text instruction with main.dol range 0x8016ef38–0x8016f32c, masking only 62 documented ELF relocation bitfields; PASS. Retail DOL SHA1 25c5959534b3c21246c6c7e42021b916b41fb578. Existing destructor register differences are inherited.

No native ShadowVolumeLine import was made, and this evidence does not establish native drawing or PunchingKinoko gameplay.

## Sensor and matrix closure follow-up

The eleven utility bodies listed in `native-utility-body-audit.json` have now been copied exactly into the shared `GameActorSensorCompat`, `MtxCompat` and `GameMathCompat` providers. Seven sensor wrappers preserve the original message IDs, receiver return values, temporary directional sender position and restoration, and the existing actual Binder sorting/deduplication path. Four math functions retain the original formulas and mutation order. `TRotation3::setScale(float,float,float)` was absent from the native JGeometry header; its original nine assignments are imported, preserving translation. No actor/factory enablement belongs to this utility checkpoint.

The donor audit caught an actual `scaleMtxToDir` error: retail passes `(axisX, rDir, axisY)` to `setXYZDir` at `0x803EBAE4`, whereas the donor passed `(axisX, axisY, rDir)`. This swaps the local scale Y/Z axes in one side of the basis transformation. The one-line correction was made in decomp first and published as `f2f8112331e126ff597f9760fd8785719e8a47ab`; remote `origin/pcp-decomp` SHA matched after push. Its full 240-byte function match improves from 99.833336% to **100%**. `verify-mtx-scale.py` independently checks the reference ELF's complete 9,528-byte text against the retail DOL with only its 350 relocation bitfields masked. Commands, before/after objects, compressed objdiff reports and `MtxUtil-match-summary.json` retain the evidence.

The unchanged `turnMtxToYDirRate` donor is also 100% at the function level. Independent MWCC compilation also gives 100% for all seven sensor functions and `turnQuatYDirRate`; see `utility-donor-match-summary.json`. The unchanged `orthogonalize` donor has 75.155556% fuzzy similarity due to register/stack layout and instruction scheduling; its control flow and original cross/normalize/save-Z/restore-Z operations were inspected against the retail assembly. This checkpoint does not present that inherited function as a high fuzzy match. Its native behavior is checked separately against analytic axes and retained translation/Z magnitude.

`OriginalSensorMatrixTests.cpp` provides four pure analytic math cases and an optional `--original-sensors` real-process probe. The latter uses the initialized original GameScene and its real SensorHitChecker, actors and Binder, with explicit local actor ownership and teardown. Contact planes in that focused message test are supplied test inputs to the actual Binder; it does not claim to validate world collision generation. Existing original-process ownership requirements remain intact.

Native validation completed:

- `xmake build -y smg-pc-original-sensor-matrix-tests`: build/link PASS, 37.105 seconds for the shared header rebuild. The initial test-only missing `MetrowerksStdCompat.hpp` include failure is retained in `native-utility-build.log`; the corrected build is `native-utility-build2.log`.
- Pure executable mode: four math cases PASS (`native-math-tests.log`). They check analytic directional scaling including the recovered Y basis, nonunit direction preservation, fallback axis, original orthogonalization/translation/Z magnitude, quaternion rates including negative/greater-than-one, aliases, multiplication order, original zero/antiparallel branch, lack of implicit quaternion normalization, and matrix turn translation.
- `--original-sensors` with the real Korean disc and an internally selected fresh save directory: actual GameScene probe PASS at frame 56, repeated twice; normal exit 0 after **120 completed frames**, 9.377 seconds. Owned child PID 55286 was reaped, with no timeout. `native-sensor-process.json` records executable SHA256, command, exit and completion provenance; its companion log preserves the complete run.
- All eleven utility symbols are defined exactly once in the linked native game archive (`native-utility-symbols.json`). Exact donor-body comparison passes for all eleven (`native-utility-body-audit.json`).

The coordinated build also linked the peer's original-layout-group test and the root's frame-button/live-input test. The latter passed (`batched-frame-button-tests.log`); the layout agent owns its runtime result. These checks do not establish PunchingKinoko gameplay: actor/factory import and remaining shadow/pointer dependencies are still handled separately.

## Native closure resumed

The original-process movement run is now stable enough to resume the missing
actor's general dependencies. The exact existing donor body of
`calcStarPointerWorldVelocityDirectionOnPlane` has been added to the native
original pointer-owner provider. It reads the actual controller's past position
and velocity and uses the actual camera projection; no manufactured pointer or
camera owner is introduced. Initial compilation exposed the missing generic
`TVec2::subInline` surface. The decomp header expresses this as a reference to a
local temporary; the native JGeometry method returns the same subtraction by
value, retaining correct lifetime at the unchanged const-reference callsite.
This is currently build/test work in progress, not validated pointer interaction.

The general shadow-owner review also found that programmatic volume construction
currently receives the CSV default start/end offsets (100), while the original
`ShadowVolumeDrawer` constructor uses zero and only CSV setup applies100. The
upcoming line-drawer ownership work must keep these distinct. Existing original
line geometry was recovered and published earlier; no new geometry invention is
needed. Programmatic cross-actor endpoints must remain the real controllers,
and line CSV lookup must preserve original preceding/self-controller behavior.

## Canonical actor import and original-process probe preparation

The general shadow dependency is now published in parent commit `e1a848cbb0ea90d8642d1fea1d45d396b6386607`; its actual-process 120-frame command-geometry and lifetime probe passes. See `notes/original-shadow-line-20260919/`. A fresh native archive audit finds every non-template MR API named by PunchingKinoko (`api-symbol-audit-after-shadow.json`); the two unmatched names are existing inline templates, not missing linked providers.

Canonical `PunchingKinoko.cpp/.hpp` and `GroundChecker.cpp/.hpp` have now been imported. Both headers and GroundChecker CPP are byte-identical to decomp. PunchingKinoko CPP differs only by the compile-time CP932 include and fourteen literal wrappers; normalized source equality passes (`native-actor-import-equivalence.json`). No actor-specific behavior changes or factory row have been made.

`tests/OriginalProcessPunchingKinokoTests.cpp` has been prepared to initialize a temporary original actor from a real HeavensDoor placement under actual OriginalProcess owners, inspect the real model/sensors/shadow graph and GroundChecker, exercise original movement and joint calculation, then retire the temporary graph before another frame. It is not yet built or run. Its Xmake target is deliberately not appended during the current coordinated O2/performance build freeze. User-requested slow-runtime investigation has priority; compilation/link/runtime claims for the actor remain pending, and factory activation remains gated on actual actor closure and the separately owned pointer test.

The actor probe target was appended after the performance build freeze ended. Its first full executable link exposed one remaining transitive original dependency: `EffectUtil::emitEffectHitBetweenSensors` references `MR::calcPosBetweenSensors`. The exact thirteen-line existing canonical body has been added to the general sensor provider with its required MathUtil declaration include, coordinated with that provider's owner. No effect/actor-specific fallback was added. The link failure and initial missing-include diagnostic remain in `native-actor-build.log` and `native-actor-build2.log`.

`native-actor-build3.log` records full actor executable link **PASS** in 4.847 seconds under the current optimized debug configuration. `native-actor-linked-symbols.json` identifies the linked actor constructor/init/control/joint callback, GroundChecker and transitive sensor helper, with executable SHA256. Runtime validation is pending the user's prioritized performance run and the earlier pointer/shadow probe queue. The main binary was not relinked by these actor-only builds.

## Original pointer-plane integration

The root-imported `calcStarPointerWorldVelocityDirectionOnPlane` body is byte-identical to current donor. Native TVec2::subInline correctly returns a value instead of the donor's expired reference; the test binds that returned value to a const reference and checks subtraction.

`OriginalSensorMatrixTests` now reads the actual system StarPointerDirector and actual scene CameraContext during normal original-process execution. Ordinary scripted pointer input supplies motion, and independent double-precision pinhole/plane intersections verify camera-facing and oblique planes, normal sign/magnitude invariance, stationary zero output, both parallel-ray rejection branches, and unchanged output on failure. No fabricated camera/controller or gameplay mutation is used for this pointer check.

Final O2 result: `native-pointer-o2-process3.json/log`, exit0,120 completed frames in3.709s,30moving samples,53stationary samples and30second-ray-only parallel checks. PID65667 was reaped. Binary SHA256 `5db9eeb67c43cfb5f6c7171b028f175c20a8a8d901d3ce9740146a0c0946cb28`. The original sensor checks/retirement also pass twice in this process.

Both fixture failures remain preserved: first O2 run normalized double-precision FMA cancellation residuals for identical intersections; the test now treats sub1e-9 expected geometry lengths as zero. Second run completed all120frames and every executed geometry assertion, but4-pixel pointer steps did not trigger the guarded second-ray-only branch. Increasing ordinary scripted steps to8pixels exercised that branch30times without weakening its assertion.

Read-only live input file audit found no substantive defect under its documented atomic-file replacement and additive-button/last-span pointer semantics. Parser revisions validate all channels before publishing; input file size is bounded and disappeared/invalid files fail explicitly. Existing pure controller tests passed under O2.

The temporary post-frame actor run did **not** pass: `native-actor-process.json`/`.log` records exit 1 at 1.683 seconds with `Original model registration must precede actor-list allocation`. This is the correct original lifecycle restriction: model draw-list capacities have already been finalized when the existing completed-frame observer runs. The attempted temporary construction source is retained as `late-construction-probe.cpp`; no construction hook or relaxed draw-list check was added.

The parent directed validation through the ordinary original placement phase instead. The exact generic factory row (`PunchingKinoko`, original constructor, `PunchingKinoko` archive) is now provisionally enabled in the working tree. The revised actual-process probe only observes all fourteen authored/factory-created instances after twenty normal scene frames, including their real models, sensors, distinct bound GroundCheckers, all forty-two original shadow controllers and borrowed line/head bindings; normal process teardown must retire all retained actor/child/drawer identities. Publication remains gated on this real-placement runtime result. The separately owned original pointer/sensor and shadow probes now pass under O2.

## Actual ordinary factory validation — passed

The revised ordinary-placement probe passes. `native-placement-process.json`/`.log` record **exit 0**, **120 completed frames**, **3.678 seconds**, with PID 67278 reaped normally. At frame 57, after twenty normal scene frames, actual stage data and factory instances agreed on all **14 PunchingKinoko actors**, **14 distinct live GroundCheckers with Binders**, and **42 original shadow controllers**. Every actor had its actual model, nerve, pointer target, head/body sensors in real groups, two sphere drawers, line drawer, and the original borrowed head-position/gravity references to its own GroundChecker. All actual Ball joint matrices were finite. No actors were separately ticked or created after draw-list allocation. Normal OriginalProcess teardown retired all retained actor, GroundChecker and drawer identities plus the original shadow holder/ownership state.

Test executable SHA256: `7c9f1215870f3d29af0db8a83de06fd1a44ec9f75ef8442ccaab23aaa9695beb`. The normal focused build batch was coordinated by the build-toolchain agent; no simultaneous production edits or main executable replacement occurred during this probe.

The single generic factory row is now validated for publication. The actor and GroundChecker remain canonical source imports (only fourteen CP932 literal wrappers plus include in the actor CPP); the transitive sensor helper remains its exact original body. This gate establishes original placement, normal bounded startup/execution and teardown. It does **not** establish all attack/pointer interactions, a rendered-image comparison, rabbit captures, or Rosalina story progression. The separately validated pointer/sensor and shadow subsystems retain their own narrower evidence.

## Retail movement restoration — passed

The parent-authorized restoration of `Mario::retainMoveDir` replaces the existing native behavior drift with the complete verified canonical body, with only explicit CP932 literal wrappers. Direct retail constants/string/instruction checks and independent MWCC comparison (99.78261% for736bytes) support the donor. The actual-owner regression fails on the former wrong DrawStates mask and passes after restoration, including countdown and0.06/initial0.1 threshold cases with all temporary state restored.

`retention-green-process.json/log`:120 completed frames, exit0,3.310s, PID67791 reaped. Original sensor dispatch/retirement and actual pointer-camera geometry also pass (30moving,54stationary,30second-parallel cases). `steering-drift-audit.md` and `retention-native-body-audit.json` record exact scope and retail proof. These focused checks do not attribute previous chase trajectories to the drift or establish story progression.
