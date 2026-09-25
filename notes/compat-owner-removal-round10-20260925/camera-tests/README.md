# Round 10 camera test cleanup

The removed camera branch supplied invented target bindings, standalone game/event/start-camera owners, a private view-output context, and a duplicate CameraSystemService. Its four dedicated test sources/targets and CameraTargetTestSupport helper are retired with those APIs. Their current pre-edit bytes are retained under `before/`, including existing working changes.

Retained tests:

- OnlyCamera keeps all seven direct original pose-math cases. Only two removed-wrapper lifetime/feedback cases are retired.
- CameraViewInterpolator keeps the existing forced-cut/interpolation gate, recursive timer/FOV, independent damping rates, and nearly-end threshold assertions with the same numeric expectations and tolerances. They now call the actual process CameraDirector matrix construction and owned interpolator, then query the actual CameraContext through MR. The existing process fixture supplies ownership; no new fixture framework is introduced. The original interpolator and view/FOV state are restored even on an assertion exception. Fake target publication, fake collision-service, and duplicate service contracts are retired.
- FeedbackRealOrAbsent removes only the duplicate service shake case; rumble and original missing-owner checks remain. AuroraNative removes only its service shake telemetry sub-block. PlayerUtilRealOrAbsent removes only the unrelated duplicate camera service state assertion and setup; player checks remain.

`tests/xmake.lua` retires the four obsolete targets and gives the retained interpolator target its actual process dependencies (`smg-pc-app`, `aurora-main`). No other existing target changes are overwritten. Original camera context/resource/director, process Mario camera, and CameraUtil missing-owner tests remain unchanged.

No builds or test runs were performed, following the requested reduced-testing pace. A source reference search finds no remaining deleted camera wrapper/service/scope APIs in tests. `owned-manifest.json` lists all 11 owned paths with exact before/after hashes; `scoped.patch` isolates this edit from the captured dirty baseline.
