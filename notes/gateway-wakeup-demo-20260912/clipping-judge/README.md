# Original ClippingJudge and registry integration — 2026-09-12

The second ordinary Gateway link attempt lacked `ClippingJudge::calcViewingVolume` and `MR::getClippingJudge`. The reference TU now contains both complete original methods and the original three-point and normal/point `TPartition3` template bodies. The native judge and plane header are byte-identical copies.

The judge builds four side planes from the actual camera view/FOV/aspect and near/far planes along that view. The original clipping near distance is 500, or 100 while the actual CameraDirector subjective frame is positive; it deliberately does not use the projection near distance. The original level-zero camera far plane and seven authored distance levels remain intact.

The existing native ClippingDirector now constructs and initializes its actual ClippingJudge child, then moves it at the original category. The SceneObjHolder factory's existing NameObj capture adopts and reversely retires this child. Actor membership remains in the existing registry; its evaluator now forwards to the actual judge. The scheduler applies that registry after Director movement. The separate CameraPose reconstruction, cached-camera fallback, distance table and trigonometric sphere-frustum implementation were removed. ClippingActorHolder/view-group and ClippingGroupHolder ownership are outside this bounded change; authored group clipping still fails explicitly.

## Proof

- Fresh original compiler: exit 0. Whole TU text 97.79426%; recovered viewing volume 95.09189%; getter 100%; original movement and both sphere query overloads 100%. Plane helpers 99.888885% and 100%. See `source-proof.json` and raw objdiff.
- Eight affected production/test TUs independently compiled natively, exit 0 (`native-compile-results.json`). No shared build was run by this lane.
- Existing GravityMathFoundationTests linked against the current shared support archives and passed exit 0 (`gravity-run.json`). Added genuine SDK plane winding, normal preservation, all six tangent/separation boundaries, and conservative corner overlap coverage.
- OriginalCameraDirectorTests now exercise the actual Director child identity, all eight far planes, near-plane tangency, subjective near distance, FOV/aspect and translated/rotated views. Its existing actual camera-to-clipping scheduler order and scene retirement cases are preserved. This test was compiled, not run in this lane; its full original process initialization remains a prerequisite.
- Both existing direct actor evaluator tests migrated from CameraPose to explicitly supplied test planes on an actual ClippingJudge. They were compiled, not represented as full runtime passes.
- Parent reports the next ordinary retail-disc/Metal Gateway build passed and its run stopped before frames at the actual GameSystem scene-controller prerequisite. See the parent `third-build` and `first-run` receipts. This is not a working intro/chase claim.

## Explicit follow-ups held for startup priority

The existing native TVec3f::normalize still differs from reference: it uses a guarded reciprocal-magnitude scale instead of PSVECNormalize. The plane helper now exposes this older shared SDK discrepancy. Also C++ overload resolution turns the recovered unqualified tan(float-expression) call into native tanf while the retail compiler calls double tan. Parent requested keeping the broad normalize correction paused until the actual demo startup frontier is addressed. Neither difference is a justification for an actor-specific override.

`source-manifest.json` names the exact owned production, reference and test files. Root owns source-list activation, reference status updates, integrated validation and publication.
